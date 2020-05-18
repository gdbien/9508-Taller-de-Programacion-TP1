#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdlib.h>
#include <stdint.h>
#include "common_message.h"

#define SUCCESS 0
#define ERROR -1

typedef int(*prot_cb_t)(void *context, char *buffer, size_t length);

typedef struct header_pre {
	int8_t endianess;
	int8_t m_type;
	int8_t flags;
	int8_t pro_version;
	int32_t body_size;
	int32_t message_id;
	int32_t arr_size;
} __attribute__((packed)) header_pre_t;

typedef struct base_param {
	int8_t par_type;
	int8_t byte_1;
	int8_t data_type;
	int8_t null_byte;
} __attribute__((packed)) base_param_t;

typedef struct norm_param {
	base_param_t base_param;
	int32_t data_size;
} __attribute__((packed)) norm_param_t;

typedef struct sign_param {
	base_param_t base_param;
	int8_t arg_count;
} __attribute__((packed)) sign_param_t;

/*
	Delega la codificacion de message, y una vez hecho, lo envia a través
	de cb_send, utilizando context como parámetro.
	Devuelve la cantidad de bytes del mensaje encodeado, o ERROR en caso
	contrario
*/
int protocol_send(prot_cb_t cb_send, message_t *message, void *context);
/*
	Se encarga de recibir el buffer encodeado a través de cb_receive,
	utilizando context como parámetro. Delega la decodificación de este,
	e inicializa (con memoria dinámica) el message pasado por referencia.
	El usuario debe encargarse de liberar la memoria de este.
	Devuelve SUCCES si ok, o ERROR en caso contrario.
*/
int protocol_receive(prot_cb_t cb_receive, message_t *message, void *context);

#endif // PROTOCOL_H
