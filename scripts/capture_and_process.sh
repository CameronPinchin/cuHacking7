while true
do
    ./camera-dump-frame-no-screen -o /data/share/captures/frame.raw
    python3 process_raw_capture.py
done