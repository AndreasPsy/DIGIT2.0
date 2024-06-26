This is the first working model for running the Raspberry Pi based DIGIT 2.0.

The DIGIT 2.0 is an open-source tactile sensor based on Meta's DIGIT design. The
DIGIT is essentially a thumb-sized box with a camera on one end and a gel with dots drawn on it on the other.
The camera can track the deformation of the gel and, through the dot displacements, extrapolate tactile data. 
The DIGIT 2.0 replaces Meta's expensive custom camera PCB with Raspberry Pi components which are cheaper, more robust,
and allow for higher quality video at higher framerates. We are also developing different kinds of gel "finger pads" for 
different applications and have designed the housing to easily switch between these fingerpads.

My personal contributions included writing the code that takes a video feed from a Raspberry Pi camera, converts it to black and white, 
increases contrast, then uses adaptive thresholding to make a cleaner image. 
I also wrote the code for the socket connection that exports dot-tracking data from the Raspberry Pi to an external computer via ethernet
or wifi, and translated dot-tracking code written by graduate students Jimmy Pendaloza, Evan Harber, and Cole Ten 
for use with the Raspberry Pi. 

The way this code works is the user will first run SockClient.py on their personal computer, 
editing the Server (Raspberry Pi) IP address and Port if needed. The user will then run the main file on the 
Raspberry Pi, which will continually export dot displacements to the personal computer as a numpy array. The user must edit
SockClient.py to use the received displacement arrays as desired.
