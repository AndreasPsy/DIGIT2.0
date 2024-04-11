import numpy as np
from picamera2 import Picamera2
from PIL import Image as im
import time
import cv2

def main():
    feed1 = start_cam(0)
    ##feed2 = start_cam(1)
    

    while (True):
        arr1 = cap_array(feed1)
        arr1_con = contrast(arr1)
        ##arr2 = cap_array(feed2)
        ##arr3 = arr2 * 1.1
        ##arr3 = arr3.astype('int8')
        
        disp_array('Feed 1', arr1)
        ##disp_array('Feed 2', arr2)
        ##disp_array('feed 3', arr3)
        disp_array('Contrast', arr1_con)

        
        if cv2.waitKey(1) == ord('q'):
            break
    
    print(arr1.ndim)  
    feed1.stop()
    ##feed2.stop()
    
    
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