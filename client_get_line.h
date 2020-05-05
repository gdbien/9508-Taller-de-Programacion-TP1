#ifndef	GET_LINE_H
#define GET_LINE_H

#include <stdio.h>

#define ERROR -1

/*
	Obtiene una línea desde input y la alloca en *line,
	input debe estar abierto y debe manejarlo el usuario.
	*Line tiene que ser NULL en el primer llamado.
	El llamador debe liberar *line en el último llamado.
	Devuelve el tamaño de la línea, o ERROR en caso contrario.
*/
int getline(char **line, FILE *input);

#endif // GET_LINE_H
