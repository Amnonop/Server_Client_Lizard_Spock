#include <WinSock2.h>
#include "ServerGetMessages.h"
#include "Commons.h"
#include "../Shared/socketS.h"
#include "../Shared/ClientSrvCommons.h"
#include "../Shared/MessageTools.h"

int ParsePlayerMoveMessage(message_t* message, MOVE_TYPE* player_move);

int GetPlayerMainMenuChoice(SOCKET socket, MAIN_MENU_OPTIONS* user_choice)
{
	int exit_code = SERVER_SUCCESS;
	message_t* message = NULL;

	exit_code = ReceiveMessageWithTimeout(socket, &message, TIMEOUT_INFINITE);
	if (exit_code != MSG_SUCCESS)
	{
		if (message != NULL)
			free(message);
		return SERVER_RECEIVE_MSG_FAILED;
	}

	if (STRINGS_ARE_EQUAL("CLIENT_CPU", message->message_type))
	{
		*user_choice = CLIENT_CPU;
		exit_code = SERVER_SUCCESS;
	}
	else if (STRINGS_ARE_EQUAL(message->message_type, "CLIENT_VERSUS"))
	{
		*user_choice = CLIENT_VERSUS;
		exit_code = SERVER_SUCCESS;
	}

	else if (STRINGS_ARE_EQUAL(message->message_type, "CLIENT_DISCONNECT"))
	{
		*user_choice = QUIT;
		exit_code = SERVER_SUCCESS;
	}

	else if (STRINGS_ARE_EQUAL(message->message_type, "CLIENT_LEADERBOARD"))
	{
		*user_choice = LEADERBOARD;
		exit_code = SERVER_SUCCESS;
	}

	free(message);
	return SERVER_SUCCESS;
}

int GetPlayerGameOverMenuChoice(SOCKET client_socket, GAME_OVER_MENU_OPTIONS* user_choice)
{
	int exit_code = SERVER_SUCCESS;
	message_t* message = NULL;
	int receive_result;

	receive_result = ReceiveMessageWithTimeout(client_socket, &message, TIMEOUT_INFINITE);
	if (receive_result != MSG_SUCCESS)
	{
		if (message != NULL)
			free(message);
		return SERVER_RECEIVE_MSG_FAILED;
	}

	if (STRINGS_ARE_EQUAL(message->message_type, "CLIENT_REPLAY"))
	{
		*user_choice = OPT_REPLAY;
		exit_code = SERVER_SUCCESS;
	}
	else if (STRINGS_ARE_EQUAL(message->message_type, "CLIENT_MAIN_MENU"))
	{
		*user_choice = OPT_MAIN_MENU;
		exit_code = SERVER_SUCCESS;
	}
	else
	{
		printf("Expected to get CLIENT_REPLAY or CLIENT_MAIN_MENU but got %s instead.\n", message->message_type);
		exit_code = SERVER_UNEXPECTED_MESSAGE;
	}

	free(message);
	return exit_code;
}

int GetPlayerMove(SOCKET client_socket, MOVE_TYPE* player_move)
{
	int exit_code = SERVER_SUCCESS;
	message_t* message = NULL;
	int receive_result;

	receive_result = ReceiveMessageWithTimeout(client_socket, &message, TIMEOUT_INFINITE);
	if (receive_result != SERVER_SUCCESS)
	{
		if (message != NULL)
			free(message);
		return receive_result;
	}

	if (STRINGS_ARE_EQUAL(message->message_type, "CLIENT_PLAYER_MOVE"))
	{
		// Get the move of the player from the parameters
		exit_code = ParsePlayerMoveMessage(message, player_move);
	}
	else
	{
		printf("Expected to get CLIENT_PLAYER_MOVE but got %s instead.\n", message->message_type);
		exit_code = SERVER_UNEXPECTED_MESSAGE;
	}

	free(message);
	return exit_code;
}

int ParsePlayerMoveMessage(message_t* message, MOVE_TYPE* player_move)
{
	char* player_move_string;

	player_move_string = message->parameters->param_value;
	if (STRINGS_ARE_EQUAL(player_move_string, "ROCK"))
	{
		*player_move = ROCK;
	}
	else if (STRINGS_ARE_EQUAL(player_move_string, "PAPER"))
	{
		*player_move = PAPER;
	}
	else if (STRINGS_ARE_EQUAL(player_move_string, "SCISSORS"))
	{
		*player_move = SCISSORS;
	}
	else if (STRINGS_ARE_EQUAL(player_move_string, "LIZARD"))
	{
		*player_move = LIZARD;
	}
	else if (STRINGS_ARE_EQUAL(player_move_string, "SPOCK"))
	{
		*player_move = SPOCK;
	}
	else
	{
		printf("Invalid move type %s.\n", player_move_string);
		return SERVER_INVALID_PARAM_VALUE;
	}

	return SERVER_SUCCESS;
}