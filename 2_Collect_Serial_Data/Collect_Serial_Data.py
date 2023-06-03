import os
try:
    import serial
    import serial.tools.list_ports
except ImportError:
    input('pyserial not installed, install it with "py -3 -m pip install pyserial"')

ports = serial.tools.list_ports.comports()

for port, desc, hwid in sorted(ports):
    print("{}: {} [{}]".format(port, desc, hwid))
print('\n')

comport = input('port: ')
baudrate = 115200

try:
    ser = serial.Serial(
        port = comport, 
        baudrate=baudrate, 
        bytesize=8, 
        timeout=1, 
        stopbits=serial.STOPBITS_ONE
    )
except:
    input('\n port could not be opened. Press any key and retry.')

ser.reset_input_buffer()
print('reading port {}, finish with Ctrl+C'.format(comport))
with open('2_Collect_Serial_Data/Serial_Data_Fan_3_normal.csv', 'a') as f:
    try:
        while True:
            f.write(str(ser.read().decode()))

    except KeyboardInterrupt:
        ser.close()
input('finished reading. Press any key to exit.')