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
/*
	Intenta enviar length bytes.
	Devuelve cantidad de bytes enviados, ERROR en caso contrario.
*/
static int _client_send(client_t *self, const char *buffer, size_t length) {
	int ret = socket_send(&self->socket, buffer, length);
	if (ret < 0) return ERROR;
	return ret;
}

/*
	Intenta recibir length bytes.
	Devuelve cantidad de bytes recibidos, ERROR en caso contrario.
*/
static int _client_receive(client_t *self, char *buffer, size_t length) {
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
	Se encarga de despachar al cliente la linea (sin \n y terminada en \0)
 	que le devuelve stdin_streamer, para luego ser procesada.
 	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
static int _communicate_callback(char* buffer, size_t length, void *context) {
	client_t *client = context;
	static int32_t message_id = 1;
	char respuesta[4] = {'\0'};
	char *encoded_data = NULL;
	size_t encoded_size;
	int count = protocol_encode(&encoded_data, buffer, &encoded_size);
	if (count < 0) {
		//free((char*)buffer);
		free(encoded_data);
		return ERROR;
	} 
	count = _client_send(client, encoded_data, encoded_size);
	free(encoded_data); //Se encarga el cliente 
	if (count < 0) return ERROR;
	count = _client_receive(client, respuesta, 3);
	if (count < 0) return ERROR;
	_client_print_ok(message_id, respuesta);
	message_id++;
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

