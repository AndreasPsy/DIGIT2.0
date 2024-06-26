#This code captures and displays a masked video from raspberry pi cameras using adaptive thresholding
import numpy as np
from picamera2 import Picamera2
from libcamera import controls
from PIL import Image as im
import time
import cv2
            
def adap_thresh(array, blocksize, c_val):
    masked = cv2.adaptiveThreshold(array,255,cv2.ADAPTIVE_THRESH_MEAN_C, cv2.THRESH_BINARY_INV, blocksize,c_val)
    masked = masked.astype(np.uint8)
    return masked

#starts then returns a camera feed given a camera port
def start_cam(port):
    picam2 = Picamera2(port)
    picam2.start()
    return picam2

#creates a numpy array from a camera feed
def cap_array(feed):
    array = feed.capture_array("main")
    array = cv2.cvtColor(array, cv2.COLOR_BGR2GRAY)
    return array

#makes imshow more obvious
def disp_array(name, array):
    cv2.imshow(name, array)

#Contrasts a Black and White array by mapping values to use full 0-255 spectrum
def contrast(array):
        array = ((array - array.min()) / (array.max() - array.min())) * 255
        array = array.astype(np.uint8)
        return array
        
def conv_feed(feed):
    arr = cap_array(feed)
    con = contrast(arr)
    img = adap_thresh(con, 51, 7)
    
    return img



def main():
    feed1 = start_cam(0)
    #feed2 = start_cam(0)
    
    feed1.set_controls({"AfMode": controls.AfModeEnum.Manual, "LensPosition": 200})
    
    arr1 =cap_array(feed1)
    
    MJPG = cv2.VideoWriter.fourcc('h','2','6','4')
    
    #main_vid = cv2.VideoWriter('Black_White_Vid.avi', MJPG, 30, arr1.shape, False)
    #con_vid = cv2.VideoWriter('Contrasted_Vid.avi', MJPG, 30, arr1.shape, False)
    #masked_vid = cv2.VideoWriter('Masked_Vid.avi', MJPG, 30, arr1.shape, False)

    
    #feed2.set_controls({"AfMode": controls.AfModeEnum.Manual, "LensPosition": 200})
    while (True):
        arr1 = cap_array(feed1)
        arr1_con = contrast(arr1)
        masked = adap_thresh(arr1_con, 51, 7)
        
        #masked = conv_feed(feed1)
        
        #main_vid.write(arr1)
        #con_vid.write(arr1_con)
        #masked_vid.write(masked)
        
        #arr2 = cap_array(feed2)
        #arr2_con = contrast(arr2)
        #masked2 = adap_thresh(arr2_con, 51, 7)
        
        #disp_array('Feed 1', arr1)
        #disp_array('Feed 2', arr2)
        
        #disp_array('Contrast 1', arr1_con)
        #disp_array('Contrast 2', arr2_con)
        
        disp_array('Masked', masked)
        #disp_array('Masked 2', masked2)
        if cv2.waitKey(1) == ord('q'):
            break
            
#main()
