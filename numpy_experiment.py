import numpy as np
from picamera2 import Picamera2
from PIL import Image as im
import time
import cv2

def start_cam(camera):
    #start camera feed
    picam2 = Picamera2(camera)
    picam2.start()

    array = picam2.capture_array("main")
    picam2.capture_file("test.jpg")
    picture = im.fromarray(array)
    picture = picture.convert('RGB')
    picture.save('convertedTest.jpg')
    
    picam2.stop()

main()