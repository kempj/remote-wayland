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

//start of event loop file
/*
 * Copyright © 2008 Kristian Høgsberg
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
/*
#include <stddef.h>
#include <errno.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
//#include "wayland-server.h"


struct wl_event_loop {
	int epoll_fd;
	struct wl_list check_list;
	struct wl_list idle_list;
};

struct wl_event_source_interface {
	int (*dispatch)(struct wl_event_source *source,
			struct epoll_event *ep);
	int (*remove)(struct wl_event_source *source);
};

struct wl_event_source {
	struct wl_event_source_interface *interface;
	struct wl_event_loop *loop;
	struct wl_list link;
	void *data;
};

struct wl_event_source_fd {
	struct wl_event_source base;
	int fd;
	wl_event_loop_fd_func_t func;
};

static int
wl_event_source_fd_dispatch(struct wl_event_source *source,
			    struct epoll_event *ep)
{
	struct wl_event_source_fd *fd_source = (struct wl_event_source_fd *) source;
	uint32_t mask;

	mask = 0;
	if (ep->events & EPOLLIN)
		mask |= WL_EVENT_READABLE;
	if (ep->events & EPOLLOUT)
		mask |= WL_EVENT_WRITEABLE;

	return fd_source->func(fd_source->fd, mask, fd_source->base.data);
}

static int
wl_event_source_fd_remove(struct wl_event_source *source)
{
	struct wl_event_source_fd *fd_source =
		(struct wl_event_source_fd *) source;
	struct wl_event_loop *loop = source->loop;
	int fd;

	fd = fd_source->fd;
	free(source);

	return epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

struct wl_event_source_interface fd_source_interface = {
	wl_event_source_fd_dispatch,
	wl_event_source_fd_remove
};

WL_EXPORT struct wl_event_source *
wl_event_loop_add_fd(struct wl_event_loop *loop,
		     int fd, uint32_t mask,
		     wl_event_loop_fd_func_t func,
		     void *data)
{
	struct wl_event_source_fd *source;
	struct epoll_event ep;

	source = malloc(sizeof *source);
	if (source == NULL)
		return NULL;

	source->base.interface = &fd_source_interface;
	source->base.loop = loop;
	wl_list_init(&source->base.link);
	source->fd = fd;
	source->func = func;
	source->base.data = data;

	memset(&ep, 0, sizeof ep);
	if (mask & WL_EVENT_READABLE)
		ep.events |= EPOLLIN;
	if (mask & WL_EVENT_WRITEABLE)
		ep.events |= EPOLLOUT;
	ep.data.ptr = source;

	if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &ep) < 0) {
		free(source);
		return NULL;
	}

	return &source->base;
}

WL_EXPORT int
wl_event_source_fd_update(struct wl_event_source *source, uint32_t mask)
{
	struct wl_event_source_fd *fd_source =
		(struct wl_event_source_fd *) source;
	struct wl_event_loop *loop = source->loop;
	struct epoll_event ep;

	memset(&ep, 0, sizeof ep);
	if (mask & WL_EVENT_READABLE)
		ep.events |= EPOLLIN;
	if (mask & WL_EVENT_WRITEABLE)
		ep.events |= EPOLLOUT;
	ep.data.ptr = source;

	return epoll_ctl(loop->epoll_fd,
			 EPOLL_CTL_MOD, fd_source->fd, &ep);
}

struct wl_event_source_timer {
	struct wl_event_source base;
	int fd;
	wl_event_loop_timer_func_t func;
};

static int
wl_event_source_timer_dispatch(struct wl_event_source *source,
			       struct epoll_event *ep)
{
	struct wl_event_source_timer *timer_source =
		(struct wl_event_source_timer *) source;
	uint64_t expires;

	read(timer_source->fd, &expires, sizeof expires);

	return timer_source->func(timer_source->base.data);
}

static int
wl_event_source_timer_remove(struct wl_event_source *source)
{
	struct wl_event_source_timer *timer_source =
		(struct wl_event_source_timer *) source;

	close(timer_source->fd);
	free(source);
	return 0;
}

struct wl_event_source_interface timer_source_interface = {
	wl_event_source_timer_dispatch,
	wl_event_source_timer_remove
};

WL_EXPORT struct wl_event_source *
wl_event_loop_add_timer(struct wl_event_loop *loop,
			wl_event_loop_timer_func_t func,
			void *data)
{
	struct wl_event_source_timer *source;
	struct epoll_event ep;

	source = malloc(sizeof *source);
	if (source == NULL)
		return NULL;

	source->base.interface = &timer_source_interface;
	source->base.loop = loop;
	wl_list_init(&source->base.link);

	source->fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
	if (source->fd < 0) {
		fprintf(stderr, "could not create timerfd\n: %m");
		free(source);
		return NULL;
	}

	source->func = func;
	source->base.data = data;

	memset(&ep, 0, sizeof ep);
	ep.events = EPOLLIN;
	ep.data.ptr = source;

	if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, source->fd, &ep) < 0) {
		close(source->fd);
		free(source);
		return NULL;
	}

	return &source->base;
}

WL_EXPORT int
wl_event_source_timer_update(struct wl_event_source *source, int ms_delay)
{
	struct wl_event_source_timer *timer_source =
		(struct wl_event_source_timer *) source;
	struct itimerspec its;

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = ms_delay / 1000;
	its.it_value.tv_nsec = (ms_delay % 1000) * 1000 * 1000;
	if (timerfd_settime(timer_source->fd, 0, &its, NULL) < 0) {
		fprintf(stderr, "could not set timerfd\n: %m");
		return -1;
	}

	return 0;
}

struct wl_event_source_signal {
	struct wl_event_source base;
	int fd;
	int signal_number;
	wl_event_loop_signal_func_t func;
};

static int
wl_event_source_signal_dispatch(struct wl_event_source *source,
			       struct epoll_event *ep)
{
	struct wl_event_source_signal *signal_source =
		(struct wl_event_source_signal *) source;
	struct signalfd_siginfo signal_info;

	read(signal_source->fd, &signal_info, sizeof signal_info);

	return signal_source->func(signal_source->signal_number,
				   signal_source->base.data);
}

static int
wl_event_source_signal_remove(struct wl_event_source *source)
{
	struct wl_event_source_signal *signal_source =
		(struct wl_event_source_signal *) source;

	close(signal_source->fd);
	free(source);
	return 0;
}

struct wl_event_source_interface signal_source_interface = {
	wl_event_source_signal_dispatch,
	wl_event_source_signal_remove
};

WL_EXPORT struct wl_event_source *
wl_event_loop_add_signal(struct wl_event_loop *loop,
			int signal_number,
			wl_event_loop_signal_func_t func,
			void *data)
{
	struct wl_event_source_signal *source;
	struct epoll_event ep;
	sigset_t mask;

	source = malloc(sizeof *source);
	if (source == NULL)
		return NULL;

	source->base.interface = &signal_source_interface;
	source->base.loop = loop;
	wl_list_init(&source->base.link);
	source->signal_number = signal_number;

	sigemptyset(&mask);
	sigaddset(&mask, signal_number);
	source->fd = signalfd(-1, &mask, SFD_CLOEXEC);
	if (source->fd < 0) {
		fprintf(stderr, "could not create fd to watch signal\n: %m");
		free(source);
		return NULL;
	}
	sigprocmask(SIG_BLOCK, &mask, NULL);

	source->func = func;
	source->base.data = data;

	memset(&ep, 0, sizeof ep);
	ep.events = EPOLLIN;
	ep.data.ptr = source;

	if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, source->fd, &ep) < 0) {
		close(source->fd);
		free(source);
		return NULL;
	}

	return &source->base;
}

struct wl_event_source_idle {
	struct wl_event_source base;
	wl_event_loop_idle_func_t func;
};

static int
wl_event_source_idle_remove(struct wl_event_source *source)
{
	free(source);

	return 0;
}

struct wl_event_source_interface idle_source_interface = {
	NULL,
	wl_event_source_idle_remove
};

WL_EXPORT struct wl_event_source *
wl_event_loop_add_idle(struct wl_event_loop *loop,
		       wl_event_loop_idle_func_t func,
		       void *data)
{
	struct wl_event_source_idle *source;

	source = malloc(sizeof *source);
	if (source == NULL)
		return NULL;

	source->base.interface = &idle_source_interface;
	source->base.loop = loop;

	source->func = func;
	source->base.data = data;

	wl_list_insert(loop->idle_list.prev, &source->base.link);

	return &source->base;
}

WL_EXPORT void
wl_event_source_check(struct wl_event_source *source)
{
	wl_list_insert(source->loop->check_list.prev, &source->link);
}

WL_EXPORT int
wl_event_source_remove(struct wl_event_source *source)
{
	if (!wl_list_empty(&source->link))
		wl_list_remove(&source->link);

	source->interface->remove(source);

	return 0;
}

WL_EXPORT struct wl_event_loop *
wl_event_loop_create(void)
{
	struct wl_event_loop *loop;

	loop = malloc(sizeof *loop);
	if (loop == NULL)
		return NULL;

	loop->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (loop->epoll_fd < 0) {
		free(loop);
		return NULL;
	}
	wl_list_init(&loop->check_list);
	wl_list_init(&loop->idle_list);

	return loop;
}

WL_EXPORT void
wl_event_loop_destroy(struct wl_event_loop *loop)
{
	close(loop->epoll_fd);
	free(loop);
}

static int
post_dispatch_check(struct wl_event_loop *loop)
{
	struct epoll_event ep;
	struct wl_event_source *source, *next;
	int n;

	ep.events = 0;
	n = 0;
	wl_list_for_each_safe(source, next, &loop->check_list, link)
		n += source->interface->dispatch(source, &ep);

	return n;
}

static void
dispatch_idle_sources(struct wl_event_loop *loop)
{
	struct wl_event_source_idle *source, *next;

	wl_list_for_each_safe(source, next, &loop->idle_list, base.link) {
		source->func(source->base.data);
		wl_event_source_remove(&source->base);
	}
}

WL_EXPORT int
wl_event_loop_dispatch(struct wl_event_loop *loop, int timeout)
{
	struct epoll_event ep[32];
	struct wl_event_source *source;
	int i, count, n;

	dispatch_idle_sources(loop);

	count = epoll_wait(loop->epoll_fd, ep, ARRAY_LENGTH(ep), timeout);
	if (count < 0)
		return -1;
	n = 0;
	for (i = 0; i < count; i++) {
		source = ep[i].data.ptr;
		n += source->interface->dispatch(source, &ep[i]);
	}

	while (n > 0)
		n = post_dispatch_check(loop);
		
	return 0;
}

WL_EXPORT int
wl_event_loop_get_fd(struct wl_event_loop *loop)
{
	return loop->epoll_fd;
}*/
//end of event loop file
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

