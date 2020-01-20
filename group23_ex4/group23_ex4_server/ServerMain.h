#ifndef SERVER_MAIN
#define SERVER_MAIN

#include <winsock2.h>

#define USERNAME_MAX_LENGTH 20

typedef struct client_info
{
	char userinfo[USERNAME_MAX_LENGTH];
	SOCKET socket;
} client_info_t;

int RunServer(int port_number);

#endif
