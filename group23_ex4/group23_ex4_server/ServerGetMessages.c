#include <WinSock2.h>
#include "ServerGetMessages.h"
#include "Commons.h"
#include "../Shared/ClientSrvCommons.h"
#include "../Shared/MessageTools.h"

int GetPlayerGameOverMenuChoice(SOCKET client_socket, GAME_OVER_MENU_OPTIONS* user_choice)
{
	int exit_code = SERVER_SUCCESS;
	message_t* message = NULL;
	int receive_result;

	receive_result = ReceiveMessage(client_socket, &message);
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