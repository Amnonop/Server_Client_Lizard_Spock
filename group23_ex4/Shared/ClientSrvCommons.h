#ifndef CLIENT_SRV_COMMONS_H
#define CLIENT_SRV_COMMONS_H

#define STRINGS_ARE_EQUAL( Str1, Str2 ) ( strcmp( (Str1), (Str2) ) == 0 )

#define SERVER_RESPONSE_TIMEOUT 15
#define CLIENT_RESPONSE_TIMEOUT 15
#define CLIENT_VS_RESPONSE_TIMEOUT 30

typedef enum
{
	ROCK = 0,
	PAPER = 1,
	SCISSORS = 2,
	LIZARD = 3,
	SPOCK = 4,
	MOVE_TYPE_NONE = 5
} MOVE_TYPE;

typedef enum
{
	CLIENT_VERSUS = 1,
	CLIENT_CPU = 2,
	LEADERBOARD = 3,
	QUIT = 4
} MAIN_MENU_OPTIONS;

typedef enum
{
	OPT_REPLAY = 1,
	OPT_MAIN_MENU = 2
} GAME_OVER_MENU_OPTIONS;

MOVE_TYPE StringToMoveType(const char* move_type_string);

#endif
