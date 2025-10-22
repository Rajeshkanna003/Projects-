import cv2
import numpy as np
import HandTrackingModule as htm
import time
import pyautogui
import math

##########################
wCam, hCam = 640, 480
frameR = 100  # Frame Reduction
smoothening = 7
#########################

pTime = 0
plocX, plocY = 0, 0
clocX, clocY = 0, 0

cap = cv2.VideoCapture(0)  # Use default webcam
cap.set(3, wCam)
cap.set(4, hCam)

detector = htm.handDetector(maxHands=1)
wScr, hScr = pyautogui.size()  # Get screen width and height

while True:
    # 1. Find hand Landmarks
    success, img = cap.read()
    img = detector.findHands(img)
    lmList, bbox = detector.findPosition(img)

    if len(lmList) != 0:
        x1, y1 = lmList[8][1:]  # Index finger tip
        x2, y2 = lmList[4][1:]  # Thumb tip

        # 2. Check which fingers are up
        fingers = detector.fingersUp()

        # 3. Draw boundary
        cv2.rectangle(img, (frameR, frameR), (wCam - frameR, hCam - frameR), (255, 0, 255), 2)

        # 4. Cursor Control - Move in Correct Direction (Only Index & Thumb Open)
        if fingers[1] == 1 and fingers[0] == 1 and sum(fingers) == 2:
            x3 = np.interp(x1, (frameR, wCam - frameR), (wScr, 0))  # Reverse X direction
            y3 = np.interp(y1, (frameR, hCam - frameR), (0, hScr))

            # Smooth movement
            clocX = plocX + (x3 - plocX) / smoothening
            clocY = plocY + (y3 - plocY) / smoothening

            pyautogui.moveTo(clocX, clocY)  # Move cursor
            cv2.circle(img, (x1, y1), 15, (255, 0, 255), cv2.FILLED)

            plocX, plocY = clocX, clocY

        # 5. Left Click - Thumb & Index Close
        length, img, lineInfo = detector.findDistance(4, 8, img)
        if length < 40:
            cv2.circle(img, (lineInfo[4], lineInfo[5]), 15, (0, 255, 0), cv2.FILLED)
            pyautogui.click()
            time.sleep(0.2)  # Avoid multiple clicks

        # 6. Scroll Page - All Fingers Open and Rotate Right/Left
        if sum(fingers) == 5:
            x9, y9 = lmList[9][1:]  # Base of middle finger
            x0, y0 = lmList[0][1:]  # Wrist
            angle = math.degrees(math.atan2(y9 - y0, x9 - x0))

            if angle > 30:  # Hand rotated right (Scroll Down)
                pyautogui.scroll(-10)
                cv2.putText(img, "Scrolling Down", (50, 50), cv2.FONT_HERSHEY_PLAIN, 2, (0, 255, 0), 2)
                time.sleep(0.1)
            elif angle < -30:  # Hand rotated left (Scroll Up)
                pyautogui.scroll(10)
                cv2.putText(img, "Scrolling Up", (50, 50), cv2.FONT_HERSHEY_PLAIN, 2, (0, 255, 0), 2)
                time.sleep(0.1)

    # 7. Display FPS
    cTime = time.time()
    fps = 1 / (cTime - pTime)
    pTime = cTime
    cv2.putText(img, str(int(fps)), (20, 50), cv2.FONT_HERSHEY_PLAIN, 3, (255, 0, 0), 3)

    cv2.imshow("Image", img)
    cv2.waitKey(1)
import cv2
import numpy as np
import HandTrackingModule as htm
import time
import pyautogui
import math

##########################
wCam, hCam = 640, 480
frameR = 100  # Frame Reduction
smoothening = 7
#########################

pTime = 0
plocX, plocY = 0, 0
clocX, clocY = 0, 0

cap = cv2.VideoCapture(0)  # Use default webcam
cap.set(3, wCam)
cap.set(4, hCam)

detector = htm.handDetector(maxHands=1)
wScr, hScr = pyautogui.size()  # Get screen width and height

while True:
    # 1. Find hand Landmarks
    success, img = cap.read()
    img = detector.findHands(img)
    lmList, bbox = detector.findPosition(img)

    if len(lmList) != 0:
        x1, y1 = lmList[8][1:]  # Index finger tip
        x2, y2 = lmList[4][1:]  # Thumb tip

        # 2. Check which fingers are up
        fingers = detector.fingersUp()

        # 3. Draw boundary
        cv2.rectangle(img, (frameR, frameR), (wCam - frameR, hCam - frameR), (255, 0, 255), 2)

        # 4. Cursor Control - Move in Correct Direction (Only Index & Thumb Open)
        if fingers[1] == 1 and fingers[0] == 1 and sum(fingers) == 2:
            x3 = np.interp(x1, (frameR, wCam - frameR), (wScr, 0))  # Reverse X direction
            y3 = np.interp(y1, (frameR, hCam - frameR), (0, hScr))

            # Smooth movement
            clocX = plocX + (x3 - plocX) / smoothening
            clocY = plocY + (y3 - plocY) / smoothening

            pyautogui.moveTo(clocX, clocY)  # Move cursor
            cv2.circle(img, (x1, y1), 15, (255, 0, 255), cv2.FILLED)

            plocX, plocY = clocX, clocY

        # 5. Left Click - Thumb & Index Close
        length, img, lineInfo = detector.findDistance(4, 8, img)
        if length < 40:
            cv2.circle(img, (lineInfo[4], lineInfo[5]), 15, (0, 255, 0), cv2.FILLED)
            pyautogui.click()
            time.sleep(0.2)  # Avoid multiple clicks

        # 6. Scroll Page - All Fingers Open and Rotate Right/Left
        if sum(fingers) == 5:
            x9, y9 = lmList[9][1:]  # Base of middle finger
            x0, y0 = lmList[0][1:]  # Wrist
            angle = math.degrees(math.atan2(y9 - y0, x9 - x0))

            if angle > 30:  # Hand rotated right (Scroll Down)
                pyautogui.scroll(-10)
                cv2.putText(img, "Scrolling Down", (50, 50), cv2.FONT_HERSHEY_PLAIN, 2, (0, 255, 0), 2)
                time.sleep(0.1)
            elif angle < -30:  # Hand rotated left (Scroll Up)
                pyautogui.scroll(10)
                cv2.putText(img, "Scrolling Up", (50, 50), cv2.FONT_HERSHEY_PLAIN, 2, (0, 255, 0), 2)
                time.sleep(0.1)

    # 7. Display FPS
    cTime = time.time()
    fps = 1 / (cTime - pTime)
    pTime = cTime
    cv2.putText(img, str(int(fps)), (20, 50), cv2.FONT_HERSHEY_PLAIN, 3, (255, 0, 0), 3)

    cv2.imshow("Image", img)
    cv2.waitKey(1)
