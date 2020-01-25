#ifndef SERVER_MAIN
#define SERVER_MAIN

#include <winsock2.h>

#define USERNAME_MAX_LENGTH 20

typedef struct client_info
{
	char userinfo[USERNAME_MAX_LENGTH];
	SOCKET socket;
	int client_id;
	BOOL request_to_play;
} client_info_t;

typedef struct winners_list
{
	char* name[USERNAME_MAX_LENGTH];;
	int won;
	int lost;
	struct winners_list* next;
} winners_list_t;

int RunServer(int port_number);

#endif
