/*
    Balance.c - Little library to manchester encode/decode data
    Copyright (C) 2018 Kurt Morgan
*/
#include "Balance.h"

// For each 16 bit nibble, return one with 1 bits set to 10 and 0 set to 01
const size_t bitsInNibble = 4;
uint8_t balance[] = {
    0x55, //0 01010101
    0x56, //1 01010110
    0x59, //2 01011001
    0x5A, //3 01011010
    0x65, //4 01100101
    0x66, //5 01100110 Esc
    0x69, //6 01101001
    0x6A, //7 01101010
    0x95, //8 10010101
    0x96, //9 10010110
    0x99, //A 10011001 Sto
    0x9A, //B 10011010
    0xA5, //C 10100101
    0xA6, //D 10100110
    0xA9, //E 10101001
    0xAA, //F 10101010 Sta
};

uint8_t getBalancedLow(const uint8_t *pData, size_t index){
    return balance[pData[index]&0x0F];
}

uint8_t getBalancedHigh(const uint8_t *pData, size_t index){
    return balance[pData[index]>>4];
}

void clearSetUnBalancedLow(uint8_t *pData, size_t index, uint8_t balancedLow  ){
    pData[index] = 0;
    pData[index] |= ((balancedLow>>1)&0x1);
    pData[index] |= ((balancedLow>>2)&0x2);
    pData[index] |= ((balancedLow>>3)&0x4);
    pData[index] |= ((balancedLow>>4)&0x8);
}

void setUnBalancedHigh(uint8_t *pData, size_t index, uint8_t balancedHigh ){
    pData[index] |= ((balancedHigh<<3)&0x10);
    pData[index] |= ((balancedHigh<<2)&0x20);
    pData[index] |= ((balancedHigh<<1)&0x40);
    pData[index] |= ((balancedHigh<<0)&0x80);
}

void ReceiveFrame::clearFrame(){
    _low = true;
    _escaped = false;
    _complete = false;
    _num_in_buffer = 0;
}

bool ReceiveFrame::receiveByte(int16_t data){
    if( -1 != data){
        if(_escaped){
            _escaped = false;
            _receiveDataByte((uint8_t)data);            
        }
        else{
            switch(data){
                case ku8BFStart:
                    clearFrame();
                    break;
                case ku8BFStop:
                    _complete = true;
                    break;
                case ku8BFEscape:
                    _escaped = true;
                    break;
                case ks8NoByte:
                    break;
                default:
                    _receiveDataByte((uint8_t)data);            
                    break;
            }
        }
    }
    return _complete;
}

uint8_t* ReceiveFrame::pGetFrame(void){
    return _buffer;
}

size_t ReceiveFrame::getFrameSize(void){
    return _num_in_buffer;
}

bool ReceiveFrame::_receiveDataByte(uint8_t data){
    if(_num_in_buffer<sizeof(_buffer)){
        if( !_complete ){ // Ignore any junk after stop and before new start
            if( _low )
            {
                clearSetUnBalancedLow(_buffer, _num_in_buffer, data);
                _low = false;
            }
            else{
                setUnBalancedHigh(_buffer, _num_in_buffer, data);
                _low = true;
                _num_in_buffer++;
            }
        }
    }
}

SendFrame::SendFrame(void){
    _num_in_buffer = 1;
    _buffer[0] = ku8BFStart;
}

void SendFrame::clearFrame(void){
    _num_in_buffer = 1;
    _buffer[0] = ku8BFStart;
};

void SendFrame::addByte(uint8_t data){
    // Don't add data past buffer end
    if(_num_in_buffer<sizeof(_buffer)){
        uint8_t low_val = getBalancedLow(&data,0);
        if( (ku8BFStart == low_val) || (ku8BFStop == low_val) || (ku8BFEscape == low_val) ){
            _buffer[_num_in_buffer] = ku8BFEscape;
            _num_in_buffer++;
        }
        _buffer[_num_in_buffer] = low_val;
        _num_in_buffer++;
        uint8_t high_val = getBalancedHigh(&data,0);
        if( (ku8BFStart == high_val) || (ku8BFStop == high_val) || (ku8BFEscape == high_val) ){
            _buffer[_num_in_buffer] = ku8BFEscape;
            _num_in_buffer++;
        }
        _buffer[_num_in_buffer] = high_val;
        _num_in_buffer++;
    }
};

void SendFrame::addBytes(uint8_t *data, size_t len){
    for( size_t i = 0; i < len; i++ ){
        addByte(data[i]);
    }
};

uint8_t* SendFrame::pGetFrame(void) {
    _buffer[_num_in_buffer] = ku8BFStop;
    return _buffer;
};

size_t SendFrame::getFrameSize(void) {
    return (_num_in_buffer + 1);
};
