#ifndef STDIN_STREAMER_H
#define STDIN_STREAMER_H

#include <stdio.h>

#define SUCCESS 0
#define ERROR -1

typedef int(*callback_t)(char *buffer, size_t length, void *context);

typedef struct stdin_streamer {
	callback_t callback;
	FILE *input;
} stdin_streamer_t;

int stdin_streamer_create(stdin_streamer_t *self, callback_t callback);
/*
	Inicializa stdin_streamer_t con filename, si es NULL deja por default stdin.
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int stdin_streamer_init(stdin_streamer_t *self, const char *file_name);
/*
	Lee el input de a líneas, y envia su contenido a
	la funcion callback, junto con el contexto.
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int stdin_line_streamer_run(stdin_streamer_t *self, void *context);
/*
	Lee el input de a bloques de 32 bytes, y envia su contenido a
	la funcion callback, junto con el contexto.
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int stdin_streamer_run(stdin_streamer_t *self, void *context);

int stdin_streamer_destroy(stdin_streamer_t *self);

#endif // STDIN_STREAMER_H