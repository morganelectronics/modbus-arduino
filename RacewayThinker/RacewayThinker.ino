/*
Code to test raceway pairing
Master side - push button to start/stop pairing
*/

#include <EEPROM.h>
#include <ModbusMaster.h>
#include <Bounce2.h>
#include <Balance.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "EE-BrightBox-rq7g79";
const char* password = "***********";
const char* host = "RacewayMaster";

ESP8266WebServer server(80);

// instantiate balanced ModbusMaster object
ModbusMaster node(true);

const uint8_t MASTER_ID = 1; // Master always 1 on raceway segment
const uint8_t FIRST_ID = 2; // Master always 1 on raceway segment

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

// Count of successful transactions
uint32_t read_tries=0;
uint32_t read_success=0;

//Used Pins
//LED_BUILTIN/USR_BTN
const int USR_LED = 2;
const int USR_BTN = 12;
const int NotRecieveEnable = 5;
const int TransmitEnable = 4;

//State for config
typedef enum Config { UNCONFIGURED, CONFIGURE_ACTIVE, CONFIGURED } Config_t;
Config_t state = CONFIGURED;

//Debouncer for button
Bounce debouncer = Bounce();

uint8_t led_on = 0;
uint8_t flash_count = 0;
// Wraps 50 days
uint32_t last_flash_time_ms = 0;
uint32_t broadcast_time_ms = 0;


void handleRoot() {
  String message = "";
  if( CONFIGURE_ACTIVE == state ){
        message += "Configuring, NextId: ";
        message += next_id;
        message += ", Count: ";
        message += count;
  }
  else{
        message += "Polling, Tries: ";
        message += read_tries;
        message += ", Success: ";
        message += read_success;
        message += ", Count: ";
        message += count;
  }
  server.sendHeader("Refresh","2");
  server.send(200, "text/plain", message);
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void preTransmission()
{
  digitalWrite(NotRecieveEnable, 1);
  digitalWrite(TransmitEnable, 1);
}

void postTransmission()
{
  digitalWrite(NotRecieveEnable, 0);
  digitalWrite(TransmitEnable, 0);
}

// the setup function runs once when you press reset or power the board
void setup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
    MDNS.begin("racewaymaster");
    server.on("/", handleRoot);
    server.onNotFound(handleNotFound);
    server.begin();
    
    debouncer.attach(USR_BTN,INPUT_PULLUP); // Attach the debouncer to a pin with INPUT_PULLUP mode
    debouncer.interval(25); // Use a debounce interval of 25 milliseconds

    // initialize digital pin LED_BUILTIN as an output & the button with a pull-up.
    pinMode(USR_LED, OUTPUT);
    digitalWrite(USR_LED, LOW);
    
    // Initialise RS485 bus driver
    digitalWrite(NotRecieveEnable, LOW);
    digitalWrite(TransmitEnable, LOW);
    pinMode(NotRecieveEnable, OUTPUT);
    pinMode(TransmitEnable, OUTPUT);
    
    // Primary Serial used for modbus, has a transmit enable and receive-enable for RS485
    Serial.begin(512000,SERIAL_8N1);
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
}

// the loop function runs over and over again forever
void loop() {
  server.handleClient();
  debouncer.update();

  if( ( millis() - broadcast_time_ms ) > SECOND ){
    broadcast_time_ms = millis();
    if( CONFIGURE_ACTIVE == state ){
        uint8_t result;
        
        // Master broadcasts non-zero ID to start configuration
        // Check if anyone has accepted the address before moving on
        node.begin(next_id, Serial);
        result = node.readHoldingRegisters(HREG_ID, 1);
        if (result == node.ku8MBSuccess)
        {
          next_id++;
        }
        
        // Broadcast next id
        node.begin(0, Serial);
        node.setTransmitBuffer(0, next_id);
        node.setTransmitBuffer(1, 0);
        result = node.writeMultipleRegisters(HREG_NEXT_ID, 1); // Ignore return code (timeout)
    }
    
    // Broadcast time for synchronised flashing, regardless of state
    node.begin(0, Serial);
    node.setTransmitBuffer(0, count++);
    node.setTransmitBuffer(1, 0);
    node.writeMultipleRegisters(HREG_COUNT, 1); // Ignore return code (timeout)
  }

  // Keep pinging the first raceway doer and keep stats
  if( CONFIGURED == state ){
    node.begin(MASTER_ID+1, Serial);
    read_tries++;
    uint8_t result = node.readHoldingRegisters(HREG_ID, 1);
    if (result == node.ku8MBSuccess)
    {
      read_success++;
    }
  }

  // Master - button press causes configuration to start/stop
  if ( debouncer.fell() ) {
    if( CONFIGURE_ACTIVE == state ){
      state = CONFIGURED;
      next_id = 0;
      read_tries = 0;
      read_success = 0;
    }
    else{
      state = CONFIGURE_ACTIVE;
      next_id = FIRST_ID;
    }
  }
  
  // Alternating flash pattern for configure active state
  if( CONFIGURE_ACTIVE == state ){
    if( ( millis() - last_flash_time_ms ) > HALF ){
      if(led_on){
        digitalWrite(USR_LED, LOW);
        led_on = 0;
      }
      else{
        digitalWrite(USR_LED, HIGH);
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
      if( (0 == (last_count & 0x4)) && (0 == flash_count) && (CONFIGURED == state) ){
        flash_count = ((MASTER_ID-1)<<1) + 1; // (id-1)*2 - 1
        last_flash_time_ms = millis();
        led_on = 1;
        digitalWrite(USR_LED, HIGH);
      }
  }
  if( flash_count > 0  ){
    if( ( millis() - last_flash_time_ms ) > EIGHTH ){
      flash_count--;
      last_flash_time_ms = millis();
      if(led_on){
        digitalWrite(USR_LED, LOW);
        led_on = 0;
      }
      else{
        digitalWrite(USR_LED, HIGH);
        led_on = 1;
      }
    }
  }
 
}

