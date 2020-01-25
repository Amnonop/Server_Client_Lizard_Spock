#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <Windows.h>
#include "client.h"
#include "others.h"
#include "../Shared/socketS.h"
#include "Commons.h"
#include "MessageQueue.h"
#include "ClientMessages.h"
#include "ClientGetMessages.h"
#include "PlayerVsPlayer.h"
#include "../Shared/ClientSrvCommons.h"
#include "../Shared/MessageTools.h"

//--> Global parameters
SOCKET m_socket; // comuunication socket
message_queue_t* msg_queue = NULL; // pointer to send message buffer

int HandleClientRequest(char* username, SOCKET socket);
int GetMainMenuMessage(SOCKET socket);
int PrintMainMenu();
int Play(SOCKET socket);
int ShowPlayMoveMenuMessage();
MOVE_TYPE ParsePlayerMove(const char* player_move);
int GetPlayerMoveRequestMessage(SOCKET socket);
void ShowGameResults(game_results_t* game_results);
GAME_OVER_MENU_OPTIONS GetGameOverMenuChoice();
int ViewLeaderBoard(SOCKET socket);
int HandleConnection(char* server_ip, int port_number, char* username);

//Sending data to the server Thread
static DWORD SendDataThread(void)
{
	char *new_msg = NULL;
	TransferResult_t SendRes;

	while (1)
	{
		new_msg = DequeueMsg(msg_queue); // Dequeue message from buffer to send to the server

		SendRes = SendString(new_msg, m_socket);

		if (SendRes == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n"); {
				exit(-1);
			}
		}
	}
}

//User application Thread (managing all user inputs to the client)
static DWORD ApplicationThread(LPVOID lpParam)
{
	int exit_code;
	char input[MAX_LINE], *send_message = NULL;
	int run = 0, type = 0;
	char* message_name = NULL;
	client_thread_params_t* thread_params;
	DWORD wait_code;
	BOOL release_res;
	int user_choice = -1;
	int receive_result;

	thread_params = (client_thread_params_t*)lpParam;

	exit_code = HandleClientRequest(thread_params->username, thread_params->socket);
	if (exit_code != CLIENT_SERVER_APPROVED)
	{
		return exit_code;
	}

	while (TRUE)
	{
		// Now we need to wait for SERVER_MAIN_MENU message
		exit_code = GetMainMenuMessage(thread_params->socket);
		if (exit_code != CLIENT_SUCCESS)
		{
			return exit_code;
		}

		PrintMainMenu();
		scanf_s("%d", &user_choice);
		switch (user_choice)
		{
			case CLIENT_CPU:
				//client_state = WAITING_TO_START_GAME;
				message_name = "CLIENT_CPU";
				exit_code = SendClientCPUMessage(msg_queue);
				if (exit_code != MSG_SUCCESS)
					return exit_code;

				exit_code = Play(thread_params->socket);
				if (exit_code != CLIENT_SUCCESS)
					return exit_code;
			break;
			case CLIENT_VERSUS:
				exit_code = PlayerVsPlayer(thread_params->socket, msg_queue);
				if (exit_code != CLIENT_SUCCESS)
				{
					return exit_code;
				}
				break;

			case LEADERBOARD:
				printf("Sorry, this option is not currently supported.\n");
				exit_code = SendLeaderboardMessage(msg_queue);
				if (exit_code != QUEUE_SUCCESS)
				{
					return CLIENT_TRNS_FAILED;
				}
				break;

			case QUIT:
				//client_state = WAITING_TO_START_GAME;
				exit_code = SendClientQuitMessage(msg_queue);
				if (exit_code != MSG_SUCCESS)
					return exit_code;

				//exit_code = Play(thread_params->socket);
				if (exit_code != CLIENT_SUCCESS)
					return exit_code;
				break;

			default:
				break;
		}
	}
	return CLIENT_SUCCESS;
}

