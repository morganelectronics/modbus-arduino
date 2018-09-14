/*
LED flashes if the balance code encodes & decodes data.
LED on other-wise.
*/

#define Serial1 Serial

#include <Balance.h>

// the setup function runs once when you press reset or power the board
void setup() {
    // initialize digital pin LED_BUILTIN as an output & the button with a pull-up.
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN,HIGH);
    // Serial1 used for a console
    Serial1.begin(9600);
}

uint8_t all_the_possible_values[256];
uint8_t all_the_possible_encoded_values[512];

// the loop function runs over and over again forever
void loop() {
  bool ok = true;
  
  Serial1.println("Making Data");
  for(uint8_t i = 0; i <255; i++ ){
    all_the_possible_values[i]=i;
  }

  Serial1.println("Encoding Data");
  for(uint8_t i = 0; i <255; i++ ){
    all_the_possible_encoded_values[i<<1]=getBalancedLow(all_the_possible_values, i);
    all_the_possible_encoded_values[(i<<1)+1]=getBalancedHigh(all_the_possible_values, i);
  }

  Serial1.println("Decoding Data");
  for(uint8_t i = 0; (i<255) && ok ; i++ ){
    clearSetUnBalancedLow(all_the_possible_values, i, all_the_possible_encoded_values[i<<1]);
    if( (0xF & all_the_possible_values[i]) != (i&0xF) ){
      Serial1.print("Whole byte should be: 0x");
      Serial1.print(i,HEX);
      Serial1.print(" low nibble was 0x");
      Serial1.println(all_the_possible_values[i],HEX);
      ok = false;
    }
    setUnBalancedHigh(all_the_possible_values, i, all_the_possible_encoded_values[(i<<1)+1]);
    if( (0xF0 & all_the_possible_values[i]) != (i&0xF0) ){
      Serial1.print("Whole byte should be: 0x");
      Serial1.print(i,HEX);
      Serial1.print(" high nibble was 0x");
      Serial1.println(all_the_possible_values[i]>>4,HEX);
      ok = false;
    }
  }

  Serial1.println("Framing");
  {
    if(ok){
      uint8_t data[] = {0x00};
      uint8_t frame_expected[] = {ku8BFStart,0x55,0x55,ku8BFStop};    
      ok = CheckFraming(data,sizeof(data),frame_expected,sizeof(frame_expected));
    }
  }

  Serial1.println("Escape Start/Stop/Escape");
  {
    if(ok){
      uint8_t data[] = {0xF0,0xA5};
      uint8_t frame_expected[] = {ku8BFStart,0x55,ku8BFEscape,ku8BFStart,ku8BFEscape,ku8BFEscape,ku8BFEscape,ku8BFStop,ku8BFStop};    
      ok = CheckFraming(data,sizeof(data),frame_expected,sizeof(frame_expected));
    }
  }

  Serial1.println("Empty");
  {
    if(ok){
      uint8_t data[] = {};
      uint8_t frame_expected[] = {ku8BFStart,ku8BFStop};    
      ok = CheckFraming(data,sizeof(data),frame_expected,sizeof(frame_expected));
    }
  }
  
  Serial1.println("Extra Start Stuff");
  {
    ReceiveFrame rf;
    rf.receiveByte(-1);
    rf.receiveByte(0x55);    
    rf.receiveByte(0x69);
    rf.receiveByte(ku8BFStart);
    rf.receiveByte(0x69);    
    rf.receiveByte(0x9A);
    rf.receiveByte(ku8BFStop);
    if(ok &&
       (( rf.getFrameSize() != 1 ) ||
        ( rf.pGetFrame()[0] != 0xB6 )
       )
       ){
      Serial1.println("Didn't Ignore Extra Start Stuff");
    }
  }
  
  Serial1.println("Extra End Stuff");
  {
    ReceiveFrame rf;
    rf.receiveByte(ku8BFStart);
    rf.receiveByte(0x69);    
    rf.receiveByte(0x9A);
    rf.receiveByte(ku8BFStop);
    rf.receiveByte(-1);
    rf.receiveByte(0x55);    
    rf.receiveByte(0x69);
    if(ok &&
       (( rf.getFrameSize() != 1 ) ||
        ( rf.pGetFrame()[0] != 0xB6 )
       )
       ){
      Serial1.println("Didn't Ignore Extra End Stuff");
    }
  }
  
  if(ok){
    Serial1.println("Pass");
    while(1){
      digitalWrite(LED_BUILTIN,LOW);
      delay(500);
      digitalWrite(LED_BUILTIN,HIGH);
      delay(500);
    }
  }
  else{
      digitalWrite(LED_BUILTIN,LOW);
      while(1);
  }

}

bool CheckFraming(uint8_t *pData, size_t dataLen, uint8_t *pExpectedFrame, size_t frameLen)
{
    bool ok = true;
    SendFrame sf;
    sf.addBytes(pData,dataLen);
    if(ok && (sf.getFrameSize() != frameLen)){
      Serial1.println("Send Frame Wrong Size");
      ok = false;
    }
    if(ok && (memcmp(sf.pGetFrame(),pExpectedFrame,frameLen) != 0)){
      Serial1.println("Send Frame Wrong Data");
      ok = false;
    }
    ReceiveFrame rf;
    bool end;
    for( size_t i=0; i<sf.getFrameSize(); i++){
          end = rf.receiveByte(sf.pGetFrame()[i]);
          if((i!=(sf.getFrameSize()-1))&&end){
            Serial1.println("Ended too soon");
            ok = false;
          }
    }
    if(ok && !end){
      Serial1.println("Didn't end");
      ok = false;
    }
    if(ok && (rf.getFrameSize() != dataLen)){
      Serial1.print("Receive Frame Wrong Size: ");
      Serial1.println(rf.getFrameSize(),DEC);
      ok = false;
    }
    if(ok && (memcmp(rf.pGetFrame(),pData,dataLen) != 0)){
      Serial1.println("Receive Frame Wrong Data");    
      ok = false;
    }
    return ok;
}

