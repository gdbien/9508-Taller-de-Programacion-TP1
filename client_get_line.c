#include "client_get_line.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define BUFFER_SIZE 32

/*
	Shitea amount veces a izquierda a partir de buf, y rellena con \0 a derecha.
	En cuanto a los parámetros, amount debe ser < o = a buf_size.
*/
static void _shift_left_buffer(char* buf, size_t buf_size, size_t amount) {
	memmove(buf, buf + amount, buf_size - amount);
	memset(buf + buf_size - amount, '\0', amount);
}

/*
	Concatena str_2 al final de str_1, mismo uso que strcat(),
	pero utiliza memoria dinámica para el string resultante.
	Devuelve el string resultante, o NULL en caso de error.
*/
static char* _dynamic_concat(char *str_1, char *str_2) {
	int length = 0;
	char *result;

	if (str_1) length = strlen(str_1);  	
	length += strlen(str_2) + 1;
	result = realloc(str_1, length);
	if (!result) {
		free(str_1);
		return NULL;
	}
	if (!str_1) *result = '\0'; 
	strcat(result, str_2);
	return result;
}

int getline(char **line, FILE *input) {
	static char buffer[BUFFER_SIZE];
	bool found_line = false;
	char *ptr;

	free(*line);
	*line = NULL;
	while (!found_line) {
		if (strlen(buffer) == 0) {
			if (fread(buffer, 1, sizeof(buffer)-1, input) < 1) return ERROR;
			buffer[sizeof(buffer)-1] = '\0';
		}
		ptr = strchr(buffer,'\n');   
		if (ptr) {
			*ptr = '\0';
			found_line = true;	
		}
		*line = _dynamic_concat(*line, buffer);
		if (!*line) return ERROR;
		_shift_left_buffer(buffer,BUFFER_SIZE,strlen(buffer)+1);
	}
	return strlen(*line); 	 
}
