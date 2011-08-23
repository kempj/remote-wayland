/*
 * Copyright © 2011 Benjamin Franzke
 * Copyright © 2010 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <wayland-client.h>
#include <wayland-egl.h>

#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include "rwl_client.h"
#include "wayland-util.h"
#include "connection.h"
#include <sys/un.h>

/*
struct display {
	struct wl_display *display;
	struct wl_visual *xrgb_visual;
	struct wl_compositor *compositor;
	struct wl_shell *shell;
	struct wl_shm *shm;
	uint32_t mask;
};

struct window {
	struct display *display;
	int width, height;
	struct wl_surface *surface;
	struct wl_buffer *buffer;
	void *data;
};

static struct wl_buffer *
create_shm_buffer(struct display *display,
		  int width, int height, struct wl_visual *visual,
		  void **data_out)
{
	char filename[] = "/tmp/wayland-shm-XXXXXX";
	struct wl_buffer *buffer;
	int fd, size, stride;
	void *data;

	fd = mkstemp(filename);
	if (fd < 0) {
		fprintf(stderr, "open %s failed: %m\n", filename);
		return NULL;
	}
	stride = width * 4;
	size = stride * height;
	if (ftruncate(fd, size) < 0) {
		fprintf(stderr, "ftruncate failed: %m\n");
		close(fd);
		return NULL;
	}

	data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	unlink(filename);

	if (data == MAP_FAILED) {
		fprintf(stderr, "mmap failed: %m\n");
		close(fd);
		return NULL;
	}

	buffer = wl_shm_create_buffer(display->shm, fd,
				      width, height, stride, visual);

	close(fd);

	*data_out = data;

	return buffer;
}

static struct window *
create_window(struct display *display, int width, int height)
{
	struct window *window;
	struct wl_visual *visual;
	
	window = malloc(sizeof *window);
	window->display = display;
	window->width = width;
	window->height = height;
	window->surface = wl_compositor_create_surface(display->compositor);
	visual = display->xrgb_visual;
	window->buffer = create_shm_buffer(display,
					   width, height,
					   visual, &window->data);

	wl_shell_set_toplevel(display->shell, window->surface);

	return window;
}

static void
redraw(struct wl_surface *surface, void *data, uint32_t time)
{
	struct window *window = data;
	uint32_t *p;
	int i, end, offset;

	p = window->data;
	end = window->width * window->height;
	offset = time >> 4;
	for (i = 0; i < end; i++)
		p[i] = (i + offset) * 0x0080401;
	wl_buffer_damage(window->buffer, 0, 0, window->width, window->height);

	wl_surface_attach(window->surface, window->buffer, 0, 0);
	wl_surface_damage(window->surface,
			  0, 0, window->width, window->height);

	wl_display_frame_callback(window->display->display,
				  window->surface,
				  redraw, window);
}

static void
compositor_handle_visual(void *data,
			 struct wl_compositor *compositor,
			 uint32_t id, uint32_t token)
{
	struct display *d = data;

	switch (token) {
	case WL_COMPOSITOR_VISUAL_XRGB32:
		d->xrgb_visual = wl_visual_create(d->display, id, 1);
		break;
	}
}

static const struct wl_compositor_listener compositor_listener = {
	compositor_handle_visual,
};

static void
display_handle_global(struct wl_display *display, uint32_t id,
		      const char *interface, uint32_t version, void *data)
{
	struct display *d = data;

	if (strcmp(interface, "wl_compositor") == 0) {
		d->compositor = wl_compositor_create(display, id, 1);
		wl_compositor_add_listener(d->compositor,
					   &compositor_listener, d);
	} else if (strcmp(interface, "wl_shell") == 0) {
		d->shell = wl_shell_create(display, id, 1);
	} else if (strcmp(interface, "wl_shm") == 0) {
		d->shm = wl_shm_create(display, id, 1);
	}
}

static int
event_mask_update(uint32_t mask, void *data)
{
	struct display *d = data;

	d->mask = mask;

	return 0;
}

static void
sync_callback(void *data)
{
	int *done = data;

	*done = 1;
}

static struct display *
create_display(void)
{
	struct display *display;
	int done;

	display = malloc(sizeof *display);
	display->display = wl_display_connect(NULL);

	wl_display_add_global_listener(display->display,
				       display_handle_global, display);
	wl_display_iterate(display->display, WL_DISPLAY_READABLE);

	wl_display_get_fd(display->display, event_mask_update, display);
	
	wl_display_sync_callback(display->display, sync_callback, &done);
	while (!display->xrgb_visual)
		wl_display_iterate(display->display, display->mask);

	return display;
}*/
/*
struct rwl_forward {
	int fd;
	
}

static int
rwl_remote_init(int fd)
{
	//
}*/

