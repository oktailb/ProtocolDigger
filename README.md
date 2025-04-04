# Protocol Digger
 Reverse engineering toolbox for fixed size protocol messages.
 
 Allows to switch from binary manual sinking
![Wireshark screenshot](ws.png)
to visual physical values interpretation
![Sinvi screenshot](Sinvi.png)

With the possibility to:
 - Change data interpretation type
 - Set data size (8, 16, 32, 64) or custom for strings
 - Set mask, shift and conversion ratio
 - Use bookmarks from/to configuration file
 - specify endianity 
 - Compare different curves over the time
 - Perform an automated search based on simple criterias such as values diversity, min, max, amplitude
 ![Sinvi toolbar screenshot](SinviToolbar.png)

The input can be
 - a pcap file
 - a network spy (such as wireshark, including filter)
 - a socket based udp connexion
 
 ```ini
 [Input]
mode=file
relayPcap=false
otherClient=true
address=127.0.0.1:3033
device=eth0
file=host_to_stealthview_flight.pcap
filter=src host 192.168.4.3 and udp port 3033
```
