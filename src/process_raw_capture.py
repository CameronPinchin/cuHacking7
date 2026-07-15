import os
import numpy as np
from PIL import Image

RAW_PATH = "/data/share/captures/frame.raw"
FINAL_PATH = "/data/share/model_input/input.jpg"
TMP_PATH = FINAL_PATH + ".tmp"

WIDTH, HEIGHT = 2304, 1296


def process_frame():
    raw = np.fromfile(RAW_PATH, dtype=np.uint8)

    # Derive stride from the actual file size so padded rows don't break us
    if raw.size % HEIGHT != 0:
        raise ValueError(
            f"raw size {raw.size} is not a multiple of height {HEIGHT}; "
            "check WIDTH/HEIGHT against the viewfinder resolution"
        )
    stride = raw.size // HEIGHT
    if stride < WIDTH:
        raise ValueError(f"stride {stride} < expected width {WIDTH}")

    img = raw.reshape((HEIGHT, stride))[:, :WIDTH]

    # Image.BOX is area-averaging, the PIL equivalent of cv2.INTER_AREA
    resized = Image.fromarray(img, mode="L").resize((512, 448), Image.BOX)

    # TMP_PATH ends in .tmp, so PIL can't infer the format -- specify it
    resized.save(TMP_PATH, format="JPEG", quality=95)
    os.replace(TMP_PATH, FINAL_PATH)


if __name__ == "__main__":
    process_frame()
