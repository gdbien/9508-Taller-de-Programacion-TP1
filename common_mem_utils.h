#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include <stdlib.h>
#include <stdint.h>

/*
	Concatena en un array resultante, arr1 y arr2, reservando memoria dinámica
	que el usuario debe liberar.
	Debe ser llamado con arr1 = NULL;
	Devuelve un puntero al array concatenado resultante, o NULL, en caso de 
	error.
*/
char* array_concat(char *arr1, size_t arr1_size, char *arr2, size_t arr2_size,
				   int32_t *result_size);
/*
	Hace una copia en el heap de str (el usuario debe liberarla).
	Trata de emular strdup() de string.h
	Devuelve una copia dinamica de str, o NULL en caso contrario.
*/
char* strdup(const char *str);

/*
	Concatena str_2 al final de str_1, mismo uso que strcat(),
	pero utiliza memoria dinámica para el string resultante.
	Devuelve el string resultante, o NULL en caso de error.
*/
char* dynamic_concat(char *str_1, char *str_2);

#endif // MEMORY_UTILS_H
