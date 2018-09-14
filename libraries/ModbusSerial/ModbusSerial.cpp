/*
    ModbusSerial.cpp - Source for Modbus Serial Library
    Copyright (C) 2014 André Sarmento Barbosa
*/
#include "ModbusSerial.h"
#include <Balance.h>

ModbusSerial::ModbusSerial( bool balance): _balance(balance),_preTransmission(0),_postTransmission(0) {
    _reply = MB_REPLY_OFF;
}

void ModbusSerial::preTransmission(void (*preTransmission)())
{
  _preTransmission = preTransmission;
}

void ModbusSerial::postTransmission(void (*postTransmission)())
{
  _postTransmission = postTransmission;
}

bool ModbusSerial::setSlaveId(byte slaveId){
    _slaveId = slaveId;
    return true;
}

byte ModbusSerial::getSlaveId() {
    return _slaveId;
}

void ModbusSerial::config(Stream* port, uint32_t baud ) {
    this->_port = port;

    if (baud > 19200) {
        _t15 = 750;
        _t35 = 1750;
    } else {
        _t15 = 15000000/baud; // 1T * 1.5 = T1.5
        _t35 = 35000000/baud; // 1T * 3.5 = T3.5
    }

    // Get ready to receive
    if(_postTransmission){
        _postTransmission();
    }
}

bool ModbusSerial::receive(byte* frame) {
    //first byte of frame = address
    byte address = frame[0];
    //Last two bytes = crc
    u_int crc = ((frame[_len - 2] << 8) | frame[_len - 1]);

    //Slave Check
    // 0 is broadcast address
    if (address != BROADCAST_ADDRESS && address != this->getSlaveId()) {
		return false;
	}

    //CRC Check
    if (crc != this->calcCrc(_frame[0], _frame+1, _len-3)) {
		return false;
    }

    //PDU starts after first byte
    //framesize PDU = framesize - address(1) - crc(2)
    this->receivePDU(frame+1);
    //No reply to Broadcasts
    if (address == BROADCAST_ADDRESS) _reply = MB_REPLY_OFF;
    return true;
}

bool ModbusSerial::send(byte* frame) {
    byte i;

    if(_preTransmission){
        _preTransmission();
    }

    //Send slaveId
    _sf.clearFrame();
    _sf.addBytes(frame,_len);

    (*_port).write(_sf.pGetFrame(),_sf.getFrameSize());
    (*_port).flush();
    delayMicroseconds(_t35);

    if(_postTransmission){
        _postTransmission();
    }
}

bool ModbusSerial::sendPDU(byte* pduframe) {

    if(_preTransmission){
        _preTransmission();
    }

    //Send slaveId
    _sf.clearFrame();
    _sf.addByte(_slaveId);

    //Send PDU
    byte i;
    _sf.addBytes(pduframe,_len);

    //Send CRC
    word crc = calcCrc(_slaveId, pduframe, _len);
    _sf.addByte(crc >> 8);
    _sf.addByte(crc & 0xFF);

    (*_port).write(_sf.pGetFrame(),_sf.getFrameSize());
    (*_port).flush();
    delayMicroseconds(_t35);

    if(_postTransmission){
        _postTransmission();
    }
}

void ModbusSerial::task() {
    if( _reply != MB_REPLY_OFF ){
        if( millis() - _receive_time > ku32Delay_ms ){
            if (_reply == MB_REPLY_NORMAL)
            {
                this->sendPDU(_frame);
            }
            if (_reply == MB_REPLY_ECHO){
                this->send(_frame);
            }
            _rf.clearFrame();
            _reply = MB_REPLY_OFF;
        }
    }
    else{
         if(_rf.receiveByte((*_port).read())){
            _frame=_rf.pGetFrame();
            _len=_rf.getFrameSize();
        
            if (receive(_frame)){
                _receive_time = millis();
            }
        }
    }
}

word ModbusSerial::calcCrc(byte address, byte* pduFrame, byte pduLen) {
	byte CRCHi = 0xFF, CRCLo = 0x0FF, Index;

    Index = CRCHi ^ address;
    CRCHi = CRCLo ^ _auchCRCHi[Index];
    CRCLo = _auchCRCLo[Index];

    while (pduLen--) {
        Index = CRCHi ^ *pduFrame++;
        CRCHi = CRCLo ^ _auchCRCHi[Index];
        CRCLo = _auchCRCLo[Index];
    }

    return (CRCHi << 8) | CRCLo;
}


void ModbusSerial::write(uint8_t data){
    if(_balance)
    {
        (*_port).write(getBalancedLow(&data,0));
        (*_port).write(getBalancedHigh(&data,0));
    }
    else{
        (*_port).write(data);
    }
}
