#include <WinSock2.h>
#include "ServerMessages.h"
#include "Commons.h"
#include "../Shared/ClientSrvCommons.h"
#include "../Shared/MessageTools.h"
#include "../Shared/socketS.h"

int SendMessageWithoutParams(const char* message_type, SOCKET socket);
char* MoveTypeToString(MOVE_TYPE move);

int SendServerApprovedMessage(SOCKET socket)
{
	const char* message_name = "SERVER_APPROVED";
	return SendMessageWithoutParams(message_name, socket);
}

int SendDeniedMessage(SOCKET socket)
{
	const char* message_name = "SERVER_DENIED";
	return SendMessageWithoutParams(message_name, socket);
}

int SendMainMenuMessage(SOCKET socket)
{
	const char* message_name = "SERVER_MAIN_MENU";
	return SendMessageWithoutParams(message_name, socket);
}

int SendPlayerMoveRequestMessage(SOCKET socket)
{
	const char* message_name = "SERVER_PLAYER_MOVE_REQUEST";
	return SendMessageWithoutParams(message_name, socket);
}

int SendNoOponentsMessage(SOCKET socket)
{
	const char* message_name = "SERVER_NO_OPPONENTS";
	return SendMessageWithoutParams(message_name, socket);
}

int SendServerInviteMessage(char* oponent_name, SOCKET socket)
{
	const char* message_name = "SERVER_INVITE";
	int message_length;
	char* message_string;
	TransferResult_t send_result;

	// Build message string
	message_length = strlen(message_name) + 1 + strlen(oponent_name) + 2;
	message_string = (char*)malloc(sizeof(char)*message_length);
	if (message_string == NULL)
		return SERVER_MEM_ALLOC_FAILED;

	sprintf_s(message_string, message_length, "%s:%s\n",
		message_name,
		oponent_name);

	printf("Sending message: %s\n", message_string);

	send_result = SendString(message_string, socket);
	if (send_result == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		closesocket(socket);
		return SERVER_TRANS_FAILED;
	}

	free(message_string);
	return SERVER_SUCCESS;
}

int SendGameResultsMessage(const char* oponent_username, MOVE_TYPE oponent_move, MOVE_TYPE player_move, const char* winner_name, SOCKET socket)
{
	const char* message_name = "SERVER_GAME_RESULTS";
	int message_length;
	char* message_string;
	TransferResult_t send_result;
	char* oponent_move_str;
	char* player_move_str;

	oponent_move_str = MoveTypeToString(oponent_move);
	player_move_str = MoveTypeToString(player_move);

	// Build message string
	message_length = strlen(message_name) + 1 + strlen(oponent_username) + 1 + strlen(oponent_move_str) + 1 + strlen(player_move_str) + 1 + strlen(winner_name) + 2;
	message_string = (char*)malloc(sizeof(char)*message_length);
	if (message_string == NULL)
		return SERVER_MEM_ALLOC_FAILED;
	if (winner_name != NULL)
	{
		sprintf_s(message_string, message_length, "%s:%s;%s;%s;%s\n",
			message_name,
			oponent_username,
			oponent_move_str,
			player_move_str,
			winner_name);

		printf("Sending message: %s\n", message_string);
	}

	else if (winner_name == NULL)
	{
		sprintf_s(message_string, message_length, "%s:%s;%s;%s;%s\n",
			message_name,
			oponent_username,
			oponent_move_str,
			player_move_str);

		printf("Sending message: %s\n", message_string);
	}
	send_result = SendString(message_string, socket);
	if (send_result == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		closesocket(socket);
		return SERVER_TRANS_FAILED;
	}

	free(message_string);
	return SERVER_SUCCESS;
}




int SendGameOverMenu(SOCKET socket)
{
	const char* message_name = "SERVER_GAME_OVER_MENU";
	return SendMessageWithoutParams(message_name, socket);
}

int SendMessageWithoutParams(const char* message_name, SOCKET socket)
{
	int message_length;
	char* message_string;
	TransferResult_t send_result;

	// Build message string
	message_length = strlen(message_name) + 2;
	message_string = (char*)malloc(sizeof(char)*message_length);
	if (message_string == NULL)
		return SERVER_MEM_ALLOC_FAILED;

	sprintf_s(message_string, message_length, "%s\n", message_name);

	printf("Sending message: %s\n", message_string);

	send_result = SendString(message_string, socket);
	if (send_result == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		closesocket(socket);
		return SERVER_TRANS_FAILED;
	}

	free(message_string);
	return SERVER_SUCCESS;
}