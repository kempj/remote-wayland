struct wl_display;

enum {
	WL_EVENT_READABLE = 0x01,
	WL_EVENT_WRITEABLE = 0x02
};

typedef int (*wl_event_loop_fd_func_t)(int fd, uint32_t mask, void *data);

//event loop functions
wl_event_loop_add_fd(struct wl_event_loop *loop,
		     int fd, uint32_t mask,
		     wl_event_loop_fd_func_t func,
		     void *data);
		     
int wl_event_source_fd_update(struct wl_event_source *source, uint32_t mask);

//wayland-server functions
void rwl_display_run(struct wl_display *display);
struct wl_display * rwl_display_create(void);

