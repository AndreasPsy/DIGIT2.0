#This file was just for messing around with video to numpy arrays. It starts a feed, converts it to an array, 
# takes a picture directly from the feed(test.jpg) and then takes a picture from the array(convertedTest.jpg)

import numpy as np
from picamera2 import Picamera2
from PIL import Image as im
import time
import cv2

def start_cam(camera):
    #start camera feed
    picam2 = Picamera2(camera)
    picam2.start()

def main()
    feed1 = start_cam(0)
    array = feed1.capture_array("main")
    feed1.capture_file("test.jpg")
    picture = im.fromarray(array)
    picture = picture.convert('RGB')
    picture.save('convertedTest.jpg')
    
    picam2.stop()

main()
