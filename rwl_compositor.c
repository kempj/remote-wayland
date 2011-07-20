#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "wayland-server.h"

#include "connection.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/file.h>
#include <netdb.h>

static char* remote_address;

struct wl_socket {
	int fd;
	int fd_lock;
	struct sockaddr_un addr;
	char lock_addr[113];
	struct wl_list link;
};

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

int rwl_get_remote_connection(char *remote_name)
{
	int err, remote_fd;
	struct addrinfo *remote, *result;
	//printf("%s\n",remote_name);

	if((err = getaddrinfo(remote_name, "35000", NULL, &remote)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(err));
		return EXIT_FAILURE;
	}

	for(result = remote;result != NULL;result = result->ai_next){
		if((remote_fd = socket(result->ai_family, result->ai_socktype,
				result->ai_protocol)) == -1){
			if(result->ai_next = NULL)
				perror("client: socket");
			continue;
		}

		if (connect(remote_fd, result->ai_addr, result->ai_addrlen) == -1) {
			close(remote_fd);
			if(result->ai_next = NULL)
				perror("client: connect");
			continue;
        	}
		
		break;
	}	
	return remote_fd;
}

static int
rwl_forward_init(int fd, uint32_t mask, void *data)
{
	struct wl_display *display = data;
	struct sockaddr_un name;
	socklen_t length;
	int client_fd, remote_fd;

	remote_fd = rwl_get_remote_connection(remote_address);

	length = sizeof name;
	client_fd =
		accept4(fd, (struct sockaddr *) &name, &length, SOCK_CLOEXEC);
	if (client_fd < 0 && errno == ENOSYS) {
		client_fd = accept(fd, (struct sockaddr *) &name, &length);
		if (client_fd >= 0 && fcntl(client_fd, F_SETFD, FD_CLOEXEC) == -1)
			fprintf(stderr, "failed to set FD_CLOEXEC flag on client fd, errno: %d\n", errno);
	}

	if (client_fd < 0)
		fprintf(stderr, "failed to accept, errno: %d\n", errno);

	//add both fds to epoll, create remote connection, add it to epoll
	//wl_closure_print();
	return 1;
}

static int
get_socket_lock(struct wl_socket *socket, socklen_t name_size)
{
	struct stat socket_stat;
	int lock_size = name_size + 5;

	snprintf(socket->lock_addr, lock_size,
		 "%s.lock", socket->addr.sun_path);

	socket->fd_lock = open(socket->lock_addr, O_CREAT | O_CLOEXEC,
			       (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP));

	if (socket->fd_lock < 0) {
		fprintf(stderr,
			"unable to open lockfile %s check permissions\n",
			socket->lock_addr);
		return -1;
	}

	if (flock(socket->fd_lock, LOCK_EX | LOCK_NB) < 0) {
		fprintf(stderr,
			"unable to lock lockfile %s, maybe another compositor is running\n",
			socket->lock_addr);
		close(socket->fd_lock);
		return -1;
	}

	if (stat(socket->addr.sun_path, &socket_stat) < 0 ) {
		if (errno != ENOENT) {
			fprintf(stderr, "did not manage to stat file %s\n",
				socket->addr.sun_path);
			close(socket->fd_lock);
			return -1;
		}
	} else if (socket_stat.st_mode & S_IWUSR ||
		   socket_stat.st_mode & S_IWGRP) {
		unlink(socket->addr.sun_path);
	}

	return 0;
}

int
rwl_display_add_socket(struct wl_display *display, const char *name)
{
	struct wl_socket *s;
	socklen_t size, name_size;
	const char *runtime_dir;

	s = malloc(sizeof *s);
	if (s == NULL)
		return -1;

	s->fd = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (s->fd < 0) {
		free(s);
		return -1;
	}

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

	memset(&s->addr, 0, sizeof s->addr);
	s->addr.sun_family = AF_LOCAL;
	name_size = snprintf(s->addr.sun_path, sizeof s->addr.sun_path,
			     "%s/%s", runtime_dir, name) + 1;
	fprintf(stderr, "using socket %s\n", s->addr.sun_path);

	if (get_socket_lock(s,name_size) < 0) {
		close(s->fd);
		free(s);
		return -1;
	}

	size = offsetof (struct sockaddr_un, sun_path) + name_size;
	if (bind(s->fd, (struct sockaddr *) &s->addr, size) < 0) {
		close(s->fd);
		free(s);
		return -1;
	}

	if (listen(s->fd, 1) < 0) {
		close(s->fd);
		unlink(s->addr.sun_path);
		free(s);
		return -1;
	}

	if (wl_event_loop_add_fd(display->loop, s->fd,
				 WL_EVENT_READABLE,
				 rwl_forward_init, display) == NULL) {
		close(s->fd);
		unlink(s->addr.sun_path);
		free(s);
		return -1;
	}
	wl_list_insert(display->socket_list.prev, &s->link);

	return 0;
}


int
main(int argc, char *argv[])
{
	int i;
	struct wl_display *display = wl_display_create();
	
	if(argc > 1){
		remote_address = argv[2];
	} else{
		remote_address = (char *) malloc(13 * sizeof (char));
		remote_address = &"127.0.0.1\0";
		//printf("assigned %s\n", remote_address);
	}
	
	if (rwl_display_add_socket(display, NULL)) {
		fprintf(stderr, "failed to add socket: %m\n");
		exit(EXIT_FAILURE);
	}

	wl_display_run(display);

}
