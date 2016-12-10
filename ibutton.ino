
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
  if (count++ % 3 != 0)
    return;
  
  state ^= 1;
  digitalWrite(led, state);
}

void loop(void) {
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];

  ds.reset_search();
  if ( !ds.search(addr)) {
      //Serial.print("No more addresses.\n");
      ds.reset_search();
      return;
  }

  /*Serial.print("R=");
  for( i = 0; i < 8; i++) {
    Serial.print(addr[i], HEX);
    Serial.print(" ");
  }*/
  Serial.printf("%02x-%02x%02x%02x%02x%02x%02x%02x%02x\r\n",addr[0],
   addr[7],addr[6],addr[5],addr[4],addr[3],addr[2],addr[1]);

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.print("CRC is not valid!\n");
      return;
  }

  /*if ( addr[0] == 0x10) {
      Serial.print("Device is a DS18S20 family device.\n");
  }
  else if ( addr[0] == 0x28) {
      Serial.print("Device is a DS18B20 family device.\n");
  }
  else {*/
      //Serial.print("Device family is not recognized: 0x");
      //Serial.println(addr[0],HEX);
      return;

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("P=");
  Serial.print(present,HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print( OneWire::crc8( data, 8), HEX);
  Serial.println();
}
