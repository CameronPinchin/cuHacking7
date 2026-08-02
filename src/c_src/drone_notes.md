# Snaptain Elite S5C Drone
*Cameron Pinchin*

The Snaptain Elite S5C is a consumer drone capable of operating at 80 - 100 meters (likely in perfect conditions), approximately ~10-15 minutes.

## Drone - Technical Notes

Establishing a programmatic connection to the drone has been the most challenging portion. My assumption was that the video is based on a UDP connection and just continually transmits datagrams on a specific port.

The Snaptain S5C base model has been reverse engineered and the communication protocol is known. The process is as follows: 
    
    1. The TCP component involves establishing four separate TCP connections to the drone over **port 8888** with a specific string attached.
    
    2. This TCP handshake is accepted which opens up the UDP stream for connections on **port 9125** . 
    
I tried replicating this on the **Snaptain Elite S5C** which didn't end up working. It appears that the TCP connections on port 8888 are not even open to begin with, leading me to believe they are on a different port or the process differs entirely from the non-Elite model. 

## Reverse Engineering the Snaptain Elite S5C

The plan is to use my phone to connect to the drone using the Snaptain FPV app. I would then use my PC to sniff the packets being sent back and forth to; identify the ports, and identify potential patterns in the communication.

This has yielded interesting results so far, having discovered **port 51167** as b

I have been using *tshark*, a cli-based version of WireShark to capture information about the transmissions between my phone and the drone. 

The drone seemingly struggles with multiple connections at once, so I switched the wifi network on my PC to monitor mode, and set it to listen on channel 2 for activity. I then used tshark:  
```
tshark -i wlan0 -w drone_capture.pcapng
```
I let this run for ~15 seconds. While it was running, I established a connection to the drones hotspot, waited a couple seconds, and then opened the **SNAPTAIN FPV** app 

This enabled me to view to separate processes: (1) the initial connection of a phone to the drones network, and (2) the network activity when the **SNAPTAIN FPV** app is opened and the live feed is established. 

For (1), the main question is: does any visible handshake occur between the drones network and the phone?   
For (2), the main question is: does a secondary handshake occur that is required for video data to be transmitted?  

### Overview of Network Activitiy

The capture created by *tshark* immediately revealed interesting information and confirmed that I am observing activity between my phone and the drones network. There was a considerable spike in the number of frames captured as soon as my phone connected, with the number of packets being sent numbering around ~100 per second. 

The second notable spike in activity was after I opened the app and established a live video feed. This showed a sharp rise in packets being transmitted per second, peaking around ~500 packets per second. For reference, you can see a graphical representation of this activity below: 

#### Network Activity: Drone-to-Phone Connection
![Network Activity: Drone-to-Phone Connection](https://i.imgur.com/RC8Ot0n.png)

