#include "common_message.h"
#include <string.h>
#include <stdio.h>
#include "common_mem_utils.h"

/*
	Devuelve la cantidad de argumentos que posee la función.
*/
static size_t _get_arg_count(const char *buffer){
	size_t count = 0;
	char *ptr_start = strchr(buffer, '(');
	char *ptr_end = strchr(buffer, ')');
	if (ptr_end - ptr_start == 1) return count;
	count += 1;
	char *ptr = ptr_start;
	while ((ptr = strchr(ptr, ','))) {
    	count++;
    	ptr++;
	}
	return count;
}

/*
	Devuelve la cantidad de bytes que ocupa el nombre del metodo
*/
static size_t _get_bytes_method_name(char *method) {
	size_t count = 0;
	while (*method != '(') {
		method++;
		count++;
	} 
	return count;
}

/*
	Se encarga de pedir la memoria dinamica exacta para almacenar el nombre
	del metodo (sin los parametros). EL usuario debe liberar la memoria, y
	method debe ser un string previamente en el heap.
	Devuelve un string con el nombre, NULL en caso de error.
*/
static char* _get_method_name(char *method) {
	size_t bytes_name_method = _get_bytes_method_name(method);
	char* result = realloc(method, bytes_name_method + 1);
	if (!result) return NULL;
	*(result + bytes_name_method) = '\0';
	return result;
}

/*
	Recibe la llamada de buffer, parsea por espacio las palabras, y las
	alloca dinamicamente en param_names.
	El usuario debe hacerse cargo de liberar la memoria de cada string.
	Devuelva SUCCESS si ok, ERROR en caso contrario.
*/
static int _dup_param_names(const char *buffer, char *param_names[4]) {
	size_t i = 0;
	char *copy = strdup(buffer);
	if (!copy) return ERROR;
	char *token = strtok(copy, " ");
    while (i < 4) {
        param_names[i] = strdup(token);
        if (!param_names[i]) {
        	free(copy);
        	return ERROR;
        }
        token = strtok(NULL, " ");
        i++;
    }
    param_names[3] = _get_method_name(param_names[3]);
    free(copy);
    return SUCCESS;
}

/*
	Recibe la llamada de buffer, parsea por ',' los argumentos, y los
	alloca dinamicamente en arg_names.
	El usuario debe hacerse cargo de liberar la memoria de cada string.
	Devuelva SUCCESS si ok, ERROR en caso contrario.
*/
static int _dup_args_names(const char *buffer, char *arg_names[]) {
	size_t i = 0;
	char *ptr_start = strchr(buffer, '(');
	char *ptr_end = strchr(buffer, ')');
	size_t bytes_amount = ptr_end - ptr_start - 1;

	char *args= malloc(bytes_amount * sizeof(char) + 1);
	if(!args) return ERROR;
	memcpy(args, ptr_start + 1, bytes_amount);
	*(args + bytes_amount) = '\0';

	char *token = strtok(args, ",");
    while (token) {
        arg_names[i] = strdup(token);
        token = strtok(NULL, ",");
        i++;
    }
    free(args);
    return SUCCESS;
}

int message_create(message_t *self) {
	return SUCCESS;
}

int message_init(message_t *self, const char *buffer) {
	static size_t id = 1;
	char *param_names[4];
	_dup_param_names(buffer,param_names);
	self->id = id;
	self->destination = param_names[0];
	self->path = param_names[1];
	self->interface = param_names[2];
	self->method = param_names[3];
	self->n_args = _get_arg_count(buffer);
	self->arguments = malloc(sizeof(char*)*self->n_args);
	if (!self->arguments) return ERROR;
	_dup_args_names(buffer, self->arguments);
	id++;
	return SUCCESS;
}

int message_destroy(message_t *self) {
	free(self->destination);
	free(self->path);
	free(self->interface);
	free(self->method);
	for (size_t i = 0; i < self->n_args; i++) {
		free(self->arguments[i]);
	}
	free(self->arguments);
	return SUCCESS;
}

void message_print(message_t *self) {
	printf("* Id: 0x%08x\n", (int)self->id);
	printf("* Destino: %s\n", self->destination);
	printf("* Ruta: %s\n", self->path);
	printf("* Interfaz: %s\n", self->interface);
	printf("* Metodo: %s\n", self->method);
	size_t n_args = self->n_args;
	if (n_args > 0) {
		printf("* Parámetros:\n");
		for (size_t i = 0; i < n_args; i++) {
			printf("	* %s\n", self->arguments[i]);
		}
	}
	printf("\n");
}

void message_setter(message_t *self, size_t id, char *destination,
					char *path, char *interface, char *method,
					size_t n_args, char **arguments) {
	self->id = id;
	self->destination = destination;
	self->path = path;
	self->interface = interface;
	self->method = method;
	self->n_args = n_args;
	self->arguments = arguments;
}

size_t message_get_id(message_t* self) {
	return self->id;
}
