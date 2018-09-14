/*
  Modbus-Arduino Example - Lamp (Modbus Serial)
  Copyright by André Sarmento Barbosa
  http://github.com/andresarmento/modbus-arduino
*/
 
#include <Modbus.h>
#include <ModbusSerial.h>

#define DOER_BOARD

#ifdef DOER_BOARD
  const uint32_t BAUD_RATE = 512000;
  const int PIN_NRE = D2;
  const int PIN_DE = D3;
  #define mySerial Serial
#else
  const uint32_t BAUD_RATE = 4800;
  const int PIN_NRE = 9;
  const int PIN_DE = 8;
  #include <SoftwareSerial.h>
  SoftwareSerial mySerial(10, 11); // RX, TX
#endif

// Modbus Registers Offsets (0-9999)
const int HOLDING_REGISTER_START_LSW = 100;
const int HOLDING_REGISTER_START_MSW = 101;
const int SLAVE_ID = 1;

// ModbusSerial object
const bool BALANCED = true;
ModbusSerial mb(BALANCED);

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
    pinMode(LED_BUILTIN, OUTPUT);

    // Set RE mode to start with
    digitalWrite(PIN_NRE, LOW);
    digitalWrite(PIN_DE, LOW);
    pinMode(PIN_NRE, OUTPUT);
    pinMode(PIN_DE, OUTPUT);
    
    // Config Modbus Serial (port, speed, byte format) 
    mySerial.begin(BAUD_RATE);
    
    mb.preTransmission(&preTransmission);
    mb.postTransmission(&postTransmission);
    mb.config(&mySerial, BAUD_RATE);
    
    // Set the Slave ID (1-247)
    mb.setSlaveId(SLAVE_ID);

    mb.addHreg(HOLDING_REGISTER_START_LSW,0);
    mb.addHreg(HOLDING_REGISTER_START_MSW,0);
}

void loop() {
   // Call once inside loop() - all magic here
   mb.task();
   
   // Flash light on change 
   digitalWrite(LED_BUILTIN, mb.Hreg(HOLDING_REGISTER_START_LSW)&0x1);
   //digitalWrite(LED_BUILTIN,LOW);
   //delay(100);
   //digitalWrite(LED_BUILTIN,HIGH);
   //delay(100);
}
