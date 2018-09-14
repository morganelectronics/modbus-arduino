/*
    Balance.h - Header for Balance Library
    Used to send balanced manchester endoded data 0->01, 1->10
    and to put it in delimited frames with balanced start/stop bytes
    Copyright (C) 2018 Kurt Morgan
*/

#ifndef BALANCE_H
#define BALANCE_H

#include "stdint.h"
#include "stddef.h"

static const size_t  kszFFrameSize = 128;
static const uint8_t ku8BFStart = 0xAA;
static const uint8_t ku8BFStop  = 0x99;
static const uint8_t ku8BFEscape  = 0x66;
static const int16_t ks8NoByte  = -1;

// Receives a byte into a message frame
// Resets on ku8BFStart, marked whole on ku8BFStop
// Removes balancing
class ReceiveFrame{
    public:
        // Reset ready for new data
        void clearFrame(void);
        // Add a newly received byte
        // Returns true when the frame is complete
        // Won't over-run buffers if data is malformed, will work again on next correct byte
        // Ignores -1s
        bool receiveByte(int16_t data);
        
        // The Decoded Frame
        uint8_t* pGetFrame(void);
        size_t getFrameSize(void);

     private:
        bool _low = true;
        bool _escaped = false;
        bool _complete = false;
        size_t _num_in_buffer = 0;
        uint8_t _buffer[kszFFrameSize];
        
        bool _receiveDataByte(uint8_t data);
};

// Function to make a balanced frame from a stream of bytes
// Puts in the start/stop flags, balances and escapes the data 
class SendFrame{
    public:
        // Have to initialise data buffer to have start byte - not clear as a default
        SendFrame(void);

        // Reset ready for new data (putting in a start byte)
        void clearFrame(void);
        // Put new balanced data into frame, escaping as required
        // Won't over-run buffers if not enough buffer space, but frame will be mal-formed
        void addByte(uint8_t data);
        void addBytes(uint8_t *data, size_t len);
        
        // The Decoded Frame
        uint8_t *pGetFrame(void); // Put on the 
        size_t getFrameSize(void);

     private:
        size_t _num_in_buffer;
        uint8_t _buffer[kszFFrameSize];
};


// Function to convert a buffer

uint8_t getBalancedLow(const uint8_t *pData, size_t index);
uint8_t getBalancedHigh(const uint8_t *pData, size_t index);

void clearSetUnBalancedLow(uint8_t *pData, size_t index, uint8_t balancedLow );
void setUnBalancedHigh(uint8_t *pData, size_t index, uint8_t balancedHigh );

#endif // BALANCE_H