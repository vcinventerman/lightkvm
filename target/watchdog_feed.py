import serial
import time

port = serial.serial_for_url('hwgrep://lightkvm', baudrate=115200)

while True:
    if not port.is_open:
        print('Reopening lost port')
        port = serial.serial_for_url('hwgrep://lightkvm', baudrate=115200)
        #port.open()

    port.write(b'1')
    port.flush()

    if (port.in_waiting > 0):
        output = port.read(port.in_waiting).decode('ascii').strip()
        print('Got data', output) 
    
    time.sleep(1)