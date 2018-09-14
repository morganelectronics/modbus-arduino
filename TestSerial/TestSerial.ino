/*
 Just send some balanced words to see if they get through.
*/


//Used Pins
//GPIO5 RE_ 
//GPIO4 DE
//GPIO12 D6 Button
const int NotRecieveEnable = 5;
const int TransmitEnable = 4;

long sample_time_ms = 0;
 
// the setup function runs once when you press reset or power the board
void setup() {

  sample_time_ms = millis();
  
    // Initialise RS485 bus driver
    digitalWrite(NotRecieveEnable, LOW);
    digitalWrite(TransmitEnable, LOW);
    pinMode(NotRecieveEnable, OUTPUT);
    pinMode(TransmitEnable, OUTPUT);
    
    // Serial1 used for a console
    Serial.begin(512000);
}

// the loop function runs over and over again forever
void loop() {
  
  // Transmit every second
  if (millis() > sample_time_ms + 1000) {
    sample_time_ms = millis();
    digitalWrite(TransmitEnable, HIGH);
    digitalWrite(NotRecieveEnable, HIGH);
    
    Serial.write(0x55);
    Serial.write(0x56);
    Serial.write(0x59);
    Serial.write(0x5A);
    Serial.write(0x65);
    Serial.write(0x66);
    Serial.write(0x69);
    Serial.write(0x6A);
    Serial.write(0x95);
    Serial.write(0x96);
    Serial.write(0x99);
    Serial.write(0x9A);
    Serial.write(0xA5);
    Serial.write(0xA6);
    Serial.write(0xA9);
    Serial.write(0xAA);

    Serial.flush();

    delayMicroseconds (50);
    digitalWrite(TransmitEnable, LOW);
    digitalWrite(NotRecieveEnable, LOW);
  }

}