struct wl_event_loop {
        int epoll_fd;
        struct wl_list check_list;
        struct wl_list idle_list;
};


struct wl_server_display {
	struct wl_object object;
	struct wl_event_loop *loop;
	struct wl_hash_table *objects;
	int run;

	struct wl_list frame_list;
	uint32_t client_id_range;
	uint32_t id;

	struct wl_list global_list;
	struct wl_list socket_list;
	struct wl_list client_list;
};

struct wl_proxy {
	struct wl_object object;
	struct wl_display *display;
	void *user_data;
};

struct wl_display {
	struct wl_proxy proxy;
	struct wl_connection *connection;
	int fd;
	uint32_t id, id_count, next_range;
	uint32_t mask;
	struct wl_hash_table *objects;
	struct wl_list global_listener_list;
	struct wl_list global_list;

	wl_display_update_func_t update;
	void *update_data;

	wl_display_global_func_t global_handler;
	void *global_handler_data;

	struct wl_list sync_list, frame_list;
	uint32_t key;
};

/*struct wl_event_loop {
	int epoll_fd;
	struct wl_list check_list;
	struct wl_list idle_list;
};*/


struct wl_event_source {
	struct wl_event_source_interface *interface;
	struct wl_event_loop *loop;
	struct wl_list link;
	void *data;
};

struct rwl_bundle {
	struct wl_server_display *server_display;
	struct wl_display *client_display;
	struct wl_connection *connection_in;
	struct wl_connection *connection_out;
};

static int
rwl_forward_data(int fd, uint32_t mask, void *data)
{
	printf("Entering wrl_forward_data()\n");
	struct rwl_bundle *rwl_data = data;
	
	struct wl_connection *connection_in = rwl_data->connection_in;
	struct wl_connection *connection_out = rwl_data->connection_out;
	struct wl_server_display *display = rwl_data->server_display;

	struct wl_object *object;
	struct wl_closure *closure;
	const struct wl_message *message;
	uint32_t p[2], opcode, size;
	uint32_t cmask = 0;
	int len;

	if (mask & WL_EVENT_READABLE)
		cmask |= WL_CONNECTION_READABLE;
	if (mask & WL_EVENT_WRITEABLE)
		cmask |= WL_CONNECTION_WRITABLE;

	len = wl_connection_data(connection_in, cmask);
	if(len < 0) {
		return 1;
	}
	
	while (len >= sizeof p) {
		wl_connection_copy(connection_in, p, sizeof p);
		opcode = p[1] & 0xffff;
		size = p[1] >> 16;
		if (len < size)
			break;

		if(opcode >= wl_display_interface.method_count){
			fprintf(stderr,"opcode out of range\n");
			return -1;
		}

                message = &wl_display_interface.methods[opcode];
                closure = wl_connection_demarshal(connection_in, size,
                                                  display->objects,
                                                  message);
                len -= size;

                if (closure == NULL && errno == EINVAL) {
                        continue;
                } else if (closure == NULL && errno == ENOMEM) {
                        continue;
                }

	//	wl_closure_send(closure, client_out->connection);		

		wl_closure_print(closure, object, 1);

		wl_closure_destroy(closure);
	}


	return 1;
}

