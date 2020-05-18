#define  _POSIX_C_SOURCE 201112L
#include "common_socket.h"
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>

const int ACCEPT_QUEUE_LEN = 10;

static void _set_hints(struct addrinfo *hints, int ai_flags, int ai_family,
					   int ai_socktype) {
	memset(hints, 0, sizeof(struct addrinfo));
	hints->ai_family = ai_family;
	hints->ai_socktype = ai_socktype;
	hints->ai_flags = ai_flags;
}

/*
	Itera las la lista de addrinfo hasta encontrar la dirreción que cumpla con
    los parametros de res, asignando el file descriptor a self.
    En el caso del servidor hace un bind(), y en el caso del cliente hace un
    connect().
	Devuelve SUCCESS si ok, y ERROR en caso contrario.
*/
static int _iterate_addrinfo(socket_t *self, struct addrinfo *res,
						     bool is_server) {
	struct addrinfo *rp;
	int opt_val = 1;

	for (rp = res; rp != NULL; rp = rp->ai_next) {
        self->fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (self->fd == -1) continue;
        if (is_server) {
        	setsockopt(self->fd, SOL_SOCKET, SO_REUSEADDR, &opt_val,
        			   sizeof(opt_val));
        	if (bind(self->fd, rp->ai_addr, rp->ai_addrlen) == 0) {
        		break;	
        	}  
        } else {
        	if (connect(self->fd, rp->ai_addr, rp->ai_addrlen) == 0) {
        		break;	
        	}
        }
        close(self->fd);
    }
    freeaddrinfo(res);
	if (!rp) return ERROR;   
	return SUCCESS;
}

/*
	Encapsula y junta todos los llamados a funciones para obtener un socket en
	funcionamiento, ya sea un cliente o servidor.
	Devuelve SUCCESS si ok, y ERROR en caso contrario.
*/
static int _getaddrinfo(socket_t *self, const char *node, const char *service,
					    bool is_server) {	
	struct addrinfo hints;
    struct addrinfo *res;
    int status;
    _set_hints(&hints, is_server ? AI_PASSIVE : 0, AF_INET, SOCK_STREAM);
    status = getaddrinfo(node, service, &hints, &res);
    if (status != 0) return ERROR;
    _iterate_addrinfo(self, res, is_server);
	return SUCCESS;   
}

int socket_create(socket_t *self) {	
	self->fd = -1;
	return SUCCESS;
}

int socket_destroy(socket_t *self) {
	return SUCCESS;
}

int socket_bind_and_listen(socket_t *self, const char *service) {
	_getaddrinfo(self, NULL, service, true);
	listen(self->fd,ACCEPT_QUEUE_LEN);
	return SUCCESS;
}

int socket_accept(socket_t *self, socket_t *accepted_socket) {
    accepted_socket->fd = accept(self->fd, NULL, NULL);
    if (accepted_socket->fd == -1) return ERROR;
  	return SUCCESS;
}

int socket_connect(socket_t *self, const char *host_name, const char *service) {
	return _getaddrinfo(self, host_name, service, false);
}

int socket_send(socket_t *self, const char *buffer, size_t length) {
	int tot_sent = 0;
	int actual_sent;
	while (tot_sent != length) {
 		actual_sent = send(self->fd, buffer + tot_sent, length - tot_sent,
 						   MSG_NOSIGNAL);
 		if (actual_sent == -1) return ERROR;
 		tot_sent += actual_sent;
 	}
 	return tot_sent;
}

int socket_receive(socket_t *self, char *buffer, size_t length) {
	int tot_recv = 0;
	int actual_recv;
	while (tot_recv != length) {
    	actual_recv = recv(self->fd, buffer + tot_recv, length - tot_recv, 0);
    	if (actual_recv == 0) break;
 		if (actual_recv == -1) return ERROR;
    	tot_recv += actual_recv;
   }
   return tot_recv;
}

int socket_shutdown(socket_t *self, int channel) {
	int value = shutdown(self->fd, channel);
	close(self->fd);
	if (value < 0) return ERROR;
	return SUCCESS;
}
