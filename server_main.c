#include <stdlib.h>
#include "server.h"

int main(int argc, char const *argv[]) {
	int ret;
	server_t server;

	ret = server_create(&server);
	if (ret < 0) {
		return EXIT_FAILURE;
	} 

	ret = server_bind_and_listen(&server,argv[1]);
	if (ret < 0) {
		server_shutdown(&server);
		server_destroy(&server);
		return EXIT_FAILURE;	
	}

	ret = server_accept(&server);
	if (ret < 0) {
		server_shutdown(&server);
		server_destroy(&server);
		return EXIT_FAILURE;	
	}

	ret = server_communicate(&server);
	if (ret < 0) {
		server_shutdown(&server);
		server_destroy(&server);
		return EXIT_FAILURE;	
	}

	server_shutdown(&server);
	server_destroy(&server);
	return EXIT_SUCCESS;
}
