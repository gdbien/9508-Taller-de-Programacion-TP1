#ifndef CLIENT_H
#define CLIENT_H

#include "common_socket.h"

typedef struct client_t {
	socket_t socket;
} client_t;

#define SUCCESS 0
#define ERROR -1

/*
 	Constructor del client.
 	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int client_create(client_t *self);
/*
 	Destructor del client.
 	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int client_destroy(client_t *self);
/*
	Se conecta a un servidor dada una dirección y un servicio o puerto
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int client_connect(client_t *self, const char *host_name, const char *service);
/*
	Intenta enviar length bytes.
	Devuelve cantidad de bytes enviados, ERROR en caso contrario.
*/
int client_send(client_t *self, const char *buffer, size_t length);
/*
	Intenta recibir length bytes.
	Devuelve cantidad de bytes recibidos, ERROR en caso contrario.
*/
int client_receive(client_t *self, char *buffer, size_t length);
/*
	Se encarga de encapsular y delegar el manejo de la entrada,
	procesando todas las llamadas.
	Devuelve SUCESS si ok, ERROR en caso contrario.
*/
int client_run(client_t *self, const char *file_name);
/*
 	Cierra la comunicación de lectura y escritura
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int client_shutdown(client_t *self);

#endif // CLIENT_H
