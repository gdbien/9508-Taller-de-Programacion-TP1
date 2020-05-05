#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdlib.h>
#include <stdint.h>

#define SUCCESS 0
#define ERROR -1

typedef struct header_pre {
	int8_t endianess;
	int8_t m_type;
	int8_t flags;
	int8_t pro_version;
	int32_t body_size;
	int32_t message_id;
	int32_t arr_size;
} __attribute__ ((packed)) header_pre_t;

typedef struct base_param {
	int8_t par_type;
	int8_t byte_1;
	int8_t data_type;
	int8_t null_byte;
} __attribute__ ((packed)) base_param_t;

typedef struct norm_param {
	base_param_t base_param;
	int32_t data_size;
} __attribute__ ((packed)) norm_param_t;

typedef struct sign_param {
	base_param_t base_param;
	int8_t arg_count;
} __attribute__ ((packed)) sign_param_t;

void set_header_pre(header_pre_t *header_pre, int8_t endianess, int8_t m_type,
					int8_t flags, int8_t pro_version, int32_t body_size,
					int32_t arr_size);

void set_base_param(base_param_t *base_param, int8_t par_type, int8_t byte_1,
					int8_t data_type, int8_t null_byte);

void set_norm_param(norm_param_t *norm_para, int8_t par_type, int8_t byte_1, 
					int8_t data_type, int8_t null_byte, int32_t data_size);


void set_sign_param(sign_param_t *sign_param, int8_t par_type, int8_t byte_1, 
					int8_t data_type, int8_t null_byte, int8_t arg_count);

int next_multiple_8(size_t number); //ESTO TENGO QUE SACARLO DE ACA Y PONERLO EN ALGO TIPO utils.h
									//igual que otras funciones tipo strdup etc


/*
	Recibe un buffer para que se le aplique el protocolo,
	y devuelva el llamado encodeado en data (reservando memoria dinámica
	para él).
	Debe ser llamado inicialmente con *data = NULL, y el usuario debe
	encargarse de liberar la memoria solo en el último llamado, o en caso
	de error.
	Devuelve la cantidad de bytes que ocupa el mensaje encodeado,
	ERROR en caso de error.
*/
int protocol_encode(char **data, const char *buffer, size_t *encoded_size);

/*
	Falta documentar, y no es la version final de las funciones, les falta mucho refactorización y no repetir código, pero primero queria tener
	un ejemplo basico andando, y luego si funcionaba, mejorar la solución.
*/

int protocol_decode_arguments(const char* encoded_body, size_t body_size, char *arg_names[], size_t n_arg); //HAY QUE CAMBIAR LA API ?
int protocol_decode_parameters(const char *encoded_arr, size_t length, char *param_names[4], size_t* n_arg);//HAY QUE CAMBIAR LA API ?

#endif // PROTOCOL_H