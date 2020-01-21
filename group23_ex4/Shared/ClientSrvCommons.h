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
	CLIENT_CPU,
	CLIENT_VERSUS,
	LEADERBOARD,
	QUIT
} CLIENT_MENU_OPTIONS;

#endif
