# Snaptain Elite S5C Drone
*Cameron Pinchin*

The Snaptain Elite S5C is a consumer drone capable of operating at 80 - 100 meters (likely in perfect conditions), approximately ~10-15 minutes.

## Drone - Technical Notes

Establishing a programmatic connection to the drone has been the most challenging portion. My assumption was that the video is based on a UDP connection and just continually transmits datagrams on a specific port.

The Snaptain S5C base model has been reverse engineered and the communication protocol is known. The process is as follows: 
    
    1. The TCP component involves establishing four separate TCP connections to the drone over **port 8888** with a specific string attached.
    
    2. This TCP handshake is accepted which opens up the UDP stream for connections on **port 9125** . 
    
I tried replicating this on the **Snaptain Elite S5C** which didn't end up working. It appears that the TCP connections on port 8888 are not even open to begin with, leading me to believe they are on a different port or the process differs entirely from the non-Elite model. 

### How do we reverse engineer the Elite model? 

The plan is to use my phone to connect to the drone using the Snaptain FPV app. I would then use my PC to sniff the packets being sent back and forth to; identify the ports, and identify potential patterns in the communication.

This has yielded interesting results so far, having discovered **port 51167** as b

I have been using *tshark*, a cli-based version of WireShark to capture information about the transmissions between my phone and the drone. 

The drone seemingly struggles with multiple connections at once, so I switched the wifi network on my PC to monitor mode. I then ran: 
```
tshark -i wlan0 -w drone_capture.pcapng
```
While this was running, I connected to the network and the app on my phone to cause some activity between the devices. I let it run for ~15 seconds and then stopped the process. The files can be read with tshark:
```
tshark -r drone_capture.pcapng
```

This returned a bunch of information, but confirmed a few things.

The original structure of the handshake appears to be similar to the non-Elite S5C variant of the drone. During the inital connection from the phone to the drone, I observed 
    
    
