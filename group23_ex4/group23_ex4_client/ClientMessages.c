#include <string.h>
#include <stdlib.h>
#include "ClientMessages.h"
#include "MessageQueue.h"
#include "../Shared/ClientSrvCommons.h"

int SendClientRequestMessage(char* username, message_queue_t* message_queue)
{
	int exit_code;
	char* message_name = "CLIENT_REQUEST";
	int message_length;
	char* message_string;

	// Build message string
	message_length = strlen(message_name) + 1 + strlen(username) + 2;
	message_string = (char*)malloc(sizeof(char)*message_length);
	// TODO: Check malloc
	sprintf_s(message_string, message_length, "%s:%s\n", message_name, username);

	// Send the message
	exit_code = EnqueueMsg(message_queue, message_string);

	return exit_code;
}

int SendClientCPUMessage(message_queue_t* message_queue)
{
	char* message_name = "CLIENT_CPU";
	int message_length;
	char* message_string;

	// Build message string
	message_length = strlen(message_name) + 2;
	message_string = (char*)malloc(sizeof(char)*message_length);
	// TODO: Check malloc
	sprintf_s(message_string, message_length, "%s\n", message_name);

	// Send the message
	return EnqueueMsg(message_queue, message_string);
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
	printf("Sending CLIENT_PLAYER_MOVE.\n");
	return EnqueueMsg(message_queue, message_string);
}