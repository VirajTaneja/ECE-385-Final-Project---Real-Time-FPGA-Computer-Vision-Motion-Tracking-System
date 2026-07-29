import socket

import serial

import time

 

MAC_IP = ""  # <--- FIND AND PUT YOUR MAC'S IP HERE
UDP_PORT = "

COM_PORT = ""       # Check Device Manager!

 

# 1. Open Serial

try:

    ser = serial.Serial(COM_PORT, 115200, timeout=0)



except:



    exit()

 

# 2. Setup UDP

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

sock.bind(("0.0.0.0", UDP_PORT)) # Listen on our side too

sock.settimeout(1.0) # Don't freeze forever if Mac is slow

 



 

while True:

    try:

        # Keep poking the Mac so it knows where we are

        sock.sendto(b"hello", (MAC_IP, UDP_PORT))

       

        # Try to catch a coordinate

        data, addr = sock.recvfrom(1024)

       

        if data == b"hello":

            continue # Ignore our own echoes/pings

           

        # If we got here, we have REAL data!

        ser.write(data)



       

    except socket.timeout:



        continue

    except Exception as e:

        print(f"Error: {e}")

