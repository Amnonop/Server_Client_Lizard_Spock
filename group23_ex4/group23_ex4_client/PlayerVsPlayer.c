#include <WinSock2.h>
#include "PlayerVsPlayer.h"
#include "ClientMessages.h"
#include "ClientGetMessages.h"
#include "Commons.h"
#include "MessageQueue.h"
#include "../Shared/MessageTools.h"

int PlayRound(SOCKET socket, message_queue_t* msg_queue);

int PlayerVsPlayer(SOCKET socket, message_queue_t* msg_queue)
{
	int exit_code;
	BOOL start_game = FALSE;
	char* oponent_name;
	BOOL game_over = FALSE;
	GAME_OVER_MENU_OPTIONS user_choice;
	BOOL replay = FALSE;

	exit_code = SendClientVersusMessage(msg_queue);
	if (exit_code != MSG_SUCCESS)
		return CLIENT_TRNS_FAILED;

	exit_code = GetStartGameMessage(&start_game, &oponent_name, socket);
	if (exit_code != CLIENT_SUCCESS)
	{
		return exit_code;
	}

	if (!start_game)
	{
		printf("No oponents.\n");
	}
	else
	{
		printf("Starting play vs %s", oponent_name);
		exit_code = GetPlayerMoveRequestMessage(socket);
		if (exit_code != CLIENT_SUCCESS)
			return exit_code;

		while (!game_over)
		{
			exit_code = PlayRound(socket, msg_queue);

			// TODO: Wait SERVER_GAME_OVER_MENU
			exit_code = GetGameOverMenuMessage(socket);
			if (exit_code != CLIENT_SUCCESS)
			{
				return exit_code;
			}

			// TODO: Handle client GAME_OVER choice
			user_choice = GetGameOverMenuChoice();
			if (user_choice == OPT_REPLAY)
			{
				exit_code = SendClientReplayMessage(msg_queue);
				if (exit_code != QUEUE_SUCCESS)
				{
					return CLIENT_SEND_MSG_FAILED;
				}

				exit_code = GetReplayStatus(&replay, socket);
				if (exit_code != CLIENT_SUCCESS)
				{
					return exit_code;
				}

				if (!replay)
				{
					printf("%s has left the game!\n", oponent_name);
					game_over = TRUE;
				}
			}
			else
			{
				exit_code = SendMainMenuMessage(msg_queue);
				if (exit_code != QUEUE_SUCCESS)
				{
					return CLIENT_SEND_MSG_FAILED;
				}
				game_over = TRUE;
			}
		}
	}

	return CLIENT_SUCCESS;
}

int PlayRound(SOCKET socket, message_queue_t* msg_queue)
{
	int exit_code;
	char user_move[9];
	MOVE_TYPE player_move;
	game_results_t* game_results = NULL;
	GAME_OVER_MENU_OPTIONS user_choice;

	ShowPlayMoveMenuMessage();
	scanf_s("%s", user_move, (rsize_t)sizeof user_move);
	player_move = translatePlayerMove(user_move);

	// TODO: Send CLIENT_PLAYER_MOVE message
	exit_code = SendPlayerMoveMessage(player_move, msg_queue);
	if (exit_code != QUEUE_SUCCESS)
	{
		return CLIENT_SEND_MSG_FAILED;
	}

	// TODO: Wait SERVER_GAME_RESULTS message
	exit_code = GetGameResultsMessage(socket, &game_results);
	if (exit_code != CLIENT_SUCCESS)
	{
		return exit_code;
	}
	ShowGameResults(game_results);
	FreeGameResults(game_results);

	return CLIENT_SUCCESS;
}