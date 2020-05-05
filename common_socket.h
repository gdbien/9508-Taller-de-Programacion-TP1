#ifndef SOCKET_H
#define SOCKET_H

#include <sys/socket.h>
#include <stddef.h>


typedef struct socket_t {
	int fd;
} socket_t;

#define SUCCESS 0
#define ERROR -1

/*
 	Constructor del socket, inicializa fd en -1
 	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int socket_create(socket_t *self);
/*
 	Destructor del socket.
 	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int socket_destroy(socket_t *self);
/*
 	Asigna una dirección al socket y lo marca como socket pasivo
 	(lado del servidor).
  	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int socket_bind_and_listen(socket_t *self, const char *service);
/*
	Espera a que se conecte un cliente y lo asigna en accepted socket
	(lado del servidor).
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int socket_accept(socket_t *self, socket_t *accepted_socket);
/*
	Se conecta a un servidor dada una dirección y un servicio o puerto
	(lado del cliente)
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int socket_connect(socket_t *self, const char *host_name, const char *service);
/*
	Intenta enviar @param length bytes a traves del socket.
	Devuelve cantidad de bytes enviados, ERROR en caso contrario.
*/
int socket_send(socket_t *self, const char *buffer, size_t length);
/*
	Intenta recibir @param length bytes a traves del socket.
	Devuelve cantidad de bytes recibidos, ERROR en caso contrario.
*/
int socket_receive(socket_t *self, char *buffer, size_t length);
/*
 	Cierra los canales de lectura y/o escritura del socket.
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int socket_shutdown(socket_t *self, int channel);

#endif // SOCKET_H






