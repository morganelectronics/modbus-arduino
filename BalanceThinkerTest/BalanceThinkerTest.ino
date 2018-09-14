
/*

  Basic.pde - example using ModbusMaster library

  Library:: ModbusMaster
  Author:: Doc Walker <4-20ma@wvfans.net>

  Copyright:: 2009-2016 Doc Walker

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

*/

#include <ModbusMaster.h>

const int HOLDING_REGISTER_START_LSW = 100;
const int HOLDING_REGISTER_START_MSW = 101;
const int SLAVE_ID = 1;
const int BROADCAST_ID = 0;

#define THINKER_BOARD
#ifdef THINKER_BOARD
  const uint32_t BAUD_RATE = 512000;
  const int PIN_NRE = 5;
  const int PIN_DE = 4;
  #define mySerial Serial
  const int USR_LED = 2;
#else
  const uint32_t BAUD_RATE = 4800;
  const int PIN_NRE = 9;
  const int PIN_DE = 8;
  #include <SoftwareSerial.h>
  SoftwareSerial mySerial(10, 11); // RX, TX
  const int USR_LED = LED_BUILTIN;
#endif

const bool BALANCED = true;
ModbusMaster node(BALANCED);

uint16_t _t15; // inter character time out
uint16_t _t35; // frame delay

// Call before transmission
// Set the TXE pin before dropping the RE pin, so that the device doesn't enter shutdown
void preTransmission( void ){
    // Set NRE mode to start with, so that device doens't enter shut-down
    digitalWrite(PIN_NRE, HIGH);
    digitalWrite(PIN_DE, HIGH);
}

// Call after transmission
// Set the TXE pin before dropping the RE pin, so that the device doesn't enter shutdown
void postTransmission( void ){
    digitalWrite(PIN_DE, LOW);
    // Set NRE mode last, so that device doens't enter shut-down
    digitalWrite(PIN_NRE, LOW);
}

void setup()
{
  // LED to indicate all is well.
  // Will remain on if all is well, or blink if there is a problem
  digitalWrite(USR_LED, HIGH);
  pinMode(USR_LED, OUTPUT);
  
  // Set RE mode to start with
  digitalWrite(PIN_NRE, LOW);
  digitalWrite(PIN_DE, LOW);
  pinMode(PIN_NRE, OUTPUT);
  pinMode(PIN_DE, OUTPUT);

  //Serial.begin(9600);
  
  // use //Serial (port 0); initialize Modbus communication baud rate
  mySerial.begin(BAUD_RATE);

  // communicate with Modbus slave ID 1 over //Serial (port 0)
  node.preTransmission(&preTransmission);
  node.postTransmission(&postTransmission);
}

uint32_t i = 0;

void loop()
{
  bool ok = true;

  if (BAUD_RATE > 19200) {
        _t35 = 1750;
    } else {
        _t35 = (int16_t)(35000000/BAUD_RATE); // 1T * 3.5 = T3.5
    }

  node.begin(0, mySerial); // Send half of messages to broadcast address
    
  // set word 0 of TX buffer to least-significant word of counter (bits 15..0)
  node.setTransmitBuffer(0, lowWord(i));
  
  // set word 1 of TX buffer to most-significant word of counter (bits 31..16)
  node.setTransmitBuffer(1, highWord(i));
  
  // slave: write TX buffer to (2) 16-bit registers starting at register 0
  auto result = node.writeMultipleRegisters(HOLDING_REGISTER_START_LSW, 2);
  if (result != node.ku8MBResponseTimedOut)
  { 
    //Serial.print("Err Wr ");
    //Serial.println((uint8_t)result, HEX);
    ok = false;
  }

  delayMicroseconds(_t35<<1);

  // Have to read from actual address
  node.begin(SLAVE_ID, mySerial);
  
  // slave: read (2) 16-bit registers starting at register 2 to RX buffer
  size_t tries = 0;
  do{
    result = node.readHoldingRegisters(HOLDING_REGISTER_START_LSW, 2);
    tries++;
  }while( (tries < 10) && !(node.ku8MBSuccess == result) );

  if (result != node.ku8MBSuccess)
  {
    //Serial.print("Err Rd ");
    //Serial.println((uint8_t)result, HEX);
    ok = false;
  }

  // do something with data if read is successful
  if (result == node.ku8MBSuccess)
  {
    uint16_t response1 = node.getResponseBuffer(0);
    uint16_t response2 = node.getResponseBuffer(1);
    if( (response1!=(i&0xFFFF))||
        (response2!=(i>>16))){
      //Serial.print("Err Cmp - should be: ");
      //Serial.print(i, HEX);
      //Serial.print(" was: ");
      //Serial.println(response1 | (response2<<16), HEX);
      ok = false;  
    }
  }
  
  i++;

  if(ok){
    //Serial.println("Ok ");
  }
  else{
    digitalWrite(USR_LED, LOW); // Flash Off
  }

  delay(1000);
  digitalWrite(USR_LED, HIGH);
}
