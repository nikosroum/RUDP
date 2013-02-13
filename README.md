Roumpoutsos Nikolaos 
Sapountzis Ioannis 
========================================================================
README file - Advanced Level
========================================================================
Delivered files:
----------------
README.txt - Instructions how to compile and run the application

Makefile - for building the sample application.

C files:
----------------
-  rudp_api.h 
Declarations for the RUDP API.
-  rudp.h 
Declarations for the RUDP protocol.
-  rudp.c 
Functions for the RUDP protocol.
-  structs.h
Declarations of structs used in RUDP.
-  event.h
Declarations for the event manager.
-  event.c 
The event manager.
-  vsftp.h 
Declarations for the sample application.
-  vs_send.c 
Sender for the sample application.
-  vs_recv.c 
Receiver for the sample application.



========================================================================
Instructions
========================================================================
The project was developped in gcc and it was tested on Ubuntu 10.04,Mac Leopard 10.6.8

– How to compile
Make

– How to run 

./vs_send [-d] host1:port1 [host2:port2 ...] file1 [file2 ...] 

./vs_recv [-d] port 

The optional flag -d turns on debug messages.