int HandleClientRequest(char* username, SOCKET socket)
{
	int exit_code;
	message_t* message = NULL;

	// Inital handshake
	exit_code = SendClientRequestMessage(username, msg_queue);
	if (exit_code != QUEUE_SUCCESS)
	{
		return CLIENT_SEND_MSG_FAILED;
	}

	exit_code = ReceiveMessage(socket, &message);
	if (exit_code != MSG_SUCCESS)
	{
		if (message != NULL)
		{
			free(message);
			return CLIENT_RECEIVE_MSG_FAILED;
		}
	}

	if (STRINGS_ARE_EQUAL(message->message_type, "SERVER_DENIED"))
	{
		exit_code = CLIENT_SERVER_DENIED;
	}
	else if (!STRINGS_ARE_EQUAL(message->message_type, "SERVER_APPROVED"))
	{
		printf("Expected to get SERVER_APPROVED or SERVER_DENIED but got %s instead.\n", message->message_type);
		exit_code = CLIENT_UNEXPECTED_MESSAGE;
	}
	else
	{
		exit_code = CLIENT_SERVER_APPROVED;
	}

	free(message);
	return exit_code;
}

int GetMainMenuMessage(SOCKET socket)
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

	if (STRINGS_ARE_EQUAL(message->message_type, "SERVER_MAIN_MENU"))
	{
		exit_code = CLIENT_SUCCESS;
	}
	else
	{
		printf("Expected to get SERVER_MAIN_MENU but got %s instead.\n", message->message_type);
		exit_code = CLIENT_UNEXPECTED_MESSAGE;
	}

	free(message);
	return exit_code;
}

int translatePlayerMove(char* move)
{
	if ((move[0] == 's') || (move[0] == 'S'))
	{
		if ((move[1] == 'c') || (move[1] == 'C'))
			return SCISSORS;
		else
			return SPOCK;
	}
	else if ((move[0] == 'l') || (move[0] == 'L'))
	{
		return LIZARD;
	}
	else if ((move[0] == 'r') || (move[0] == 'R'))
	{
		return ROCK;
	}
	else if ((move[0] == 'p') || (move[0] == 'P'))
	{
		return PAPER;
	}
}

