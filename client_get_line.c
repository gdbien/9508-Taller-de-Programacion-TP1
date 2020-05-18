#include "client_get_line.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "common_mem_utils.h"

#define BUFFER_SIZE 32

/*
	Shitea amount veces a izquierda a partir de buf, y rellena con \0 a derecha.
	En cuanto a los parámetros, amount debe ser < o = a buf_size.
*/
static void _shift_left_buffer(char* buf, size_t buf_size, size_t amount) {
	memmove(buf, buf + amount, buf_size - amount);
	memset(buf + buf_size - amount, '\0', amount);
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
		*line = dynamic_concat(*line, buffer);
		if (!*line) return ERROR;
		_shift_left_buffer(buffer,BUFFER_SIZE, strlen(buffer)+1);
	}
	return strlen(*line); 	 
}
