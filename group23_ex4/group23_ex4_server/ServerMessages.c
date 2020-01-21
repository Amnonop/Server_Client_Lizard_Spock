#include <WinSock2.h>
#include "ServerMessages.h"
#include "Commons.h"
#include "../Shared/ClientSrvCommons.h"
#include "../Shared/MessageTools.h"
#include "socketS.h"

int GetPlayerMove(SOCKET client_socket, MOVE_TYPE* player_move)
{
	int exit_code = SERVER_SUCCESS;
	message_t* message = NULL;
	int receive_result;

	receive_result = ReceiveMessage(client_socket, message);
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

int SendGameResultsMessage(const char* oponent_username, MOVE_TYPE oponent_move, MOVE_TYPE player_move, const char* winner_name, SOCKET socket)
{
	return SERVER_SUCCESS;
}

int SendGameOverMenu(SOCKET socket)
{
	return SERVER_SUCCESS;
}

int ReceiveMessage(SOCKET client_socket, message_t* message)
{
	TransferResult_t receive_result;
	char* accepted_string = NULL;

	message = (message_t*)malloc(sizeof(message_t));
	if (message == NULL)
	{
		printf("Failed to allocate memory.\n");
		return SERVER_MEM_ALLOC_FAILED;
	}

	// Waiting for CLIENT_REQUEST message
	receive_result = ReceiveString(&accepted_string, client_socket);
	if (receive_result == TRNS_FAILED)
	{
		printf("Player disconnected. Ending communication.\n");
		return SERVER_TRANS_FAILED;
	}
	else if (receive_result == TRNS_DISCONNECTED)
	{
		printf("Player disconnected. Ending communication.\n");
		return SERVER_TRANS_FAILED;
	}

	printf("Received message: %s\n", accepted_string);
	GetMessageStruct(message, accepted_string);

	free(accepted_string);
	return SERVER_SUCCESS;
}