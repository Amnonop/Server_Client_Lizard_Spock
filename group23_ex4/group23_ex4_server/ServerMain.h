#ifndef SERVER_MAIN
#define SERVER_MAIN

#include <winsock2.h>

#define USERNAME_MAX_LENGTH 20

typedef enum
{
	STATE_MAIN_MENU,
	STATE_VERSUS,
	STATE_CPU,
	STATE_LEADERBOARD,
	STATE_QUIT,
	STATE_NONE
} CLIENT_STATE;

typedef struct client_info
{
	char userinfo[USERNAME_MAX_LENGTH];
	SOCKET socket;
	int client_id;
	BOOL request_to_play;
	CLIENT_STATE state;
} client_info_t;

//typedef struct game_session
//{
//	client_info_t* owner;
//	client_info_t* opponent;
//} game_session_t;

typedef struct winners_list
{
	char* name[USERNAME_MAX_LENGTH];
	int won;
	int lost;
	struct winners_list* next;
} winners_list_t;

int RunServer(int port_number);

#endif
