#include <stdlib.h>
#include "client.h"

int main(int argc, char const *argv[]) {
	int ret;
	client_t client;

	ret = client_create(&client);
	if (ret < 0) {
		return EXIT_FAILURE;
	} 

	ret = client_connect(&client, argv[1], argv[2]);
	if (ret < 0) {
		client_shutdown(&client);
		client_destroy(&client);
		return EXIT_FAILURE;	
	}

	ret = client_run(&client, argv[3]);
	if (ret < 0) {
		client_shutdown(&client);
		client_destroy(&client);
		return EXIT_FAILURE;	
	}

	client_shutdown(&client);
	client_destroy(&client);
	return EXIT_SUCCESS;
}