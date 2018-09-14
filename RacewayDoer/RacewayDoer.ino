/*
Code to test raceway pairing
*/

#include <EEPROM.h>
#include <Modbus.h>
#include <ModbusSerial.h>
#include <Bounce2.h>
#include <Balance.h>

//Modbus Registers Offsets (0-9999)
const int HREG_COUNT = 1;
const int HREG_NEXT_ID = 2;
const int HREG_ID = 3;
uint32_t count = 0;
uint32_t last_count = 0;
uint8_t next_id = 0;

const uint32_t EIGHTH = 125;
const uint32_t HALF = 500;
const uint32_t SECOND = 1000;

const uint8_t MASTER_ID = 1; // Master always 1 on raceway segment
const uint8_t FIRST_ID = 2; // Master always 1 on raceway segment

//Used Pins
//LED_BUILTIN/USR_BTN
const int NotRecieveEnable = D2;
const int TransmitEnable = D3;

//State for config
typedef enum Config { UNCONFIGURED, CONFIGURE_ACTIVE, CONFIGURED } Config_t;
Config_t state = UNCONFIGURED;

//Debouncer for button
Bounce debouncer = Bounce();

uint8_t led_on = 0;
uint8_t flash_count = 0;
// Wraps 50 days
uint32_t last_flash_time_ms = 0;

// ModbusSerial object
ModbusSerial mb(true);
uint8_t client_id = 0;

// Function to get ID stored in EEPROM
// Returns 0 if not set properly
uint8_t getID(){
  uint8_t id = 0;
  if( EEPROM.read(0) == ~(EEPROM.read(1)) ){
    if( EEPROM.read(0) == EEPROM.read(2) ){
      id = EEPROM.read(1);
    }
  }
  return id;
}

// Function to set ID stored in EEPROM
void setID(uint8_t id){
      EEPROM.write(0,id);
      EEPROM.write(1,~id);
      EEPROM.write(2,id);
}
 
// the setup function runs once when you press reset or power the board
void setup() {
    // initialize digital pin LED_BUILTIN as an output & the button with a pull-up.
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(USER_BTN, INPUT_PULLUP);
    digitalWrite(LED_BUILTIN, HIGH);

    // Initialise RS485 bus driver
    digitalWrite(NotRecieveEnable, LOW);
    digitalWrite(TransmitEnable, LOW);
    pinMode(NotRecieveEnable, OUTPUT);
    pinMode(TransmitEnable, OUTPUT);
    
    // Serial1 used for a console
    Serial1.begin(9600);
    
    // Primary Serial used for modbus, has a transmit enable and receive-enable for RS485
    mb.config(&Serial, 256000, SERIAL_8N1, TransmitEnable, NotRecieveEnable);
    // Set the Slave ID - can be zero for broadcast only
    client_id = getID();
    mb.setSlaveId(client_id);
    if( client_id ){
      state = CONFIGURED;
    }
    else{
      state = UNCONFIGURED;
    }

    // Registers for Modbus
    mb.addHreg(HREG_COUNT,0);
    mb.addHreg(HREG_NEXT_ID,0);
    mb.addHreg(HREG_ID,client_id);
}

// the loop function runs over and over again forever
void loop() {
  // Modbus serial processing
  mb.task();

  uint16_t count = mb.Hreg(HREG_COUNT);

  // Start configuring if recieving broadcast of next id and it's different
  // Don't want ot re-enter configuration if ID just set
  if( next_id != mb.Hreg(HREG_NEXT_ID) ){
    next_id = mb.Hreg(HREG_NEXT_ID);
    if( FIRST_ID == next_id ){
      state = CONFIGURE_ACTIVE;
    }
  }

  // Slave - button press causes slave to get ID and be configured 
  if ( debouncer.fell() ) {
    if( CONFIGURE_ACTIVE == state ){
      client_id = next_id;
      mb.Hreg(HREG_ID,client_id);
      mb.setSlaveId(client_id);
      state = CONFIGURED;
      // Force flash pattern to start immediately
      count = 0;
      last_count = 1;
    }
  }
  
  // Alternating flash pattern for configure active state
  if( CONFIGURE_ACTIVE == state ){
    if( ( millis() - last_flash_time_ms ) > HALF ){
      if(led_on){
        digitalWrite(LED_BUILTIN, LOW);
        led_on = 0;
      }
      else{
        digitalWrite(LED_BUILTIN, HIGH);
        led_on = 1;
      }
      last_flash_time_ms = millis();
    }
  }

  // Starts on
  // id 1, count 1,  off
  // id 2, count 3, off, on, off
  // id 2, count 5, off, on, off, on, off

  // Flash pattern for showing address
  // Start every 4s, so that the flashes all normally start together
  if( last_count != count ){
      last_count = count;
      Serial1.println(count,DEC);
      if( (0 == (last_count & 0x4)) && (0 == flash_count) && (CONFIGURED == state) ){
        flash_count = ((client_id-1)<<1) + 1; // (id-1)*2 - 1
        last_flash_time_ms = millis();
        led_on = 1;
        digitalWrite(LED_BUILTIN, HIGH);
      }
  }
  if( flash_count > 0  ){
    if( ( millis() - last_flash_time_ms ) > EIGHTH ){
      flash_count--;
      last_flash_time_ms = millis();
      if(led_on){
        digitalWrite(LED_BUILTIN, LOW);
        led_on = 0;
      }
      else{
        digitalWrite(LED_BUILTIN, HIGH);
        led_on = 1;
      }
    }
  }
  
  if (Serial1.available() > 0) {
    // Echo the incoming byte:
    int incomingByte = Serial1.read();
    Serial1.write(incomingByte);
  }

}

