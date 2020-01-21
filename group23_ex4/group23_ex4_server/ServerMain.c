#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <WinSock2.h>
#include <stdbool.h>
#include "ServerMain.h"
#include "Commons.h"
#include "../Shared/MessageTools.h"
#include "socketS.h"
#include "../Shared/ClientSrvCommons.h"
#include "ServerMessages.h"

#define SERVER_ADDRESS_STR "127.0.0.1"
#define NUM_OF_WORKER_THREADS 2
#define CLIENTS_MAX_NUM 2

typedef struct client_params
{
	int client_number;
} client_params_t;

HANDLE client_thread_handles[NUM_OF_WORKER_THREADS];
client_info_t connected_clients[CLIENTS_MAX_NUM];
client_params_t client_params[CLIENTS_MAX_NUM];
TransferResult_t SendRes;
static DWORD ClientThread(LPVOID thread_params);
MOVE_TYPE GetComputerMove();
int AcceptPlayer(SOCKET client_socket);
int CheckWinner(MOVE_TYPE player_a_move, MOVE_TYPE player_b_move);

int SendServerApprovedMessage(SOCKET player)
{
	char* message_name = "SERVER_APPROVED";
	int message_length;
	char* message_string;

	// Build message string
	message_length = strlen(message_name) + 2;
	message_string = (char*)malloc(sizeof(char)*message_length);
	// TODO: Check malloc
	sprintf_s(message_string, message_length, "%s\n", message_name);

	printf("Sending message: %s\n", message_string);

	SendRes = SendString(message_name, player);
	if (SendRes == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		closesocket(player);
	}
}

int SendServerMenuMessage(SOCKET player)
{
	char* message_name = "SERVER_MAIN_MENU";
	int message_length;
	char* message_string;

	// Build message string
	message_length = strlen(message_name) + 2;
	message_string = (char*)malloc(sizeof(char)*message_length);
	// TODO: Check malloc
	sprintf_s(message_string, message_length, "%s\n", message_name);

	printf("Sending message: %s\n", message_string);

	SendRes = SendString(message_name, player);
	if (SendRes == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		closesocket(player);
	}
}

int RunServer(int port_number)
{
	WSADATA wsa_data;
	int startup_result;
	SOCKET server_socket = INVALID_SOCKET;
	unsigned long address;
	SOCKADDR_IN service;
	int bind_result;
	int listen_result;
	srand(42);
	int thread_index;
	int client_thread_count;
	int connected_clients_count = 0;

	SOCKET accept_socket;

	// Initialize Winsock
	startup_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (startup_result != NO_ERROR)
	{
		printf("WSAStartup error %ld\n", WSAGetLastError());
		return SERVER_WSA_STARTUP_ERROR;
	}

	// Create a socket
	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == INVALID_SOCKET)
	{
		printf("Error creating a socket: %ld\n", WSAGetLastError());
		if (WSACleanup() == SOCKET_ERROR)
		{
			printf("Failed to close Winsocket: %ld\n", WSAGetLastError());
		}
		return SERVER_SOCKET_CREATION_FAILED;
	}

	// Bind the socket
	address = inet_addr(SERVER_ADDRESS_STR);
	if (address == INADDR_NONE)
	{
		printf("The string \"%s\" is not valid ip string.\n", SERVER_ADDRESS_STR);
		if (closesocket(server_socket) == SOCKET_ERROR)
		{
			printf("Failed to close main socket: %ld\n", WSAGetLastError());
		}
		if (WSACleanup() == SOCKET_ERROR)
		{
			printf("Failed to close Winsocket: %ld\n", WSAGetLastError());
		}
		return SERVER_INVALID_IP_ADDRESS;
	}
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = address;
	service.sin_port = htons(port_number);

	// Bind socket
	bind_result = bind(server_socket, (SOCKADDR*)&service, sizeof(service));
	if (bind_result == SOCKET_ERROR)
	{
		printf("Failed to bind socket: %ld\n", WSAGetLastError());
		if (closesocket(server_socket) == SOCKET_ERROR)
		{
			printf("Failed to close main socket: %ld\n", WSAGetLastError());
		}
		if (WSACleanup() == SOCKET_ERROR)
		{
			printf("Failed to close Winsocket: %ld\n", WSAGetLastError());
		}
		return SERVER_SOCKET_BINDING_FAILED;
	}

	listen_result = listen(server_socket, SOMAXCONN);
	if (listen_result == SOCKET_ERROR)
	{
		printf("Failed to listen on socket: %ld\n", WSAGetLastError());
		if (closesocket(server_socket) == SOCKET_ERROR)
		{
			printf("Failed to close main socket: %ld\n", WSAGetLastError());
		}
		if (WSACleanup() == SOCKET_ERROR)
		{
			printf("Failed to close Winsocket: %ld\n", WSAGetLastError());
		}
		return SERVER_SOCKET_LISTEN_FAILED;
	}

	for (client_thread_count = 0; client_thread_count < NUM_OF_WORKER_THREADS; client_thread_count++)
		client_thread_handles[client_thread_count] = NULL;

	printf("Waiting for a client to connect...\n");

	while (TRUE)
	{
		accept_socket = accept(server_socket, NULL, NULL);
		if (accept_socket == INVALID_SOCKET)
		{
			printf("Accepting connection with client failed: %ld\n", WSAGetLastError());
			// Clenup
			return SERVER_ACCEPT_CONNECTION_FAILED;
		}

		printf("Client connected.\n");

		if (connected_clients_count < CLIENTS_MAX_NUM)
		{
			connected_clients[connected_clients_count].socket = accept_socket;

			// Open a thread for the client
			client_params[connected_clients_count].client_number = connected_clients_count;
			client_thread_handles[connected_clients_count] = CreateThread(
				NULL, 
				0, 
				(LPTHREAD_START_ROUTINE)ClientThread, 
				(LPVOID)&(client_params[connected_clients_count]), 
				0, 
				NULL);
			// TODO: Check if null

			connected_clients_count++;
		}
	}

	return SERVER_SUCCESS;
}

