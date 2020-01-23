#ifndef CLIENT_SRV_COMMONS_H
#define CLIENT_SRV_COMMONS_H

typedef enum
{
	ROCK = 0,
	PAPER = 1,
	SCISSORS = 2,
	LIZARD = 3,
	SPOCK = 4
} MOVE_TYPE;

typedef enum
{
	CLIENT_VERSUS = 1,
	CLIENT_CPU = 2,
	LEADERBOARD = 3,
	QUIT = 4
} MAIN_MENU_OPTIONS;

#endif
