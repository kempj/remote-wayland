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

#include "event-loop.h"
//#include "wayland-util.h"

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

/*struct wl_display {
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
};*/


/*struct wl_event_loop {
	int epoll_fd;
	struct wl_list check_list;
	struct wl_list idle_list;
};*/

struct rwl_connection {
	struct wl_connection *connection;
	int fd_to, fd_from;
	struct wl_server_display *display;
};

struct rwl_remote_display {
	int fd;
	struct wl_server_display *display;
};

static int
rwl_forward(int fd, uint32_t mask, void *data)
{
	struct rwl_connection *connection = data;

	//read in from fd_from
	//write to fd_to
	return 1;
}

int
rwl_create_forward(int remote_fd, struct wl_server_display *main_display)
{
	struct wl_display *client_display = wl_display_connect(NULL);	
	int local_fd = wl_display_get_fd(client_display,NULL,NULL);
	struct rwl_connection *connection_in, *connection_out;

	connection_in = malloc(sizeof connection_in);
	connection_in->fd_from = remote_fd;
	connection_in->fd_to = local_fd;
	connection_in->connection = malloc(sizeof connection_in->connection);
	//connection_in->display = 

	connection_out = malloc(sizeof connection_out);
	connection_out->fd_from = local_fd;
	connection_out->fd_to = remote_fd;
	connection_out->connection = malloc(sizeof connection_out->connection);

	int local_fd = wl_display_get_fd(client_display,NULL,NULL);

	wl_event_loop_add_fd(main_display->loop,
		     remote_fd, WL_EVENT_READABLE,
		     rwl_forward,
		     connection_in);

	wl_event_loop_add_fd(display->loop,
		     fd, WL_EVENT_READABLE,
		     rwl_forward,
		     connection_out);
}

static int 
rwl_remote_connection(int fd, uint32_t mask, void *data)
{
	struct rwl_remote_display *display = data;
	struct sockaddr_storage incoming_addr;
	socklen_t size;

	int remote_fd = accept(display->fd,(struct sockaddr *) &incoming_addr, size);

	rwl_create_forward(remote_fd, display->display);

	return 1;
}

int
main(int argc, char **argv)
{
	struct wl_server_display *display;
	struct rwl_remote_display *remote_display;
	struct window *window;
	int fd, err;
	struct addrinfo *local_address_info;
	
	display = rwl_display_create();

	if((err = getaddrinfo(NULL, "35000", NULL, &local_address_info)) != 0) {
		fprintf(stderr, "pclient:getaddrinfo error: %s\n", gai_strerror(err));
		return EXIT_FAILURE;
	}
	fd = socket(local_address_info->ai_family, 
			local_address_info->ai_socktype,
			local_address_info->ai_protocol);
	bind(fd,local_address_info->ai_addr,local_address_info->ai_addrlen);
	listen(fd,8);

	remote_display = malloc(sizeof remote_display);
	remote_display->display = display;
	remote_display->fd = fd;

	wl_event_loop_add_fd(display->loop,
		     fd, WL_EVENT_READABLE,
		     rwl_remote_connection,
		     remote_display);

	while (true)
		rwl_display_run(display);

	return 0;
}
