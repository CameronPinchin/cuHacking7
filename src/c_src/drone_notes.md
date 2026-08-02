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

This has yielded interesting results so far, having discovered **port 51167** has packets being sent over it. 

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

Further observations from these captures yielded increasingly diminishing returns. Not much usable information could be obtained from it, so I switched to a separate method. From my PC, I created a small program to try to capture the payload by joining the multicast group I identified earlier. 

The drone transmits multicast beacons to: **[239.1.2.255:51167]** every two seconds without any prompting event. The payload is 234 bytes long, so the program just captures this packet and performs a hex dump to the terminal. This produced the following output: 

```
f0 bf 00 00 01 00 01 00 01 00 01 00 02 00 09 00  
00 00 d4 00 00 00 35 30 2d 39 42 2d 39 34 2d 41  
37 2d 44 33 2d 33 45 00 31 37 32 2e 31 39 2e 31  
30 2e 31 00 00 00 00 00 32 35 35 2e 32 35 35 2e  
30 2e 30 00 00 00 00 00 31 37 32 2e 31 39 2e 31  
30 2e 31 00 00 00 00 00 a2 22 30 2e 30 2e 30 20  
28 62 75 69 6c 64 20 30 29 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 30 2e 30 2e 30 20  
28 62 75 69 6c 64 20 30 29 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 46 48 38 38 33 30  
5f 35 30 2d 39 42 2d 39 34 2d 41 37 2d 44 33 2d  
33 45 00 00 00 00 00 00 00 00 35 30 2d 39 42 2d  
39 34 2d 41 37 2d 44 33 2d 33 45 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 
```

#### Packet Breakdown

The first 18 bytes are related to the ethernet protocol and control structures for the packet. Useful, but not super interesting.
```
f0 bf 00 00 01 00 01 00 01 00 01 00 02 00 09 00 00 00 d4 00 00 00
```
The next 17 bytes identify the drone's MAC address: 
```
35 30 2d 39 42 2d 39 34 2d 41 37 2d 44 33 2d 33 45 == 50-9B-94-A7-D3-3E
```
The next 15 bytes identify the drone's IP address:
```
31 37 32 2e 31 39 2e 31 30 2e 31 == 172.19.10.1
```
The next 15 bytes identify the subnet mask:
```
32 35 35 2e 32 35 35 2e 30 2e 30 == 255.255.0.0
```
The next 15 bytes identify the gateway:
```
31 37 32 2e 31 39 2e 31 30 2e 31 == 172.19.10.1
```
The next 2 bytes seem to identify a port, potentially the video port:
```
a2 22 == 0x22A2 == 8866
```
*This is unconfirmed, but lines up perfectly with a little-endian 16-bit unsigned integer.*  
The next 15 bytes identify a build version, highly likely to be the firmware version:
```
30 2e 30 2e 30 20 28 62 75 69 6c 64 20 30 29 == "0.0.0 (build 0)"
```
The final 23 bytes identify hardware information:
```
46 48 38 38 33 30 5f 35 30 2d 39 42 2d 39 34 2d 41 37 2d 44 33 2d 33 45 == "FH8830_50-9B-94-A7-D3-3E"
```

This was a bit of a breakthrough, as this provided two key pieces of information. 

    1. The port 8866 is being used, potentially for video transmission. 
    2. The hardware information revealed the onboard camera SoC, which was unknown before. 
    
### Port 8866

Identifying this port was crucial for the next steps. I ran another program designed to establish a TCP connection to the drone on port 8866, which was successful and proved it was infact open. I then listened for ~3 seconds, and didn't receive any data. This indicates the drone is waiting for a message to proceed rather than emitting anything continually. 
    
### FH8830 SoC for Cameras

The FH8830 SoC is commonly used by cheap, consumer-grade FPV drones and has likely been reverse engineered before. 


