# ClearShot 

Contributors:
- **Fisher Walsh <something-german-ithink>**
- **Cameron Pinchin <cwpinchin@outlook.com>**

## Overview

This is a hackathon project targeting QNX 8.0 which leverages open-source packages to deploy the LFD-NET model onto a Raspberry Pi 5. The model was trained on the [OHAZE] dataset to reduce the impact of smog, fog, or general 'haze' within images.

The project relies on the Raspberry Pi Camera Module 3 to continually capture frames from the camera and subsequently feed that data to the model for inference. 

OSS Packages can be found at <oss.qnx.com>.

The LFD-NET paper inwhich this project was based can be found here: <link>.

## Installation

This project relies on the following APKs:

- Pillow
- Numpy
- python3-tflite-runtime

which can be installed on QNX targets running the QSTI / the QNX Developer Self-Hosted environment, otherwise you'll need to cross-compile each package for the target.

You require this entire project on the Pi (for now), but can be run with the [run.sh] script.

#### Steps 

1. chmod +x run.sh

2. Depending on the model you wish to run, (256, 512) do the following:

  i] ./run.sh 256

 ii] ./run.sh 512

