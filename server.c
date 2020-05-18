#include "server.h"
#include <stdio.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "common_protocol.h"

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

int server_bind_and_listen(server_t *self, const char *service) {
	int ret = socket_bind_and_listen(&self->socket, service);
	if (ret < 0) return ERROR;
	return SUCCESS;
}

int server_accept(server_t *self) {
	int ret = socket_accept(&self->socket, &self->socket_acpt);
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

int server_send(server_t *self, const char *buffer, size_t length) {
	int ret = socket_send(&self->socket_acpt, buffer, length);
	if (ret < 0) return ERROR;
	return ret;
}

int server_receive(server_t *self, char *buffer, size_t length) {
	int ret = socket_receive(&self->socket_acpt, buffer, length);
	if (ret < 0) return ERROR;
	return ret;
}

int _server_receive_wrapper(void* self, char *buffer, size_t length) {
	return server_receive((server_t*) self, buffer, length);
}

int server_communicate(server_t *self) {
	int ret;
	int count;
	message_t message;
	while (true) {
		message_create(&message);
		ret = protocol_receive(_server_receive_wrapper, &message, self);
		if (ret < 0) {
			break;
		}
		message_print(&message);
		message_destroy(&message);
		count = server_send(self, "OK\n", 3);
		if (count < 0) break;	
	}
	return SUCCESS;
}