int Play(SOCKET socket)
{
	int exit_code;
	char user_move[9];
	MOVE_TYPE player_move;
	game_results_t* game_results = NULL;
	GAME_OVER_MENU_OPTIONS user_choice;
	BOOL game_over = FALSE;

	while (!game_over)
	{
		exit_code = GetPlayerMoveRequestMessage(socket);
		if (exit_code != CLIENT_SUCCESS)
			return exit_code;

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

int ViewLeaderBoard(SOCKET socket)
{
	int exit_code;
	char user_move[9];
	MOVE_TYPE player_move = SPOCK;
	game_results_t* game_results = NULL;
	GAME_OVER_MENU_OPTIONS user_choice;
	BOOL game_over = FALSE;
	const char* board_headline = "Name \t Won \t Lost \t W/L Ratio";

	while (!game_over)
	{
		exit_code = GetLeaderBoardMessage(socket);
		if (exit_code != CLIENT_SUCCESS)
			return exit_code;

		user_choice = ShowLeaderBoardMenuMessage();
		if (user_choice == OPT_REPLAY)
		{
			/*exit_code = SendPlayerMoveMessage(msg_queue);
			if (exit_code != QUEUE_SUCCESS)
			{
				return CLIENT_SEND_MSG_FAILED;
			}*/
		}

		else if (user_choice == OPT_MAIN_MENU)
		{
			exit_code = SendMainMenuMessage(msg_queue);
			if (exit_code != QUEUE_SUCCESS)
			{
				return CLIENT_SEND_MSG_FAILED;
			}
			game_over = TRUE;
		}




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

int GetLeaderBoardMessage(SOCKET socket)
{
	int exit_code;
	message_t* message = NULL;

	printf("Waiting for SERVER_LEADERBOARD.\n");
	exit_code = ReceiveMessage(socket, &message);
	if (exit_code != MSG_SUCCESS)
	{
		if (message != NULL)
		{
			free(message);
			return CLIENT_RECEIVE_MSG_FAILED;
		}
	}

	if (STRINGS_ARE_EQUAL(message->message_type, "SERVER_LEADERBOARD"))
	{
		exit_code = CLIENT_SUCCESS;
	}
	else
	{
		printf("Expected to get SERVER_LEADERBOARD but got %s instead.\n", message->message_type);
		exit_code = CLIENT_UNEXPECTED_MESSAGE;
	}

	free(message);
	return exit_code;
}

int GetPlayerMoveRequestMessage(SOCKET socket)
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

	if (STRINGS_ARE_EQUAL(message->message_type, "SERVER_PLAYER_MOVE_REQUEST"))
	{
		exit_code = CLIENT_SUCCESS;
	}
	else
	{
		printf("Expected to get SERVER_PLAYER_MOVE_REQUEST but got %s instead.\n", message->message_type);
		exit_code = CLIENT_UNEXPECTED_MESSAGE;
	}

	free(message);
	return exit_code;
}

int ShowPlayMoveMenuMessage()
{
	printf("Choose a move from the list: Rock, Papar, Scissors, Lizard or Spock:\n");
}

void ShowGameResults(game_results_t* game_results)
{
	printf("You played: %s\n", game_results->player_move);
	printf("%s played: %s\n", game_results->oponent_name, game_results->oponent_move);
	if (!STRINGS_ARE_EQUAL(game_results->player_move, game_results->oponent_move))
	{
		printf("%s won!\n", game_results->winner);
	}
}

GAME_OVER_MENU_OPTIONS GetGameOverMenuChoice()
{
	GAME_OVER_MENU_OPTIONS user_choice;

	printf("Choose what to do next:\n");
	printf("1.	Play again\n");
	printf("2.	Return to the main menu\n");

	scanf_s("%d", &user_choice);

	return user_choice;
}

GAME_OVER_MENU_OPTIONS ShowLeaderBoardMenuMessage()
{
	GAME_OVER_MENU_OPTIONS user_choice;

	printf("Choose what to do next:\n");
	printf("1.	Refresh\n");
	printf("2.	Return to the main menu\n");

	scanf_s("%d", &user_choice);

	return user_choice;
}

MOVE_TYPE ParsePlayerMove(const char* player_move)
{
	if (STRINGS_ARE_EQUAL(player_move, "ROCK"))
		return ROCK;
	else if (STRINGS_ARE_EQUAL(player_move, "PAPER"))
		return PAPER;
	else if (STRINGS_ARE_EQUAL(player_move, "SCISSORS"))
		return SCISSORS;
	else if (STRINGS_ARE_EQUAL(player_move, "LIZARD"))
		return LIZARD;
	else if (STRINGS_ARE_EQUAL(player_move, "SPOCK"))
		return SPOCK;
	else
	{
		printf("Wrong input, exiting.\n");
		return (-1);
	}
}

//Main thread that inisilatize all the sockets, programs and opens the send, recive and API threads
int MainClient(char* server_ip, int port_number, char* username)
{
	int exit_code;
	SOCKADDR_IN client_service;
	HANDLE hThread[2];
	WSADATA wsaData; 
	int startup_result;
	int connect_result;
	SERVER_CONNECT_MENU_OPTIONS user_choice;

	client_thread_params_t thread_params;

	// Initialize the message queue
	msg_queue = CreateMessageQueue();
	if (msg_queue == NULL) 
	{
		printf("Error initializing the message queue.\n");
		return CLIENT_MSG_QUEUE_INIT_FAILED;
	}
	
	startup_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (startup_result != NO_ERROR)
	{
		printf("Error at WSAStartup()\n");
		// TODO: Free message queue, Send Data thread handler
		return CLIENT_WSA_STARTUP_ERROR;
	}

	exit_code = HandleConnection(server_ip, port_number, username);

	//---> Closing handles and free memory

	CloseHandle(hThread[0]); // SendDataThread
	CloseHandle(hThread[1]); // ApplicationThread

	WSACleanup();

	CloseHandle(msg_queue->access_mutex); // Message queue access_mutex
	CloseHandle(msg_queue->msgs_count_semaphore); // Message queue msgs_count_semaphore
	CloseHandle(msg_queue->queue_empty_event); // Message queue queue_empty_event
	CloseHandle(msg_queue->stop_event); // Message queue stop_event
	free(msg_queue); // Free Message queue

	return exit_code;
}

int HandleConnection(char* server_ip, int port_number, char* username)
{
	int exit_code;
	SOCKADDR_IN client_service;
	HANDLE hThread[2];
	int connect_result;
	SERVER_CONNECT_MENU_OPTIONS user_choice;

	client_thread_params_t thread_params;

	while (TRUE)
	{
		m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_socket == INVALID_SOCKET) {
			printf("Error at socket(): %ld\n", WSAGetLastError());
			WSACleanup();
			return CLIENT_SOCKET_CREATION_FAILED;
		}

		client_service.sin_family = AF_INET;
		client_service.sin_addr.s_addr = inet_addr(server_ip);
		client_service.sin_port = htons(port_number);

		// Connection to server
		while (TRUE)
		{
			connect_result = connect(m_socket, (SOCKADDR*)&client_service, sizeof(client_service));
			if (connect_result == SOCKET_ERROR)
			{
				printf("Failed connecting to server on %s:%d\n", server_ip, port_number);
				printf("Choose what to do next:\n");
				printf("1.	Try to reconnect\n");
				printf("2.	Exit\n");

				scanf_s("%d", &user_choice);
				if (user_choice == OPT_EXIT)
				{
					// TODO: Cleanup
					return CLIENT_SUCCESS; // User chose to exit
				}
			}
			else
			{
				printf("Connected to server on %s:%d\n", server_ip, port_number);
				break;
			}
		}

		thread_params.username = username;
		thread_params.server_ip = server_ip;
		thread_params.server_port = port_number;
		thread_params.socket = m_socket;

		// Start the Send Data thread
		hThread[0] = CreateThread(
			NULL,
			0,
			(LPTHREAD_START_ROUTINE)SendDataThread,
			NULL,
			0,
			NULL
		);
		if (hThread[0] == NULL)
		{
			// TODO: Free message queue
			return CLIENT_THREAD_CREATION_FAILED;
		}

		hThread[1] = CreateThread(
			NULL,
			0,
			(LPTHREAD_START_ROUTINE)ApplicationThread,
			(LPVOID)&thread_params,
			0,
			NULL
		);

		WaitForMultipleObjects(2, hThread, FALSE, INFINITE);

		GetExitCodeThread(hThread[0], &exit_code); // Getting the error code from the application thread
		if (exit_code == CLIENT_SERVER_DENIED)
		{
			closesocket(m_socket);

			printf("Server on %s:%d denied the connection request.\n", server_ip, port_number);
		}
		else if (exit_code == CLIENT_TRNS_FAILED)
		{
			closesocket(m_socket);

			printf("Connection to server on %s:%d has been lost.\n", server_ip, port_number);
		}
		else
		{
			closesocket(m_socket);
			return exit_code;
		}

		printf("Choose what to do next:\n");
		printf("1.	Try to reconnect\n");
		printf("2.	Exit\n");

		scanf_s("%d", &user_choice);
		if (user_choice == OPT_EXIT)
		{
			// TODO: Cleanup
			return CLIENT_SUCCESS; // User chose to exit
		}
	}
}

int PrintMainMenu()
{
	printf("Choose what to do next:\n");
	printf("1.	Play against another client\n");
	printf("2.	Play against the server\n");
	printf("3.	View the leaderboard\n");
	printf("4.	Quit\n");

	return 0;
}