/*
 *I don't think this will work here, I need to use the server version.
static int
connection_update(struct wl_connection *connection,                                 
                  uint32_t mask, void *data)                                        
{
//from wayland-client.c
        struct wl_display *display = data;                                          

        display->mask = mask;                                                       
        if (display->update)                                                        
                return display->update(display->mask,                               
                                       display->update_data);                       

        return 0;
}*/


static int
rwl_client_connection_update(struct wl_connection *connection,
			    uint32_t mask, void *data)
{
	struct wl_event_source *source = data;
	uint32_t emask = 0;

	if (mask & WL_CONNECTION_READABLE)
		emask |= WL_EVENT_READABLE;
	if (mask & WL_CONNECTION_WRITABLE)
		emask |= WL_EVENT_WRITEABLE;

	return wl_event_source_fd_update(source, emask);
}


int
rwl_create_forward(struct wl_display *client_display,
		   int remote_fd, 
		   struct wl_server_display *main_display)
{
	printf("creating forwards, main loop fd = %d\n", main_display->loop->epoll_fd);
	//Might not need all of one of the displays
	//only need the loop in this function, later
	//each of the connections will need a hash table

	struct wl_event_source *source;
	struct rwl_bundle *incoming_fwd;
	struct rwl_bundle *outgoing_fwd;
	struct wl_connection *remote_connection;

//	source = malloc(sizeof *source);
//	if(source == NULL)
//		return -1;

	incoming_fwd = malloc(sizeof *incoming_fwd);
	if(incoming_fwd == NULL)
	{
		fprintf(stderr," failed to make the incoming fwd\n");
		return -1;
	}

	outgoing_fwd = malloc(sizeof *outgoing_fwd);
	if(outgoing_fwd == NULL)
	{
		fprintf(stderr," failed to make the outgoing fws\n");
		return -1;
	}
	incoming_fwd->server_display = main_display;
	incoming_fwd->client_display = client_display;
	incoming_fwd->connection_in = remote_connection;
	incoming_fwd->connection_out = client_display->connection; 

	outgoing_fwd->server_display = main_display;
	outgoing_fwd->client_display = client_display;
	outgoing_fwd->connection_in = client_display->connection;
	outgoing_fwd->connection_out = remote_connection; 
	

	source = wl_event_loop_add_fd(main_display->loop,
		     remote_fd, WL_EVENT_READABLE,
		     rwl_forward_data,
		     incoming_fwd);

	if(source == NULL)
	{
		fprintf(stderr,"failed to add fd to main loop\n");
		return -1;
	}
	wl_event_loop_add_fd(main_display->loop,
		     client_display->fd,
		     WL_EVENT_READABLE,
		     rwl_forward_data,
		     outgoing_fwd);
	
	source->loop = main_display->loop;
	remote_connection = wl_connection_create(remote_fd,
						 rwl_client_connection_update,
						 source);

	return 1;
}

static int
connect_to_socket(struct wl_display *display, const char *name)
{
        struct sockaddr_un addr;
        socklen_t size;
        const char *runtime_dir;
        size_t name_size;

        display->fd = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (display->fd < 0)
                return -1;

        runtime_dir = getenv("XDG_RUNTIME_DIR");
        if (runtime_dir == NULL) {
                runtime_dir = ".";
                fprintf(stderr,
                        "XDG_RUNTIME_DIR not set, falling back to %s\n",
                        runtime_dir);
        }

        if (name == NULL)
                name = getenv("WAYLAND_DISPLAY");
        if (name == NULL)
                name = "wayland-0";

        memset(&addr, 0, sizeof addr);
        addr.sun_family = AF_LOCAL;
        name_size =
                snprintf(addr.sun_path, sizeof addr.sun_path,
                         "%s/%s", runtime_dir, name) + 1;

        size = offsetof (struct sockaddr_un, sun_path) + name_size;

        if (connect(display->fd, (struct sockaddr *) &addr, size) < 0) {
                close(display->fd);
                return -1;
        }

        return 0;
}

