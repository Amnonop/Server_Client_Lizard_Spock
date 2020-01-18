#include <stdio.h>
#include "Commons.h"
#include "ServerMain.h"

#define NUM_OF_ARGUMENTS 1
#define PORT_ARG_INDEX 1

int main(int argc, char* argv[]) 
{
	int port_number;
	int exit_code = SERVER_SUCCESS;

	if (argc != NUM_OF_ARGUMENTS + 1)
	{
		printf("You must specify a port number.\n\n");
		printf("usage: group23_ex4_server.exe <port>");
		return SERVER_NOT_ENOUGH_CMD_ARGS;
	}

	sscanf_s(argv[PORT_ARG_INDEX], "%d", &port_number);

	RunServer(port_number);

	return exit_code;
}