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

char* remote_address;

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

struct wl_client {
	struct wl_connection *connection;
	struct wl_event_source *source;
	struct wl_display *display;
	struct wl_list resource_list;
	uint32_t id_count;
	uint32_t mask;
	struct wl_list link;
};

struct rwl_client_pair {
	struct wl_client *client_in;
	struct wl_client *client_out;
};

static int
wl_client_connection_update(struct wl_connection *connection,
			    uint32_t mask, void *data)
{
	struct wl_client *client = data;
	uint32_t emask = 0;

	client->mask = mask;
	if (mask & WL_CONNECTION_READABLE)
		emask |= WL_EVENT_READABLE;
	if (mask & WL_CONNECTION_WRITABLE)
		emask |= WL_EVENT_WRITEABLE;

	return wl_event_source_fd_update(client->source, emask);
}


int
rwl_get_remote_connection(char *remote_name)
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
rwl_client_connection_data(int fd, uint32_t mask, void *data)
{
	struct rwl_client_pair *client_pair = data;
	struct wl_client *client_in, *client_out;
	client_in = client_pair->client_in;
	client_out = client_pair->client_out;
	
	struct wl_connection *connection_in, *connection_out;
	connection_in = client_in->connection;
	connection_out = client_out->connection;

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
		wl_client_destroy(client_in);
		//wl_client_destroy(client_out);
		return 1;
	}
	
	while (len >= sizeof p) {
		wl_connection_copy(connection_in, p, sizeof p);
		opcode = p[1] & 0xffff;
		size = p[1] >> 16;
		if (len < size)
			break;

		/*object = wl_hash_table_lookup(client_in->display->objects, p[0]);
		if (object == NULL) {
			wl_client_post_error(client_in, &client_in->display->object,
					     WL_DISPLAY_ERROR_INVALID_OBJECT,
					     "invalid object %d", p[0]);
			wl_connection_consume(connection_in, size);
			len -= size;
			continue;
		}

		if (opcode >= object->interface->method_count) {
			wl_client_post_error(client_in,&client_in->display->object,
                                             WL_DISPLAY_ERROR_INVALID_METHOD,
                                             "invalid method %d, object %s@%d",
                                             object->interface->name,
                                             object->id, opcode);
                        wl_connection_consume(connection_in, size);
                        len -= size;
                        continue;
                }*/

                message = &wl_display_interface.methods[opcode];
			//&object->interface->methods[opcode];
                closure = wl_connection_demarshal(client_in->connection, size,
                                                  client_in->display->objects,
                                                  message);
                len -= size;

                if (closure == NULL && errno == EINVAL) {
                        wl_client_post_error(client_in, &client_in->display->object,
                                             WL_DISPLAY_ERROR_INVALID_METHOD,
                                             "invalid arguments for %s@%d.%s",
                                             object->interface->name,
                                             object->id, message->name);
                        continue;
                } else if (closure == NULL && errno == ENOMEM) {
                        wl_client_post_no_memory(client_in);
                        continue;
                }

		wl_closure_send(closure, client_out->connection);		

		wl_closure_print(closure, object, 1);

		wl_closure_destroy(closure);
	} 

	return 1;
}

static int
rwl_client_pair_create(struct wl_display *display, int fd_local, int fd_remote)
{
	struct wl_client *local_client;
	struct wl_client *remote_client;
	
	struct rwl_client_pair *incoming_fwd;
	struct rwl_client_pair *outgoing_fwd;
	
	incoming_fwd = malloc(sizeof *incoming_fwd);
	if (incoming_fwd == NULL)
		return -1;
	outgoing_fwd = malloc(sizeof *outgoing_fwd);
	if (outgoing_fwd == NULL)
		return -1;

	
	outgoing_fwd->client_in = local_client;
	outgoing_fwd->client_out = remote_client;

	incoming_fwd->client_in = remote_client;
	incoming_fwd->client_out = local_client;
		
	
	local_client = malloc(sizeof *local_client);
	if (local_client == NULL)
		return -1;
	memset(local_client, 0, sizeof *local_client);
	local_client->display = display;

	remote_client = malloc(sizeof *remote_client);
	if (remote_client == NULL)
		return -1;
	memset(remote_client, 0, sizeof *remote_client);
	remote_client->display = display;

	
	local_client->source = wl_event_loop_add_fd(display->loop, fd_local,
					      WL_EVENT_READABLE,
					      rwl_client_connection_data, outgoing_fwd);

	remote_client->source = wl_event_loop_add_fd(display->loop, fd_remote,
					      WL_EVENT_READABLE,
					      rwl_client_connection_data, incoming_fwd);
	
	if(remote_client->source == NULL)
		printf("This shoudln't be null\n");

	local_client->connection =
		wl_connection_create(fd_local, wl_client_connection_update, local_client);
	if (local_client->connection == NULL) {
		free(local_client);
		return -1;
	}
	//wl_list_insert(display->client_list.prev, &local_client->link);
	//wl_list_init(&local_client->resource_list);
	
	remote_client->connection =
		wl_connection_create(fd_remote, wl_client_connection_update, remote_client);
	if (remote_client->connection == NULL) {
		free(remote_client);
		return -1;
	}

	//wl_list_insert(display->client_list.prev, &remote_client->link);
	//wl_list_init(&remote_client->resource_list);
	
}


static int
rwl_socket_data(int fd, uint32_t mask, void *data)
{
	struct wl_display *display = data;
	struct sockaddr_un name;
	socklen_t length;
	int client_fd, remote_fd;

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

	remote_fd = rwl_get_remote_connection(remote_address);

	rwl_client_pair_create(display, client_fd, remote_fd);
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
				 rwl_socket_data, display) == NULL) {
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
		remote_address = (char *)&"127.0.0.1\0";
	}
	
	if (rwl_display_add_socket(display, NULL)) {
		fprintf(stderr, "failed to add socket: %m\n");
		exit(EXIT_FAILURE);
	}
	
	wl_display_run(display);

}
