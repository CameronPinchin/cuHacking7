#!/bin/sh

$ARG_256        = "256"
$ARG_512        = "512"

$MODEL_256      = src/models/lfd_net_outdoor_256.tflite
$MODEL_512      = src/models/lfd_net_outdoor_512.tflite

$INPUT_DIR      = /data/share/model_input
$OUTPUT_DIR     = /data/share/model_output

$NUM_THREADS    = 4 

if [ "$#" -ne 2 ]; then
    echo "ERROR: $0 opts: <256 || 512>" >&2 
    exit 1
fi

if [ "$1" == "$ARG_256" ]; then 
    echo "Running inference with model $MODEL_256"
    python -m $MODEL_512 -id $INPUT_DIR -sd $OUTPUT_DIR -nt $NUM_THREADS
    exit 1
fi

if [ "$1" == "$ARG_512" ]; then
    echo "Running inference with model $MODEL_512"
    python -m $MODEL_512 -id $INPUT_DIR -sd $OUTPUT_DIR -nt $NUM_THREADS
    exit 1
fi
