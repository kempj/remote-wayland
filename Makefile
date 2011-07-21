all: pserver pclient

event-loop.o: event-loop.c
	gcc $< -c -o event-loop.o

rwl_client.o: rwl_client.c 
	gcc $< -c -o rwl_client.o -I.

pserver: rwl_compositor.c
	gcc rwl_compositor.c /home/jeremy/wayland/install/lib/libwayland-server.a  /usr/lib/x86_64-linux-gnu/libffi.a -lrt -o pserver 

pclient: event-loop.o rwl_client.o
	gcc -o pclient $^ /home/jeremy/wayland/install/lib/libwayland-client.a  /usr/lib/x86_64-linux-gnu/libffi.a  -o pclient -I. /home/jeremy/wayland/install/lib/libwayland-server.a /usr/lib/x86_64-linux-gnu/libffi.a /usr/lib/x86_64-linux-gnu/librt.a 
