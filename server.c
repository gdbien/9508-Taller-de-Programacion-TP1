#include "server.h"
#include <stdio.h>
#include "common_protocol.h"
#include <arpa/inet.h>

int server_create(server_t *self) {
	int ret;
	ret = socket_create(&self->socket);
	if (ret < 0) return ERROR;
	ret = socket_create(&self->socket_acpt);
	if (ret < 0) return ERROR;
	return SUCCESS;
}

int server_destroy(server_t *self) {
	socket_destroy(&self->socket);
	socket_destroy(&self->socket_acpt);
	return SUCCESS;
}

int server_run(server_t *self, const char *service) {
	int ret;
	ret = socket_bind_and_listen(&self->socket, service);
	if (ret < 0) return ERROR;
	ret = socket_accept(&self->socket, &self->socket_acpt);
	if (ret < 0) return ERROR;
	return SUCCESS;
}


int server_shutdown(server_t *self) {
	int ret = socket_shutdown(&self->socket, SHUT_RDWR);
	if (ret < 0) return ERROR;
	ret = socket_shutdown(&self->socket_acpt, SHUT_RDWR);
	if (ret < 0) return ERROR;
	return SUCCESS;
}

/*
	Intenta enviar length bytes de buffer.
	Devuelve cantidad de bytes enviados, ERROR en caso contrario.
*/
int _server_send(server_t *self, const char *buffer, size_t length) {
	int ret = socket_send(&self->socket_acpt, buffer, length);
	if (ret < 0) return ERROR;
	return ret;
}

/*
	Intenta recibir length bytes de buffer.
	Devuelve cantidad de bytes recibidos, ERROR en caso contrario.
*/
int _server_receive(server_t *self, char *buffer, size_t length) {
	int ret = socket_receive(&self->socket_acpt, buffer, length);
	if (ret < 0) return ERROR;
	return ret;
}

void _server_print_message(int32_t message_id, char *param_print[4], char *args_print[], size_t n_arg) {
	printf("* Id: 0x%08x\n", message_id);
	printf("* Destino: %s\n", param_print[0]);
	printf("* Path: %s\n", param_print[1]);
	printf("* Interfaz: %s\n", param_print[2]);
	printf("* Metodo: %s\n", param_print[3]);
	if (n_arg > 0) {
		printf("* Parámetros:\n");
		for (size_t i = 0; i < n_arg; i++) {
			printf("	* %s\n", args_print[i]);
		}
	}
}

int server_communicate(server_t *self) {
	int count;
	size_t last_padding = 0;
	//char *words[];
	int n_multiple_8;
	size_t n_arg = 0;
	char *arg_names[n_arg];
	header_pre_t header_pre;
	char *buffer = NULL;
	char *param_names[4];
	size_t arr_size;
	do {
		count = _server_receive(self, (char*) &header_pre, sizeof(header_pre_t));
		if (count < 0) return ERROR;
		//if (count == 0) break;
		arr_size = ntohl(header_pre.arr_size);

		n_multiple_8 = next_multiple_8(arr_size);
		if (arr_size != n_multiple_8) {
			last_padding = n_multiple_8 - arr_size;
		}

		buffer = malloc(arr_size * sizeof(char));
		count = _server_receive(self, buffer, arr_size);
		if (count < 0) return ERROR;
		//if (count == 0) break;
		protocol_decode_parameters(buffer, arr_size, param_names, &n_arg);

		
		printf("N ARG: %ld\n",n_arg);

		//Tengo que consumir el padding
		buffer = realloc(buffer, last_padding * sizeof(char)); //MEDIO FEO, SI FALLA REALLOC PIERDO LA MEMORIA DE BUFFER, CAMBIAR DESPUES
		count = _server_receive(self, buffer, last_padding);
		if (count < 0) return ERROR;
		//if (count == 0) break;

		size_t body_size = ntohl(header_pre.body_size);
		if (body_size != 0)	{
			buffer = realloc(buffer, body_size * sizeof(char));
			count = _server_receive(self, buffer, body_size);
			if (count < 0) return ERROR;
			//if (count == 0) break;
			protocol_decode_arguments(buffer, body_size, arg_names, n_arg);
		}
		
		_server_print_message(header_pre.message_id, param_names, arg_names, n_arg);

		for (size_t i = 0; i < 4; i++) {
			free(param_names[i]);
		}

		if (n_arg > 0) {
			for (size_t i = 0; i < n_arg; i++) {
				free(arg_names[i]);
			}	
		}
		
		free(buffer);
		count = _server_send(self, "OK\n" , 3);
		if (count < 0) return ERROR;	
	} while (count > 0);
	return SUCCESS;
}


