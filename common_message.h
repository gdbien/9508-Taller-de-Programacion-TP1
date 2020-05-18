#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdlib.h>

#define SUCCESS 0
#define ERROR -1

typedef struct message {
	size_t id;
	char *destination;
	char *path;
	char *interface;
	char *method;
	size_t n_args;
	char **arguments;
} message_t;

int message_create(message_t *self);

int message_init(message_t *self, const char *buffer);

int message_destroy(message_t *self);

void message_print(message_t *self);

void message_setter(message_t *self, size_t id, char *destination,
					char *path, char *interface, char *method,
					size_t n_args, char **arguments);

size_t message_get_id(message_t *self);

#endif // MESSAGE_H
