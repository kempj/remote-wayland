all: pserver pclient

pserver: rwl_compositor.c connection.o
	gcc -g rwl_compositor.c connection.o -lwayland-server  -lffi -lrt -o pserver  -I.

pclient: event-loop.o rwl_client.o
	gcc -g -o pclient rwl_client.o event-loop.o connection.o -I. -lwayland-server -lwayland-client -lrt -lffi

event-loop.o: event-loop.c
	gcc $< -g -c -o event-loop.o

rwl_client.o: rwl_client.c 
	gcc $< -g -c -o rwl_client.o -I.

connection.o: connection.c
	gcc $< -g -c -o connection.o
clean:
	rm -rf *.o pserver pclient