struct wl_display {
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

struct wl_event_loop {
	int epoll_fd;
	struct wl_list check_list;
	struct wl_list idle_list;
};

int
main(int argc, char **argv)
{
	struct wl_display *display;
	struct window *window;
	int fd, err;
	struct addrinfo *local_address_info;
	struct sockaddr_storage incoming_addr;
	socklen_t size;

	display = rwl_display_create();//create_display();
	//window = create_window(display, 250, 250);

	//redraw(window->surface, window, 0);

	if((err = getaddrinfo(NULL, "35000", NULL, &local_address_info)) != 0) {
		fprintf(stderr, "pclient:getaddrinfo error: %s\n", gai_strerror(err));
		return EXIT_FAILURE;
	}
	fd = socket(local_address_info->ai_family, 
			local_address_info->ai_socktype,
			local_address_info->ai_protocol);
	bind(fd,local_address_info->ai_addr,local_address_info->ai_addrlen);
	listen(fd,8);
	int remote_fd = accept(fd,(struct sockaddr *) &incoming_addr, size);

/*	wl_event_loop_add_fd(display->loop,
		     remote fd, WL_EVENT_READABLE,
		     wl_event_loop_fd_func_t func,
		     void *data);*/

	//make display
	//make a new struct/function to be called
	//add to event loop
	while (true)
		//wl_display_iterate(display->display, display->mask);
		rwl_display_run(display);

	return 0;
}
