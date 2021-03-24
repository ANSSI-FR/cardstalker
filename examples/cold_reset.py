#!/usr/bin/python3



import serial



s = serial.Serial(port="/dev/ttyUSB0", baudrate=9600)
s.write("\x02\x00".encode());
s.flush()
r = s.read(2);
s.close();

print(r)
#print(":".join("{:02x}".format(ord(c)) for c in r))





