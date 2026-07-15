import argparse
import os

import numpy as np
from PIL import Image

try:
    # deployment targets (QNX) ship the classic tflite_runtime wheel
    from tflite_runtime.interpreter import Interpreter
except ImportError:
    from ai_edge_litert.interpreter import Interpreter


class LFDNetTFLite:

    def __init__(self, model_path, num_threads=None):
        self.interpreter = Interpreter(model_path=model_path, num_threads=num_threads)
        self.interpreter.allocate_tensors()
        self.input_detail = self.interpreter.get_input_details()[0]
        self.output_detail = self.interpreter.get_output_details()[0]
        _, self.height, self.width, _ = self.input_detail["shape"]

    def dehaze(self, image):
        original_size = image.size
        resized = image.convert("RGB").resize((self.width, self.height), Image.BICUBIC)

        hazy_image = np.asarray(resized, dtype=np.float32) / 255.0
        hazy_image = np.expand_dims(hazy_image, 0)

        self.interpreter.set_tensor(self.input_detail["index"], hazy_image)
        self.interpreter.invoke()
        dehaze_image = self.interpreter.get_tensor(self.output_detail["index"])[0]

        dehaze_image = np.clip(dehaze_image * 255.0, 0, 255).astype(np.uint8)
        return Image.fromarray(dehaze_image).resize(original_size, Image.BICUBIC)


def multiple_dehaze_test(model_path, test_directory, save_directory, num_threads):
    lfd_net = LFDNetTFLite(model_path, num_threads)
    os.makedirs(save_directory, exist_ok=True)

    for filename in os.listdir(test_directory):
        path = os.path.join(test_directory, filename)
        try:
            image = Image.open(path)
        except Exception:
            continue
        lfd_net.dehaze(image).save(os.path.join(save_directory, filename))


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="dehaze images with a LiteRT (.tflite) LFD-Net")
    ap.add_argument("-m", "--model", default="lfd_net.tflite", help="path to the .tflite model")
    ap.add_argument("-td", "--test_directory", required=True, help="path to test images directory")
    ap.add_argument("-sd", "--save_directory", required=True, help="path to save test results directory")
    ap.add_argument("-nt", "--num_threads", type=int, help="interpreter threads")
    args = vars(ap.parse_args())

    multiple_dehaze_test(args["model"], args["test_directory"], args["save_directory"], args["num_threads"])
