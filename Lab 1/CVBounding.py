# https://docs.opencv.org/4.x/df/d9d/tutorial_py_colorspaces.html
# https://docs.opencv.org/4.x/dd/d49/tutorial_py_contour_features.html

import numpy as np
import cv2

cap = cv2.VideoCapture(0)

# Upper and lower bounds for HSV "green," apparently hsv values in opencv range from 0 to 180

lower_green_hsv = np.array([30, 60, 30])
upper_green_hsv = np.array([90, 255, 255])

while (True):
    # Capture frame-by-frame from video stream
    _, frame = cap.read()

    # Convert to HSV from BGR
    transcodedFrame = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

    # Threshold the HSV image to only get the isolated color
    # From https://pythonprogramming.net/color-filter-python-opencv-tutorial/
    thresh = cv2.inRange(transcodedFrame, lower_green_hsv, upper_green_hsv)

    # Contour the area
    contours, _ = cv2.findContours(thresh, 1, 2)

    # Check if contours are empty and draw a bounding box around the frame
    if contours:
        # Find contour with the biggest area to draw
        cnt = max(contours, key=cv2.contourArea)

        # Bounding Rectangle
        rect = cv2.minAreaRect(cnt)
        box = cv2.boxPoints(rect)
        box = np.int0(box)

        # Draw red bounding box in frame
        cv2.drawContours(frame, [box], 0, (0, 0, 255), 2)
        # Draw white bounding box here in the threshold image
        cv2.drawContours(thresh, [box], 0, (255, 255, 255), 2)

    # Display the resulting frame
    cv2.imshow('frame', frame)
    cv2.imshow('rawThreshold', thresh)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# When everything done, release the capture
cap.release()
cv2.destroyAllWindows()