/*
struct wl_display *
rwl_local_display_connect(const char *name, struct wl_server_display *server_display)
{
	//I need all of this because to prevent the display_bind call
	//and to change the function in the connection update
	struct wl_display *display;
	const char *debug;
	char *connection, *end;
	int flags;

	display = malloc(sizeof *display);
	if (display == NULL)
		return NULL;

	memset(display, 0, sizeof *display);
	connection = getenv("WAYLAND_SOCKET");
	if (connection) {
		display->fd = strtol(connection, &end, 0);
		if (*end != '\0') {
			free(display);
			return NULL;
		}
		flags = fcntl(display->fd, F_GETFD);
		if (flags != -1)
			fcntl(display->fd, F_SETFD, flags | FD_CLOEXEC);
	} else if (connect_to_socket(display, name) < 0) {
		free(display);
		return NULL;
	}

	display->proxy.object.interface = &wl_display_interface;
        display->proxy.object.id = 1;
        display->proxy.display = display;


	display->proxy.object.implementation =
                (void(**)(void)) &display_listener;
        display->proxy.user_data = display;


	struct wl_event_source *source = malloc(sizeof *source);
	source->loop = server_display->loop;

	display->connection = wl_connection_create(display->fd,
						   rwl_client_connection_update,
						   source);
	if (display->connection == NULL) {
		wl_hash_table_destroy(display->objects);
		close(display->fd);
		free(display);
		return NULL;
	}

	wl_display_bind(display, 1, "wl_display", 1);

	return display;
}*/

static int 
rwl_remote_connection(int fd, uint32_t mask, void *data)
{
	printf("Initial connection, fd = %d\n",fd);
	struct wl_server_display *display = data;
	struct wl_display *client_display;
	struct sockaddr_storage incoming_addr;
	socklen_t size;
	int remote_fd;

	printf("1main loop epoll = %d\n", display->loop->epoll_fd);


	if((remote_fd = accept(fd,(struct sockaddr *) &incoming_addr, size)) == -1){
		fprintf(stderr, "remote connection failed\n");
		return -1;
	}
	
	printf("2main loop epoll = %d\n", display->loop->epoll_fd);
	client_display = wl_display_connect(NULL);
	printf("3main loop epoll = %d, initial fd = %d, new fd = %d\n", display->loop->epoll_fd, fd, remote_fd);

	if(rwl_create_forward(client_display, remote_fd, display) == -1){
		fprintf(stderr,"failed to create forwards\n");
		printf("4main loop epoll = %d\n", display->loop->epoll_fd);

		return -1;
	}

	return 1;
}

int
get_listening_fd(char *port)
{
	int fd, err;
	struct addrinfo *local_address_info;

	if(port == NULL)
		port = (char *)&"35000\0";
	
	if((err = getaddrinfo(NULL, port , NULL, &local_address_info)) != 0) {
		fprintf(stderr, "pclient:getaddrinfo error: %s\n", gai_strerror(err));
		return EXIT_FAILURE;
	}
	
	if((fd = socket(local_address_info->ai_family, 
			local_address_info->ai_socktype,
			local_address_info->ai_protocol)) == -1){
		fprintf(stderr, "pclient: socket failed\n");
		return EXIT_FAILURE;
	}
	if((bind(fd,local_address_info->ai_addr,
		 local_address_info->ai_addrlen)) == -1)
	{
		fprintf(stderr, "pclient: bind failed\n");
		return EXIT_FAILURE;
	}

	if(listen(fd,8) == -1)
	{
		fprintf(stderr, "pclient:listen failed\n");
		return EXIT_FAILURE;
	}
	printf("fd = %d\n",fd);

	return fd;
}

int
main(int argc, char **argv)
{
	struct wl_server_display *display;
	struct window *window;
	int fd;
	printf("beginning\n");	
	display = rwl_display_create();

	fd = get_listening_fd(NULL);
	printf("fd = %d, main loop fd = %d\n",fd, display->loop->epoll_fd);

	wl_event_loop_add_fd(display->loop,
		     fd, WL_EVENT_READABLE,
		     rwl_remote_connection,
		     display);

	while (true)
		rwl_display_run(display);
	
	return 0;
}
