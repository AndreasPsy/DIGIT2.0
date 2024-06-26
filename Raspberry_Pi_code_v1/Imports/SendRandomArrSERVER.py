#This code creates a UDP Server for another computer to connect with over Ethernet, 
#then generates random 2x3 arrays and sends them to the connected computer
#This is mainly the proof-of-concept of how we're going to send dot positions from the Pi to an external computer for processing
#THIS CODE IS FOR USE ON THE PI, if its to be used on anything other than the biomechatronics user account on our pi 5, 
#SERVER_IP and SERVER_PORT will likely have to be changed

import socket
import sys
import time
import numpy as np
from picamera2 import Picamera2
import cv2

SERVER_IP = '192.168.1.150'
SERVER_PORT = 5900
BUFFER_SIZE = 65507

def main():
    sock = setupServer()

    #Confirms a Client has connected and receives their address
    Client_addy = ConnectConfirm(sock)
    #bytesToSend = conv_img('smaller.jpg')
    #sock.sendto(bytesToSend, Client_addy)

    #Loops continuously to create random arrays and send them to the client address
    while True:
        arr1 = np.random.randint(50, size=(5,6,2), dtype=np.int32)
        print(arr1)
        bytesToSend = arr1.tobytes()   
        print("Size: ", sys.getsizeof(bytesToSend))
    
        sock.sendto(bytesToSend, Client_addy)
        time.sleep(100)

def setupServer():
    #establish the socket under UDP Protocol
    RPIsocket = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    #Bind the socket to the RPI IP address and the correct Server Port
    RPIsocket.bind((SERVER_IP,SERVER_PORT))
    print('Server is up and listening . . .')
    return RPIsocket
    
    
def ConnectConfirm(RPIsock):
    #Receives a connection confirmation message from the Client along with their IP address and the port they connect on
    message,address = RPIsock.recvfrom(BUFFER_SIZE)
    
    #decodes message from bytes into string, then prints the confirmation message and returns the address
    message = message.decode('utf-8')
    print(message)
    return address
    
def conv_img(file_name):
    image = cv2.imread(name)
    image = cv2.cvtColor(image, cv2.COLOR_RGB2GRAY)
    cv2.waitkey(0)
    return image

#see previous files for explanations of start_cam and cap_array
def start_cam(camera):
    #start camera feed
    picam2 = Picamera2(camera)
    picam2.start()
    return picam2
    
def cap_array(feed):
    array = feed.capture_array("main")
    array = cv2.cvtColor(array, cv2.COLOR_RGB2GRAY)
    return array
    

main()
