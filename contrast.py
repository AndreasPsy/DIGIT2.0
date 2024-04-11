#Reference DualFeed.py for explanations of start_cam(), cap_array(), and disp_array()
#This file adds the contrast algorithm to dual feed, and will display a contrasted black and white video feed in addition to the black and white feed 
    #Note: The contrast works by stretching brightness values across the full 255-value spectrum; 
    #      ie it makes the lightest pixel 0 and the darkest 255 and then scales all other pixels appropriately

import numpy as np
from picamera2 import Picamera2
from PIL import Image as im
import time
import cv2

def main():
    feed1 = start_cam(0)
    #feed2 = start_cam(1)
    

    while (True):
        arr1 = cap_array(feed1)
        arr1_con = contrast(arr1)
        #arr2 = cap_array(feed2)
        #arr2_con = contrast(arr2)
        
        disp_array('Feed 1', arr1)
        #disp_array('Feed 2', arr2)
        
        disp_array('Contrast 1', arr1_con)
        #disp_array('Contrast 2', arr2_con)

        
        if cv2.waitKey(1) == ord('q'):
            break
    
    feed1.stop()
    #feed2.stop()
    
    
def start_cam(camera):
    #start camera feed
    picam2 = Picamera2(camera)
    picam2.start()
    return picam2

def cap_array(feed):
    array = feed.capture_array("main")
    array = cv2.cvtColor(array, cv2.COLOR_RGB2GRAY)
    return array

def disp_array(name, array):
    cv2.imshow(name, array)
    
def contrast(array):
        array = ((array - array.min()) / (array.max() - array.min())) * 255
        array = array.astype(np.uint8)
        return array
    

main()
