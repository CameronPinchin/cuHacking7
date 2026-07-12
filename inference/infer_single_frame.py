import argparse
import os
import subprocess
import time
import numpy as np
from PIL import Image

try:
    # deployment targets (QNX) ship the classic tflite_runtime wheel
    from tflite_runtime.interpreter import Interpreter
except ImportError:
    from ai_edge_litert.interpreter import Interpreter

INPUT_DIR = os.path.join("/", "data", "share", "model_input")
POLL_INTERVAL = 0.05


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


def request_frame():
    """Signal the producer that we're ready for a new frame."""
    result = subprocess.run(['bash', 'capture_and_process.sh'])
    return result.returncode


def wait_for_frame(input_dir, poll_interval=POLL_INTERVAL):
    """Block until a readable image appears in input_dir; return (path, Image)."""
    while True:
        try:
            filenames = sorted(os.listdir(input_dir))
        except FileNotFoundError:
            filenames = []

        for filename in filenames:
            path = os.path.join(input_dir, filename)
            if not os.path.isfile(path):
                continue
            try:
                image = Image.open(path)
                image.load()
            except Exception:
                # still being written, or not an image
                continue
            return path, image

        time.sleep(poll_interval)


def streaming_dehaze(model_path, input_directory, save_directory, num_threads):
    lfd_net = LFDNetTFLite(model_path, num_threads)
    os.makedirs(input_directory, exist_ok=True)
    os.makedirs(save_directory, exist_ok=True)

    while True:
        request_frame()
        path, image = wait_for_frame(input_directory)
        filename = os.path.basename(path)
        lfd_net.dehaze(image).save(os.path.join(save_directory, filename))
        image.close()
        os.remove(path)


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="dehaze streamed frames with a LiteRT (.tflite) LFD-Net")
    ap.add_argument("-m", "--model", default="lfd_net.tflite", help="path to the .tflite model")
    ap.add_argument("-id", "--input_directory", default=INPUT_DIR, help="directory frames appear in")
    ap.add_argument("-sd", "--save_directory", required=True, help="path to save results directory")
    ap.add_argument("-nt", "--num_threads", type=int, help="interpreter threads")
    args = vars(ap.parse_args())

    try:
        streaming_dehaze(args["model"], args["input_directory"], args["save_directory"], args["num_threads"])
    except KeyboardInterrupt:
        pass