static DWORD ClientThread(LPVOID thread_params)
{
	//char SendStr[SEND_STR_SIZE];
	BOOL Done = FALSE;
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	int found_place;
	int exit_code;
	//char str_to_send[MAX_MESSAGE_SIZE];

	client_params_t* client_params;
	message_t* message = NULL;
	char* accepted_string = NULL;

	client_params = (client_params_t*)thread_params;
	
	exit_code = AcceptPlayer(connected_clients[client_params->client_number].socket);
	if (exit_code != SERVER_SUCCESS)
	{
		// TODO: Terminate the thread for this client
		return exit_code;
	}
		
	while (TRUE)
	{
		// waiting for new message
		accepted_string = NULL;
		RecvRes = ReceiveString(&accepted_string, connected_clients[client_params->client_number].socket);
		if (RecvRes == TRNS_FAILED) {
			printf("Player disconnected. Ending communication.\n");
			//closesocket(players_sockets[player_number]);
			//SetEvent(DiscconectThem);
			return -1;
		}
		else if (RecvRes == TRNS_DISCONNECTED) {
			printf("Player disconnected. Ending communication.\n");
			//closesocket(players_sockets[player_number]);
			//SetEvent(DiscconectThem);
			return -1;
		}

		// breake message into message type and it's other parts
		printf("Received message: %s\n", accepted_string);
		GetMessageStruct(message, accepted_string);
		int move = GetComputerMove();
		if (STRINGS_ARE_EQUAL("CLIENT_CPU", message->message_type))
		{
			int move = GetComputerMove();
			SendServerMoveMessage(connected_clients[client_params->client_number].socket);
		}
	}
	
	free(message);
}

int AcceptPlayer(SOCKET client_socket)
{
	message_t* message = NULL;
	int receive_result;

	receive_result = ReceiveMessage(client_socket, message);
	if (receive_result != SERVER_SUCCESS)
	{
		if (message != NULL)
			free(message);
		return receive_result;
	}

	if (STRINGS_ARE_EQUAL(message->message_type, "CLIENT_REQUEST"))
	{
		SendServerApprovedMessage(connected_clients[client_params->client_number].socket);
		SendServerMenuMessage(connected_clients[client_params->client_number].socket);
	}

	free(message);
	return SERVER_SUCCESS;
}

int HandlePlayer(SOCKET client_socket)
{
	message_t* message = NULL;
	int receive_result;
	int exit_code = SERVER_SUCCESS;

	receive_result = ReceiveMessage(client_socket, message);
	if (receive_result != SERVER_SUCCESS)
	{
		if (message != NULL)
			free(message);
		return receive_result;
	}

	if (STRINGS_ARE_EQUAL("CLIENT_CPU", message->message_type))
	{
		exit_code = Play(client_socket);
	}

	free(message);
	return SERVER_SUCCESS;
}

int Play(SOCKET client_socket)
{
	int exit_code = SERVER_SUCCESS;
	BOOL end_game = FALSE;
	MOVE_TYPE computer_move;
	MOVE_TYPE player_move;
	int winner;
	CLIENT_MENU_OPTIONS client_choice = CLIENT_CPU;

	while (!end_game)
	{
		computer_move = GetComputerMove();

		exit_code = SendPlayerMoveRequestMessage(client_socket);
		if (exit_code != SERVER_SUCCESS)
			return exit_code;

		// Wait for the player's move CLIENT_PLAYER_MOVE
		exit_code = GetPlayerMove(client_socket, &player_move);
		if (exit_code != SERVER_SUCCESS)
		{
			return exit_code;
		}

		// Send SERVER_GAME_RESULTS
		winner = CheckWinner(player_move, computer_move);
		if (winner == 1)
			exit_code = SendGameResultsMessage("Server", computer_move, player_move, "", client_socket); // TODO: Send player username
		else
			exit_code = SendGameResultsMessage("Server", computer_move, player_move, "Server", client_socket);
		
		if (exit_code != SERVER_SUCCESS)
			return exit_code;

		// Wait for client's choice 
		exit_code = SendGameOverMenu(client_socket);
		if (exit_code != SERVER_SUCCESS)
			return exit_code;

		if (client_choice != CLIENT_CPU)
			end_game = TRUE;
	}
}

int CheckWinner(MOVE_TYPE player_a_move, MOVE_TYPE player_b_move)
{
	int exit_code = SERVER_SUCCESS;

	return exit_code;
}

MOVE_TYPE GetComputerMove()
{
	return rand() % 5;
}