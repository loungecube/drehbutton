#include <EEPROM.h>
#include <OneWire.h>
#include <TimerOne.h>

#define debug Serial.print
#define debugf Serial.printf
#define EEPROM_SIZE 128 // bytes

const int
  ledPin       = 13,
  learnmodePin = 14,
  ibuttonPin   = 16;

OneWire ds(ibuttonPin);  // iButton reader on pin 16

void setup(void)
{
  // initialize inputs/outputs
  // start serial port
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);
  pinMode(learnmodePin, INPUT_PULLUP); // connect to GND via a button

  Timer1.initialize(~0); // still too often
  //XXX feels like quite a lot of overkill to use a timer just to blink a LED but oh well, I'm not using them anyway
  Timer1.attachInterrupt(blinkLED);

  delay(4200); // hopefully Serial is set up by then
  debug("Hello!\r\n");
  dump_eeprom();
}

// check whether the learnmode button is being pushed
inline bool learnmode(void)
{
  return digitalRead(learnmodePin) == LOW;
}

void blinkLED(void)
{
  static int state = HIGH, count = 0;
  if (!learnmode() && count++ < 5)
    return; // blink slower if not in learnmode

  count = 0;
  state ^= 1; // toggle
  digitalWrite(ledPin, state);
}

void format_addr(char *str, byte (&addr)[8])
{
  sprintf(str, "%02x-%02x%02x%02x%02x%02x%02x",
   addr[0], // device type
   addr[6],addr[5],addr[4],addr[3],addr[2],addr[1]
  );
}

// squash a 6 byte iButton identifier into 4 bytes
uint32_t hash_addr(byte (&addr)[8])
{
  // there are a lot of zeroes in the addrs, so this should not collide too much.
  // EEPROM only has space for 32 32-bit values on Teensy LC, that's why.
  
  // On the other hand, maybe there is an even more collision-free way to do this.
  // Only time will tell *shrug*
  
  return (addr[0]         <<24)|
        ((addr[6]^addr[1])<<16)|
        ((addr[5]^addr[2])<< 8)|
        ((addr[4]^addr[3])    );
}

void learn_ibutton(byte (&addr)[8])
{
  size_t i, base;
  uint32_t hash = hash_addr(addr);

  for (i = 0; i < EEPROM_SIZE/4; i++) {
    base = i*4;
    if (EEPROM.read(base  ) == 0xFF &&
        EEPROM.read(base+1) == 0xFF &&
        EEPROM.read(base+2) == 0xFF &&
        EEPROM.read(base+3) == 0xFF)
      break;
  }

  if (i == EEPROM_SIZE/4) {
    debug("Error: Out of EEPROM space!\r\n");
    return;
  }

  EEPROM.write(base  , (hash>>24)&0xFF);
  EEPROM.write(base+1, (hash>>16)&0xFF);
  EEPROM.write(base+2, (hash>> 8)&0xFF);
  EEPROM.write(base+3, (hash    )&0xFF);

  char str[32];
  format_addr(str, addr);
  debugf("Learned iButton %d: %s (%x)\r\n", i, str, hash_addr(addr));
}

int find_ibutton(byte (&addr)[8])
{
  size_t i, base;
  uint32_t hash = hash_addr(addr);

  for (i = 0; i < EEPROM_SIZE/4; i++) {
    base = i*4;
    if (EEPROM.read(base  ) == ((hash>>24)&0xFF) &&
        EEPROM.read(base+1) == ((hash>>16)&0xFF) &&
        EEPROM.read(base+2) == ((hash>> 8)&0xFF) &&
        EEPROM.read(base+3) == ((hash    )&0xFF))
      return i;
  }

  return -1;
}

void dump_eeprom(void)
{
  size_t i, base;
  byte b[4];

  debug("I know about the following iButtons:\r\n");
  
  for (i = 0; i < EEPROM_SIZE/4; i++) {
    base = i*4;
    b[0] = EEPROM.read(base  );
    b[1] = EEPROM.read(base+1);
    b[2] = EEPROM.read(base+2);
    b[3] = EEPROM.read(base+3);
  
    if ((b[0] == 0xFF) &&
        (b[1] == 0xFF) &&
        (b[2] == 0xFF) &&
        (b[3] == 0xFF))
        continue; // don't print empty slots
        
    debugf("%d\t%02x%02x%02x%02x\r\n", i, b[0],b[1],b[2],b[3]);
  }
}

void loop(void)
{
  byte addr[8];
  ///static unsigned int count = 0;
  static bool got_button = false; // only handle the button once per contact
  // XXX should also probably cache the hash because the contact isn't always very stable

  ds.reset_search();
  if (!ds.search(addr)) {
      ds.reset_search();
      got_button = false;
      return;
  } else if (got_button) {
    return;
  }
  
  got_button = true;

  if (OneWire::crc8(addr, 7) != addr[7]) {
      debug("CRC is not valid!\r\n");
      return;
  }

  int ibutton = find_ibutton(addr);
  if (learnmode()) {
    if (ibutton != -1) {
      char str[32];
      format_addr(str, addr);
      debugf("I already know about iButton %s (index: %d)!\r\n", str, ibutton);
    } else {
      learn_ibutton(addr);
    }
    return;
  }

  // not in learnmode, authenticate.
  if (ibutton == -1) {
    debug("I'm sorry Dave, I'm afraid I can't do that.\r\n");
    // TODO: display failure message on display
  } else {
    debug("Missile system engaged. Target locked.\r\n");
    // TODO: write to the correct pin that has the relay connected
  }
}
