/*
  Ecample code to test features of raceway board
  2 Serial Ports, Analogue Input, Digitial PWM outut & EEPROM storage.
*/

#include <EEPROM.h>
#include <Modbus.h>
#include <ModbusSerial.h>


//Modbus Registers Offsets (0-9999)
const int IREG_RAW_BUS = 100;
const int IREG_RAW_A = 101;
const int HREG_PULLUP_D = 200;
const int HREG_PWM_THOU_D = 201;
const int HREG_EEPROM = 203;

//PWM Period
const int PWM_RANGE_THOU = 255;

//Used Pins
const int busAPin = A0;
const int APin = A1;
const int DisablePullDownPin = D12;
const int EnablePullUpPin = D13;

const int NotRecieveEnable = D2;
const int TransmitEnable = D3;

// ModbusSerial object
ModbusSerial mb;

long sample_time_ms;

word pullupValue = 0;
word pwmValue = 0;

int toggle = 0;
 
// the setup function runs once when you press reset or power the board
void setup() {
    // initialize digital pin LED_BUILTIN as an output & the button with a pull-up.
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(USER_BTN, INPUT_PULLUP);

    // Initialise RS485 bus driver
    digitalWrite(NotRecieveEnable, LOW);
    digitalWrite(TransmitEnable, LOW);
    pinMode(NotRecieveEnable, OUTPUT);
    pinMode(TransmitEnable, OUTPUT);

    // LED Off
    digitalWrite(LED_BUILTIN, LOW);
    
    
    // Serial used for bus
    Serial.begin(512000);

    // Serial1 used for a console
    Serial1.begin(512000);

    Serial1.println("Test");
}

// the loop function runs over and over again forever
void loop() {    
  if (Serial.available() > 0) {
    // read the incoming byte:
    uint8_t data = Serial.read();
    if(data == 0xAA){
      if( toggle ){
        toggle= 0;
        digitalWrite(LED_BUILTIN, LOW);
      }
      else{
        toggle= 1;
        digitalWrite(LED_BUILTIN, HIGH);
      }
    }
    Serial1.write(data);
  }
}

