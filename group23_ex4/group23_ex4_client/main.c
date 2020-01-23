#include <stdio.h>
#include <string.h>
#include "Commons.h"
#include "..\Shared\StringTools.h"
#include "client.h"

#define NUM_OF_ARGUMENTS 3
#define SERVER_IP_ARG_INDEX 1
#define SERVER_PORT_ARG_INDEX 2
#define USERNAME_ARG_INDEX 3

int main(int argc, char* argv[])
{
	int server_ip_length;
	char* server_ip;
	int port_number;
	char* username;
	int exit_code = CLIENT_SUCCESS;
	int str_copy_result;

	if (argc != NUM_OF_ARGUMENTS + 1)
	{
		printf("Not enough arguments.\n\n");
		printf("usage: group23_ex4_client.exe <server ip> <server port> <username>");
		return CLIENT_NOT_ENOUGH_CMD_ARGS;
	}

	//str_copy_result = CopyString(argv[SERVER_IP_ARG_INDEX], &server_ip);
	server_ip = CopyString(argv[SERVER_IP_ARG_INDEX]);
	if (server_ip == NULL)
		return CLIENT_MEM_ALLOC_FAILED;

	sscanf_s(argv[SERVER_PORT_ARG_INDEX], "%d", &port_number);

	username = CopyString(argv[USERNAME_ARG_INDEX]);
	if (username == NULL)
	{
		free(server_ip);
		return CLIENT_MEM_ALLOC_FAILED;
	}

	MainClient(server_ip, port_number, username);

	free(server_ip);
	free(username);

	return exit_code;
}