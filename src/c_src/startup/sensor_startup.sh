#!/bin/sh

# Startup script for the camera sensor and screen.
#  This script will be triggered at the end of the startup script.
#  - Designed for the headless, barebones variant of the RPi5
#

echo 'Initializing screen support...'
time screen -u 36:36 -c /usr/share/screen/graphics-rpi5-2xhdmi.conf >/dev/null 2>&1 # silences stderr, stdout
time waitfor /dev/screen/ 15
if [$? -ne 0 ]; then
    echo 'waitfor /dev/screen/ failed.'
fi

echo 'Dropping screen priviliges'
echo drop_privileges > /dev/screen/command

echo 'Initializing camera support...'
# i2c6 [i2c stuff] used by camera
# enable the correct [ RP1 MSIX I2C6/MIPIO/MIPI1 interrupts ]
#i2c-dwc-rpi5 is the I2C driver for the RPi5 board
msix-rp1 --mode=config 13lll,47,48
gpio-rp1 set 38 a3 pu
gpio-rp1 set 39 a3 pu
/system/bin/i2c-dwc-rpi5 -p0x1f00088000 -c200000000 -q0xad --u6

waitfor /dev/i2c6

if [ $? -ne 0 ]; then # $? -ne 0 checks if the most recently executed command failed by checking its return ( $? checks the status code / return value of the executed command )
    echo 'waitfor /dev/i2c6 failed.'
fi

# enable CSI2 PiSP Frontend
gpio-rp1 set 34 op dh pd

# start sensor process with the camera_module3 configuration file loaded
if [ -d /dev/screen ]; then
    echo 'Starting sensor framework...'
    sensor -U 521:521 -b external -r /data/share/sensor -c /system/etc/config/sensor/camera_module3.conf

    waitfor /dev/sensor/camera0
    if [ $? -ne 0 ]; then
        echo 'waitfor /dev/sensor/camera0 failed.'
    fi

else
    echo 'Error, screen not available. Sensor framework will not be started.'
fi
