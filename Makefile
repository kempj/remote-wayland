all: pserver pclient

event-loop.o: event-loop.c
	gcc $< -c -o event-loop.o

rwl_client.o: rwl_client.c 
	gcc $< -c -o rwl_client.o -I.

pserver: rwl_compositor.c
	gcc rwl_compositor.c -lwayland-server  -lffi -lrt -o pserver 

pclient: event-loop.o rwl_client.o
	gcc -o pclient event-loop.o rwl_client.o -I. -lwayland-server -lwayland-client -lrt -lffi
