#include "client.h"
#include "common_protocol.h"
#include "client_stdin_streamer.h"

int client_create(client_t *self) {
	int ret = socket_create(&self->socket);
	if (ret < 0) return ERROR;
	return SUCCESS;
}

int client_destroy(client_t *self) {
	socket_destroy(&self->socket);
	return SUCCESS;
}

int client_connect(client_t *self, const char *host_name, const char *service) {
	int ret = socket_connect(&self->socket, host_name, service);
	if (ret < 0) return ERROR;
	return SUCCESS;
}

int client_shutdown(client_t *self) {
	int ret = socket_shutdown(&self->socket, SHUT_RDWR);
	if (ret < 0) return ERROR;
	return SUCCESS;
}

int client_send(client_t *self, const char *buffer, size_t length) {
	int ret = socket_send(&self->socket, buffer, length);
	if (ret < 0) return ERROR;
	return ret;
}

int client_receive(client_t *self, char *buffer, size_t length) {
	int ret = socket_receive(&self->socket, buffer, length);
	if (ret < 0) return ERROR;
	return ret;
}

/*
	Imprime el OK que le envia el servidor, con el message_id
	en formate hexadecimal.
*/
static void _client_print_ok(int32_t message_id, char *respuesta) {
	printf("0x%08x: %s", message_id, respuesta);
}

/*
	Wrapper que se utiliza para poder pasarle la funcion client_send,
	al protocolo, y que este no tenga que depender de client.h
	Devuelve lo mismo que client_send().

*/
static int _client_send_wrapper(void* self, char *buffer, size_t length) {
	return client_send((client_t*) self, buffer, length);
}

/*
	Se encarga de despachar al cliente la linea (sin \n y terminada en \0)
 	que le devuelve stdin_streamer, para luego ser procesada.
 	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
static int _communicate_callback(char* buffer, size_t length, void *context) {
	client_t *client = context;
	message_t message;
	message_create(&message);
	message_init(&message, buffer);
	char respuesta[4] = {'\0'};
	int ret = protocol_send(_client_send_wrapper, &message, client);
	if (ret < 0) {
		message_destroy(&message);
		return ERROR;
	}
	message_destroy(&message);
	int count = client_receive(client, respuesta, 3);
	if (count < 0) return ERROR;
	_client_print_ok(message_get_id(&message), respuesta);
	return SUCCESS;
}

int client_run(client_t *self, const char *file_name) {
	stdin_streamer_t stdin_streamer;
	stdin_streamer_create(&stdin_streamer, _communicate_callback);
	int ret = stdin_streamer_init(&stdin_streamer, file_name);
	if (ret < 0) return ERROR;
	ret = stdin_line_streamer_run(&stdin_streamer, self);
	stdin_streamer_destroy(&stdin_streamer);
	if (ret < 0) return ERROR;
	return SUCCESS;
}
