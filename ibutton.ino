
#include <OneWire.h>
#include <TimerOne.h>

OneWire ds(16);  // iButton reader on pin 16

const int led = 13;

void setup(void) {
  // initialize inputs/outputs
  // start serial port
  Serial.begin(9600);

  pinMode(led, OUTPUT);
  Timer1.initialize(~0); // still too often
  Timer1.attachInterrupt(blinkLED);
}

void blinkLED(void)
{
  static int state = HIGH, count = 0;
  if (count++ % 4 != 0) // make it blink a little less often
    return;
  
  state ^= 1;
  digitalWrite(led, state);
}

void loop(void) {
  byte addr[8];

  ds.reset_search();
  if ( !ds.search(addr)) {
      ds.reset_search();
      return;
  }


  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.print("CRC is not valid!\n");
      return;
  }

  Serial.printf("%02x-%02x%02x%02x%02x%02x%02x%02x%02x\r\n",addr[0],
   addr[7],addr[6],addr[5],addr[4],addr[3],addr[2],addr[1]);

}
