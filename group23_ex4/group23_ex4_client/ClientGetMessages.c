#include <stdlib.h>
#include <WinSock2.h>
#include "ClientGetMessages.h"
#include "../Shared/MessageTools.h"
#include "Commons.h"
#include "../Shared/StringTools.h"
#include "../Shared/ClientSrvCommons.h"

typedef enum
{
	OPONENT_NAME,
	OPONENT_MOVE,
	PLAYER_MOVE,
	WINNER_NAME
} GAME_RESULTS_PARAM;

game_results_t* ParseGameResultsMessage(param_node_t* head);

int GetStartGameMessage(BOOL* start_game, char** oponent_name, SOCKET socket)
{
	int exit_code;
	message_t* message = NULL;

	exit_code = ReceiveMessageWithTimeout(socket, &message, CLIENT_VS_RESPONSE_TIMEOUT);
	if (exit_code != MSG_SUCCESS)
	{
		if (message != NULL)
		{
			free(message);
			return CLIENT_TRNS_FAILED;
		}
	}

	if (STRINGS_ARE_EQUAL(message->message_type, "SERVER_NO_OPPONENTS"))
	{
		*start_game = FALSE;
		return CLIENT_SUCCESS;
	}
	else if (STRINGS_ARE_EQUAL(message->message_type, "SERVER_INVITE"))
	{
		*start_game = TRUE;
		*oponent_name = CopyString(message->parameters);
		if (*oponent_name == NULL)
		{
			free(message);
			return CLIENT_MEM_ALLOC_FAILED;
		}

		return CLIENT_SUCCESS;
	}
	else
	{
		printf("Expected to get SERVER_NO_OPPONENTS or SERVER_INVITE but got %s instead.\n", message->message_type);
		exit_code = CLIENT_UNEXPECTED_MESSAGE;
	}

	free(message);
	return exit_code;
}

int GetGameResultsMessage(SOCKET socket, game_results_t** game_results)
{
	int exit_code;
	message_t* message = NULL;

	printf("Waiting for SERVER_GAME_RESULTS.\n");
	exit_code = ReceiveMessage(socket, &message);
	if (exit_code != MSG_SUCCESS)
	{
		if (message != NULL)
		{
			free(message);
			return CLIENT_RECEIVE_MSG_FAILED;
		}
	}

	if (STRINGS_ARE_EQUAL(message->message_type, "SERVER_GAME_RESULTS"))
	{
		*game_results = ParseGameResultsMessage(message->parameters);
		if (*game_results == NULL)
		{
			exit_code = CLIENT_MSG_PARSE_FAILED;
		}
		else
		{
			exit_code = CLIENT_SUCCESS;
		}
	}
	else
	{
		printf("Expected to get SERVER_GAME_RESULTS but got %s instead.\n", message->message_type);
		exit_code = CLIENT_UNEXPECTED_MESSAGE;
	}

	free(message);
	return exit_code;
}

int GetGameOverMenuMessage(SOCKET socket)
{
	int exit_code;
	message_t* message = NULL;

	exit_code = ReceiveMessage(socket, &message);
	if (exit_code != MSG_SUCCESS)
	{
		if (message != NULL)
		{
			free(message);
			return CLIENT_RECEIVE_MSG_FAILED;
		}
	}

	if (STRINGS_ARE_EQUAL(message->message_type, "SERVER_GAME_OVER_MENU"))
	{
		exit_code = CLIENT_SUCCESS;
	}
	else
	{
		printf("Expected to get SERVER_GAME_OVER_MENU but got %s instead.\n", message->message_type);
		exit_code = CLIENT_UNEXPECTED_MESSAGE;
	}

	free(message);
	return exit_code;
}

game_results_t* ParseGameResultsMessage(param_node_t* head)
{
	int param_index = OPONENT_NAME;
	param_node_t* iter;
	game_results_t* game_results = (game_results_t*)malloc(sizeof(game_results_t));
	if (game_results == NULL)
	{
		printf("Memory allocation failed.\n");
		return NULL;
	}

	iter = head;
	while (iter != NULL)
	{
		switch (param_index)
		{
			case OPONENT_NAME:
				game_results->oponent_name = CopyString(iter->param_value);
				if (game_results->oponent_name == NULL)
				{
					FreeGameResults(game_results);
					return NULL;
				}
				break;
			case OPONENT_MOVE:
				game_results->oponent_move = CopyString(iter->param_value);
				if (game_results->oponent_move == NULL)
				{
					FreeGameResults(game_results);
					return NULL;
				}
				break;
			case PLAYER_MOVE:
				game_results->player_move = CopyString(iter->param_value);
				if (game_results->player_move == NULL)
				{
					FreeGameResults(game_results);
					return NULL;
				}
				break;
			case WINNER_NAME:
				game_results->winner = CopyString(iter->param_value);
				if (game_results->winner == NULL)
				{
					FreeGameResults(game_results);
					return NULL;
				}
				break;
			default:
				printf("Too many parameters in message SERVER_GAME_RESULTS.\n");
				FreeGameResults(game_results);
				return NULL;
				break;
		}

		param_index++;

		iter = iter->next;
	}

	return game_results;
}

void FreeGameResults(game_results_t* game_results)
{
	if (game_results->oponent_name != NULL)
		free(game_results->oponent_name);

	if (game_results->oponent_move != NULL)
		free(game_results->oponent_move);

	if (game_results->player_move != NULL)
		free(game_results->player_move);

	if (game_results->winner != NULL)
		free(game_results->winner);

	free(game_results);
}