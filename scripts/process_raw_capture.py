import numpy as np
import cv2
import os

RAW_PATH = "/data/share/captures/frame.raw"
FINAL_PATH = "/data/share/model_input/input.jpg"
TMP_PATH = FINAL_PATH + ".tmp"

WIDTH, HEIGHT = 2304, 1296

def process_frame():
    raw = np.fromfile(RAW_PATH, dtype=np.uint8)

    img = raw.reshape((HEIGHT, WIDTH))
    resized = cv2.resize(img, (512, 448), interpolation=cv2.INTER_AREA)
    
    cv2.imwrite(TMP_PATH, resized)
    os.rename(TMP_PATH, FINAL_PATH)

if __name__ == "__main__":
    process_frame()