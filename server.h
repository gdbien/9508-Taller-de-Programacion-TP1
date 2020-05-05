#ifndef SERVER_H
#define SERVER_H

#include "common_socket.h"

#define SUCCESS 0
#define ERROR -1

typedef struct server_t {
	socket_t socket;
	socket_t socket_acpt;
} server_t;

/*
 	Constructor del server.
 	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int server_create(server_t *self);
/*
 	Destructor del server.
 	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int server_destroy(server_t *self);
/*
 	Pone al servidor a correr en el puerto service
 	y listo para comunicarse.
 	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int server_run(server_t *self, const char *service);
/*
 	Se encarga de la recepción y envío de mensajes.
 	Devuelve SUCCES si ok, ERROR en caso contrario.
*/
int server_communicate(server_t *self);
/*
 	Cierra la comunicación de lectura y escritura
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int server_shutdown(server_t *self);


#endif // SERVER_H
