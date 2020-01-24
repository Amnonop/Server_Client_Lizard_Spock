#ifndef COMMONS_H
#define COMMONS_H

#define STRINGS_ARE_EQUAL( Str1, Str2 ) ( strcmp( (Str1), (Str2) ) == 0 )

typedef enum
{
	CLIENT_SUCCESS,
	CLIENT_NOT_ENOUGH_CMD_ARGS,
	CLIENT_WSA_STARTUP_ERROR,
	CLIENT_SOCKET_CREATION_FAILED,
	CLIENT_INVALID_IP_ADDRESS,
	CLIENT_SOCKET_BINDING_FAILED,
	CLIENT_SOCKET_LISTEN_FAILED,
	CLIENT_ACCEPT_CONNECTION_FAILED,
	CLIENT_MEM_ALLOC_FAILED,
	CLIENT_SEND_MSG_FAILED,
	CLIENT_RECEIVE_MSG_FAILED,
	CLIENT_SERVER_APPROVED,
	CLIENT_SERVER_DENIED,
	CLIENT_UNEXPECTED_MESSAGE,
	CLIENT_MSG_PARSE_FAILED,
	CLIENT_MSG_QUEUE_INIT_FAILED,
	CLIENT_THREAD_CREATION_FAILED,
} CLIENT_EXIT_CODES;

typedef enum
{
	CLIENT_REQUEST
} CLIENT_MESSAGE_TYPES;

typedef enum
{
	FIRST_CONNECTION,
	WAITING_SERVER_APPROVAL,
	SERVER_APPROVED,
	MAIN_MENU,
	WAITING_TO_START_GAME,
	PLAY_MOVE,
	WAITING_GAME_RESULTS
} CLIENT_STATE;

typedef enum
{
	OPT_RECONNECT = 1,
	OPT_EXIT = 2
} SERVER_CONNECT_MENU_OPTIONS;


typedef enum
{
	CLIENT_REFRESH = 1,
	CLIENT_MAIN_MENU = 2
} CLIENT_LEADERBOARD_OPTIONS;

#endif
