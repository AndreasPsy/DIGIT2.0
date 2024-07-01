#I (Andreas) only slightly altered detect_marker_centers and get_marker_displacements to work with the rest of my code, they were originally written by grad students Jimmy Penaloza, Cole Ten, and Evan Harber
#Everything in the digit_dot_tracking folder was entirely written by the above grad students

from digit_dot_tracking import Point2DMatcher
from Imports.MaskedVideo import start_cam, cap_array, contrast, adap_thresh, conv_feed, disp_array
from Imports.SockSERVER import setupServer, ConnectConfirm, conv_img
from libcamera import controls
from PIL import Image as im
from numpy.typing import NDArray
import numpy as np
import time
import cv2
import imutils
import faulthandler
import time

def detect_marker_centers(
    img: NDArray[np.uint8]
) -> tuple[NDArray[np.double], NDArray[np.uint8]]:
    # Find external contours in the image
    detected_contours = cv2.findContours(
        img.copy(), cv2.RETR_LIST, cv2.CHAIN_APPROX_SIMPLE
    )
    detected_contours = imutils.grab_contours(detected_contours)
    
    # Iterate over detected contours to find centers of markers
    marker_centers = []
    contour_image = img.copy()

    for (i, contour) in enumerate(detected_contours):
        # Compute the center of the contour
        if cv2.contourArea(contour) > 1000 or cv2.contourArea(contour) < 20:
            continue
        m = cv2.moments(contour)
        cx = int(m["m10"] / (m["m00"] + 1e-7))
        cy = int(m["m01"] / (m["m00"] + 1e-7))
        if cx and cy != 0:
            marker_centers.append([cx, cy])
        # draw the contour on a copy to publish as debugging tool
        cv2.drawContours(contour_image, [contour], -1, (255, 0, 255), 1)
        disp_array("contours", contour_image)
    
        if cv2.waitKey(1) == ord('q'): 
            break

    # return the image with contours drawn on it as well - Jimmy
    return np.asarray(marker_centers), contour_image

#Find distance between centers of contours and initial positions
def get_marker_displacements(marker_centers: NDArray[np.double]) -> NDArray[np.double]:
    #Find Initial Positions
    mark_centers = marker_centers.tolist()
    point_matcher.update_detected_points(marker_centers)
    point_matcher.match_points()
    (
        initial_x_positions,
        initial_y_positions,
        current_x_positions,
        current_y_positions,
        _,
    ) = point_matcher.calc_marker_displacements()

    #convert to numpy array
    initial_x_positions = np.asarray(initial_x_positions)
    initial_y_positions = np.asarray(initial_y_positions)
    current_x_positions = np.asarray(current_x_positions)
    current_y_positions = np.asarray(current_y_positions)

    marker_displacements = np.stack(
        [
            current_x_positions - initial_x_positions,
           current_y_positions - initial_y_positions,
        ],
        axis=-1,
    )

    return marker_displacements

if __name__ == "__main__":
    faulthandler.enable()
    sock = setupServer()
    Client_Address = ConnectConfirm(sock)
    
    point_matcher = Point2DMatcher(
        # Replace arguments based on specific digit
        num_grid_rows=5, # Number of dot rows
        num_grid_cols=6, # Number of dot columns
        camera_fps=30,
        point0_x_coord=150, # (Approximate) X position, in pixels, of top left dot center (with no deformation)
        point0_y_coord=170, # (Approximate) Y position, in pixels, of top left dot center (with no deformation)
        x_grid_spacing=50, # (Approximate) distance, in pixels, between dot centers between columns (x-axis)
        y_grid_spacing=50, # (Approximate) distance, in pixels, between dot centers between rows (y-axis)
    )

    #start video feed and set focus to closest setting
    feed = start_cam(0)
    feed.set_controls({"AfMode": controls.AfModeEnum.Manual, "LensPosition": 200})

    #allow lens time to focus
    time.sleep(1)
    
    while (True):
        #convert frame from RGB picture to Black and White, Adaptively Thresholded numpy array (see MaskedVideo.py in imports folder)
        img = conv_feed(feed)
        
        if cv2.waitKey(1) == ord('q'): 
            break

        marker_centers, contour_image = detect_marker_centers(img.copy())
        marker_displacements = get_marker_displacements(marker_centers)
        
        #send displacement data through socket connection
        bytesToSend = marker_displacements.tobytes()
        sock.sendto(bytesToSend, Client_Address)
