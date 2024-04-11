import numpy as np
from picamera2 import Picamera2
from PIL import Image as im
import time
import cv2

def main():
    feed1 = start_cam(0)
    feed2 = start_cam(1)
    
    while (True):
        arr1 = cap_array(feed1)
        arr2 = cap_array(feed2)
    
        disp_array('Feed 1', arr1)
        disp_array('Feed 2', arr2)
        
        if cv2.waitKey(1) == ord('q'):
            break
        
    feed1.stop()
    feed2.stop()
    
    
def start_cam(camera):
    #start camera feed
    picam2 = Picamera2(camera)
    picam2.start()
    return picam2

def cap_array(feed):
    array = feed.capture_array("main")
    array = cv2.cvtColor(array, cv2.COLOR_RGB2BGR)
    return array

def disp_array(name, array):
    cv2.imshow(name, array)
    

main()