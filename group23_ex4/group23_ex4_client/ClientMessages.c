#include <string.h>
#include <stdlib.h>
#include "ClientMessages.h"
#include "MessageQueue.h"
#include "Commons.h"
#include "../Shared/ClientSrvCommons.h"

int SendLeaderboardMessage(message_queue_t* message_queue)
{
	const char* message_name = "CLIENT_LEADERBOARD";
	return SendClientMessageWithoutParams(message_name, message_queue);
}

int SendClientMessageWithoutParams(const char* message_name, message_queue_t* message_queue)
{
	int message_length;
	char* message_string;

	// Build message string
	message_length = strlen(message_name) + 2;
	message_string = (char*)malloc(sizeof(char)*message_length);
	if (message_string == NULL)
		return QUEUE_MEM_ALLOC_FAILED;
	sprintf_s(message_string, message_length, "%s\n", message_name);

	// Send the message
	return EnqueueMsg(message_queue, message_string);
}

int SendClientRequestMessage(char* username, message_queue_t* message_queue)
{
	const char* message_name = "CLIENT_REQUEST";
	int message_length;
	char* message_string;

	// Build message string
	message_length = strlen(message_name) + 1 + strlen(username) + 2;
	message_string = (char*)malloc(sizeof(char)*message_length);
	// TODO: Check malloc
	sprintf_s(message_string, message_length, "%s:%s\n", message_name, username);

	// Send the message
	//printf("Sending CLIENT_PLAYER_MOVE.\n");
	return EnqueueMsg(message_queue, message_string);
}


int SendClientCPUMessage(message_queue_t* message_queue)
{
	const char* message_name = "CLIENT_CPU";
	return SendClientMessageWithoutParams(message_name, message_queue);
}

int SendClientVersusMessage(message_queue_t* message_queue)
{
	const char* message_name = "CLIENT_VERSUS";
	return SendClientMessageWithoutParams(message_name, message_queue);
}

int SendClientLeaderBoardMessage(message_queue_t* message_queue)
{
	const char* message_name = "CLIENT_LEADERBOARD";
	return SendClientMessageWithoutParams(message_name, message_queue);
}

int SendClientQuitMessage(message_queue_t* message_queue)
{
	const char* message_name = "CLIENT_DISCONNECT";
	return SendClientMessageWithoutParams(message_name, message_queue);
}

int SendPlayerMoveMessage(MOVE_TYPE player_move, message_queue_t* message_queue)
{
	char* message_name = "CLIENT_PLAYER_MOVE";
	int message_length;
	char* message_string;
	char* player_move_str = "";

	// Parse move
	switch (player_move)
	{
		case (ROCK):
			player_move_str = "ROCK";
			break;
		case (PAPER):
			player_move_str = "PAPER";
			break;
		case(SCISSORS):
			player_move_str = "SCISSORS";
			break;
		case (LIZARD):
			player_move_str = "LIZARD";
			break;
		case (SPOCK):
			player_move_str = "SPOCK";
			break;
		default:
			break;
	}

	// Build message string
	message_length = strlen(message_name) + 1 + strlen(player_move_str) + 2;
	message_string = (char*)malloc(sizeof(char)*message_length);
	// TODO: Check malloc
	sprintf_s(message_string, message_length, "%s:%s\n", message_name, player_move_str);

	// Send the message
	//printf("Sending %s.\n", message_string);
	return EnqueueMsg(message_queue, message_string);
}




int SendClientReplayMessage(message_queue_t* message_queue)
{
	const char* message_name = "CLIENT_REPLAY";
	return SendClientMessageWithoutParams(message_name, message_queue);
}

int SendMainMenuMessage(message_queue_t* message_queue)
{
	const char* message_name = "CLIENT_MAIN_MENU";
	return SendClientMessageWithoutParams(message_name, message_queue);
}

