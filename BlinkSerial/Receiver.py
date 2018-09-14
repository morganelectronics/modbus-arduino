import serial

right = 0;
wrong = 0;

with serial.Serial('COM5', 512000, timeout=0.5) as ser:
    print('Port Open')
    while True:
        s = ser.read(20)        # read up to ten bytes (timeout)    
        if len(s):
            print(s.hex())
            //shouldbe=bytes.fromhex('5556595a6566696a9596999aa5a6a9aa')
            //if(shouldbe == s[:-1]):
            //    right = right+1;
            //else:
            //    wrong = wrong+1
            //    print(s[:-1].hex(), shouldbe.hex())
            //print('right ' + str(right) + ' wrong ' + str(wrong));
