#This file is to test using one function to display feeds from multiple cameras

import numpy as np
from picamera2 import Picamera2
from PIL import Image as im
import time
import cv2

def main():
    #start feeds from camera ports 0 & 1
    feed1 = start_cam(0)
    feed2 = start_cam(1)

    #Continuously update a numpy array from the video feeds
    while (True):
        arr1 = cap_array(feed1)
        arr2 = cap_array(feed2)
    
        disp_array('Feed 1', arr1)
        disp_array('Feed 2', arr2)

        #break loop and end video feed if q key is pressed
        if cv2.waitKey(1) == ord('q'):
            break
    #stop displaying feeds
    feed1.stop()
    feed2.stop()
    
#takes camera port as input, outputs video feed from camera **If trying to use a multiplexer you will have to change this function to control GPio Pins and switch between cameras
def start_cam(camera):
    picam2 = Picamera2(camera)
    picam2.start()
    return picam2

#converts video feed to numpy array
def cap_array(feed):
    #gets the array
    array = feed.capture_array("main")
    
    #converts to black and white
    array = cv2.cvtColor(array, cv2.COLOR_RGB2BGR)
    return array
    
#just wrote a custom function to make displaying an array a little less cryptic
def disp_array(name, array):
    cv2.imshow(name, array)
    

main()
