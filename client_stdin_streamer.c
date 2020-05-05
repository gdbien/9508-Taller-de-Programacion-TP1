#include "client_stdin_streamer.h"
#include <stdlib.h>
#include "client_get_line.h"

#define BUFFER_SIZE 32

int stdin_streamer_create(stdin_streamer_t *self, callback_t callback) {
	self->callback = callback;
	self->input = stdin;
	return SUCCESS;
}

int stdin_streamer_init(stdin_streamer_t *self, const char *file_name) {
	if (file_name) {
		self->input = fopen(file_name, "r");
	 	if (!self->input) {
	 		return ERROR;
	 	}
	}
	return SUCCESS;
}

int stdin_line_streamer_run(stdin_streamer_t *self, void *context) {
	char *line = NULL;
	int count;
	int ret = 0;
	while (!feof(self->input)) {
		while ((count = getline(&line, self->input)) > 0) {
			ret = self->callback(line, count, context);
			if (ret < 0) break;
		}
	}
	free(line);
	if (ret < 0) return ERROR;
	return SUCCESS;
}

int stdin_streamer_run(stdin_streamer_t *self, void *context) {
	char buffer[BUFFER_SIZE];
	size_t read_count;
	int ret = 0;
	while (!feof(self->input)) {
		read_count = fread(buffer, 1, sizeof(buffer), self->input);
		ret = self->callback(buffer, read_count, context);
		if (ret < 0) return ERROR;
	}
	return SUCCESS;
}

int stdin_streamer_destroy(stdin_streamer_t *self) {
	if (self->input != stdin) fclose(self->input);
	return SUCCESS;
}
