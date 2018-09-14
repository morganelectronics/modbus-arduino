import serial

right = 0;
wrong = 0;

with serial.Serial('COM5', 512000, timeout=0.0001) as ser:
    print('Port Open')
    while True:
        s = ser.read(100)        # read up to ten bytes (timeout)    
        if len(s):
            print(s.hex())

