#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <WinSock2.h>
#include <stdbool.h>
#include "ServerMain.h"
#include "Commons.h"
#include "../Shared/MessageTools.h"
#include "../Shared/ClientSrvCommons.h"
#include "ServerMessages.h"
#include "ServerGetMessages.h"
#include "../Shared/socketS.h"
#include <Windows.h>
#include "GameSession.h"

#define SERVER_ADDRESS_STR "127.0.0.1"
#define NUM_OF_WORKER_THREADS 2
#define CLIENTS_MAX_NUM 2

static char* GAME_SESSION_MUTEX_NAME = "game_session_mutex";
static char* GAME_SESSION_FILENAME = "GameSession.txt";

typedef struct client_params
{
	int client_number;
} client_params_t;

HANDLE client_thread_handles[NUM_OF_WORKER_THREADS];
client_info_t* connected_clients[CLIENTS_MAX_NUM];
client_params_t client_params[CLIENTS_MAX_NUM];
TransferResult_t SendRes;
HANDLE game_session_mutex;
HANDLE session_owner_write_event;
HANDLE oponent_write_event;
HANDLE read_log_file_semaphore_handles;
HANDLE write_log_file_mutex_handle;

int GetAvailableClientId();
static DWORD ClientThread(LPVOID thread_params);
MOVE_TYPE GetComputerMove();
int AcceptPlayer(client_info_t* client);
int CheckWinner(MOVE_TYPE player_a_move, MOVE_TYPE player_b_move);
int HandlePlayer(client_info_t* client);
int SaveUsername(const char* username, client_info_t* client);
int Play(client_info_t* client);
int ClientDisconnected(int client_id);
int FindOponent(int client_id);
int OpenGameSession(HANDLE game_session_mutex);
int PlayRoundVsPlayer(client_info_t* client, int is_session_owner, const char* oponent_name);
int UpdatePlayerMove(const char* game_session_path, const char* username, MOVE_TYPE move, HANDLE write_event);
int GetOponentMove(const char* game_session_path, const char* opponent_name, MOVE_TYPE* opponent_move, HANDLE write_event);

int RunServer(int port_number)
{
	int exit_code;
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
	int client_id;

	SOCKET accept_socket;

	game_session_mutex = CreateMutex(
		NULL,	/* default security attributes */
		FALSE,	/* initially not owned */
		GAME_SESSION_MUTEX_NAME);	/* unnamed mutex */
	if (game_session_mutex == NULL)
	{
		printf("Error creating Game Session Mutex.\n");
		return SERVER_CREATE_MUTEX_FAILED;
	}

	session_owner_write_event = CreateEvent(
		NULL,               // default security attributes
		FALSE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		TEXT("session_owner_write_event")  // object name
	);
	if (session_owner_write_event == NULL)
	{
		if (!CloseHandle(game_session_mutex))
		{

		}
		return SERVER_CREATE_EVENT_FAILED;
	}

	oponent_write_event = CreateEvent(
		NULL,               // default security attributes
		FALSE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		TEXT("session_owner_write_event")  // object name
	);
	if (oponent_write_event == NULL)
	{
		if (!CloseHandle(game_session_mutex))
		{

		}

		if (!CloseHandle(session_owner_write_event))
		{

		}

		return SERVER_CREATE_EVENT_FAILED;
	}

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

		client_id = GetAvailableClientId();
		if (client_id == -1)
		{
			// Send SERVER_DENIED message to the client
			printf("Sending SERVER_DENIED message.\n");
			exit_code = SendDeniedMessage(accept_socket);
			if (exit_code != SERVER_SUCCESS)
			{
				// TODO: Cleanup
				return exit_code;
			}
		}
		else
		{
			// Open a thread for the client
			connected_clients[client_id] = malloc(sizeof(client_info_t));
			if (connected_clients[client_id] == NULL)
			{
				printf("Memory allocation failed.\n");
				// TODO: Cleanup
				return SERVER_MEM_ALLOC_FAILED;
			}
			connected_clients[client_id]->socket = accept_socket;
			connected_clients[client_id]->client_id = client_id;
			connected_clients[client_id]->request_to_play = FALSE;

			// Open a thread for the client
			client_params[client_id].client_number = client_id;
			
			client_thread_handles[client_id] = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)ClientThread,
				(LPVOID)&(client_params[client_id]),
				0,
				NULL);
			if (client_thread_handles[client_id] == NULL)
			{
				printf("Failed to create a thread for a new client.\n");
				// TODO: Cleanup
				return SERVER_THREAD_CREATION_FAILED;
			}

			connected_clients_count++;
		}
		//if (connected_clients_count < CLIENTS_MAX_NUM)
		//{
		//	connected_clients[connected_clients_count].socket = accept_socket;

		//	// Open a thread for the client
		//	client_params[connected_clients_count].client_number = connected_clients_count;
		//	client_thread_handles[connected_clients_count] = CreateThread(
		//		NULL, 
		//		0, 
		//		(LPTHREAD_START_ROUTINE)ClientThread, 
		//		(LPVOID)&(client_params[connected_clients_count]), 
		//		0, 
		//		NULL);
		//	// TODO: Check if null

		//	connected_clients_count++;
		//}
	}

	return SERVER_SUCCESS;
}

int GetAvailableClientId()
{
	int i = 0;

	for (i = 0; i < CLIENTS_MAX_NUM; i++)
	{
		if (connected_clients[i] == NULL)
		{
			return i;
		}
	}

	return -1;
}

int translatePlayerMove(char* move)
{
	if ((move[0] == "s") || (move[0] == "S"))
	{
		if ((move[1] == "c") || (move[1] == "C"))
			return SCISSORS;
		else
			return SPOCK;
	}
	else if ((move[0] == "l") || (move[0] == "L"))
	{
		return LIZARD;
	}
	else if ((move[0] == "r") || (move[0] == "R"))
	{
		return ROCK;
	}
	else if ((move[0] == "p") || (move[0] == "P"))
	{
		return PAPER;
	}
}
int computeWinner(int first_player_move, int  sec_player_move)
{
	//tie//
	if (first_player_move = sec_player_move)
		return 0;
	
	else if (first_player_move == ROCK)
	{
		if ((sec_player_move == SCISSORS) || (sec_player_move == LIZARD))
			return 1;
		else return 2;
	}
	else if (first_player_move == PAPER)
	{
		if ((sec_player_move == ROCK) || (sec_player_move == SPOCK))
			return 1;
		else return 2;
	}

	else if (first_player_move == SCISSORS)
	{
		if ((sec_player_move == PAPER) || (sec_player_move == LIZARD))
			return 1;
		else return 2;
	}

	else if (first_player_move == LIZARD)
	{
		if ((sec_player_move == PAPER) || (sec_player_move == SPOCK))
			return 1;
		else return 2;
	}

	else if (first_player_move == SPOCK)
	{
		if ((sec_player_move == ROCK) || (sec_player_move == SCISSORS))
			return 1;
		else return 2;
	}
	else
	{
		return 0;
	}
}

static DWORD ClientThread(LPVOID thread_params)
{
	int exit_code;
	client_params_t* client_params;

	client_params = (client_params_t*)thread_params;
	
	exit_code = AcceptPlayer(connected_clients[client_params->client_number]);
	if (exit_code != SERVER_SUCCESS)
	{
		// TODO: Terminate the thread for this client
		return exit_code;
	}
		
	while (TRUE)
	{
		exit_code = HandlePlayer(connected_clients[client_params->client_number]);
		if (exit_code == SERVER_CLIENT_DISCONNECTED)
		{
			return ClientDisconnected(client_params->client_number);
		}
		else if (exit_code != SERVER_SUCCESS)
		{
			return exit_code;
		}
		//// waiting for new message
		//accepted_string = NULL;
		//RecvRes = ReceiveString(&accepted_string, connected_clients[client_params->client_number].socket);
		//if (RecvRes == TRNS_FAILED) {
		//	printf("Player disconnected. Ending communication.\n");
		//	//closesocket(players_sockets[player_number]);
		//	//SetEvent(DiscconectThem);
		//	return -1;
		//}
		//else if (RecvRes == TRNS_DISCONNECTED) {
		//	printf("Player disconnected. Ending communication.\n");
		//	//closesocket(players_sockets[player_number]);
		//	//SetEvent(DiscconectThem);
		//	return -1;
		//}

		//// breake message into message type and it's other parts
		//printf("Received message: %s\n", accepted_string);
		//GetMessageStruct(message, accepted_string);
		//int move = GetComputerMove();
		//if (STRINGS_ARE_EQUAL("CLIENT_CPU", message->message_type))
		//{
		//	int move = GetComputerMove();
		//	SendServerMoveMessage(connected_clients[client_params->client_number].socket);
		//}
	}
}

int AcceptPlayer(client_info_t* client)
{
	int exit_code = SERVER_SUCCESS;
	message_t* message = NULL;
	int receive_result;

	receive_result = ReceiveMessage(client->socket, &message);
	if (receive_result != MSG_SUCCESS)
	{
		if (message != NULL)
			free(message);
		return receive_result;
	}

	if (STRINGS_ARE_EQUAL(message->message_type, "CLIENT_REQUEST"))
	{
		// TODO: Save the user's name
		SaveUsername(message->parameters->param_value, client);

		exit_code = SendServerApprovedMessage(client->socket);
	}
	else
	{
		printf("Expected to get CLIENT_REQUEST but got %s instead.\n", message->message_type);
		exit_code = SERVER_UNEXPECTED_MESSAGE;
	}

	free(message);
	return exit_code;
}

int SaveUsername(const char* username, client_info_t* client)
{
	int username_length;

	username_length = strlen(username) + 1;
	strcpy_s(client->userinfo, username_length, username);
}

int PlayerVersusPlayer(client_info_t* client)
{
	BOOL end_game = FALSE;
	BOOL other_player_status = FALSE;
	int exit_code;
	MOVE_TYPE player_0_move;
	MOVE_TYPE player_1_move;
	BOOL curr_client_id = client->client_id;
	int winner;
	GAME_OVER_MENU_OPTIONS user_0_choice = OPT_REPLAY;
	GAME_OVER_MENU_OPTIONS user_1_choice = OPT_REPLAY;
	int oponent_id = -1;
	BOOL session_owner = FALSE;

	oponent_id = FindOponent(client->client_id);
	if (oponent_id == -1)
	{
		return SendNoOponentsMessage(client->socket);
	}
	else
	{
		// Send INVITE_SERVER
		exit_code = SendServerInviteMessage(connected_clients[oponent_id]->userinfo, client->socket);
		if (exit_code != SERVER_SUCCESS)
		{
			return exit_code;
		}

		// Open game session
		exit_code = OpenGameSession(game_session_mutex);
		if (exit_code == SERVER_SUCCESS)
		{
			session_owner = TRUE;
		}
		else if (exit_code == SERVER_FILE_EXISTS)
		{
			session_owner = FALSE;
		}
		else
		{
			return exit_code;
		}
		write_log_file_mutex_handle = CreateMutex(
			NULL,
			FALSE,
			"log_file_mutex_handle");
		if (write_log_file_mutex_handle == NULL)
		{
			CloseHandle(write_log_file_mutex_handle);
			printf("Error writing to log file\n");
				
			//return HM_MUTEX_CREATE_FAILED;
		}

		read_log_file_semaphore_handles = CreateSemaphore(
			NULL,												/* Default security attributes */
			0,			/* Initial Count - the room is empty */
			1,			/* Maximum Count */
			NULL);												/* un-named */

		if (read_log_file_semaphore_handles == NULL)
		{
			CloseHandle(read_log_file_semaphore_handles);
			printf("Error reading log file\n");
		}
	}
}

int PlayRoundVsPlayer(client_info_t* client, int is_session_owner, const char* opponent_name)
{
	int exit_code;
	MOVE_TYPE player_move = SPOCK;
	MOVE_TYPE opponent_move = SPOCK;
	int winner;
	DWORD wait_result;

	exit_code = SendPlayerMoveRequestMessage(client->socket);
	if (exit_code != SERVER_SUCCESS)
		return exit_code;

	// Wait for the player's move CLIENT_PLAYER_MOVE
	exit_code = GetPlayerMove(client->socket, &player_move);
	if (exit_code != SERVER_SUCCESS)
	{
		return exit_code;
	}

	// TODO: Write move to file and signal write event
	if (is_session_owner)
	{
		exit_code = UpdatePlayerMove(GAME_SESSION_FILENAME, client->userinfo, player_move, session_owner_write_event);
		if (exit_code != SERVER_SUCCESS)
		{
			return exit_code;
		}

		exit_code = GetOponentMove(GAME_SESSION_FILENAME, opponent_name, &opponent_move, session_owner_write_event);
		if (exit_code != SERVER_SUCCESS)
		{
			return exit_code;
		}
	}
	else
	{
		// We are not the session owner, so first we wait the read and then we write
		exit_code = GetOponentMove(GAME_SESSION_FILENAME, opponent_name, &opponent_move, session_owner_write_event);
		if (exit_code != SERVER_SUCCESS)
		{
			return exit_code;
		}

		exit_code = UpdatePlayerMove(GAME_SESSION_FILENAME, client->userinfo, player_move, session_owner_write_event);
		if (exit_code != SERVER_SUCCESS)
		{
			return exit_code;
		}
	}

	// Send SERVER_GAME_RESULTS
	winner = CheckWinner(player_move, opponent_move);
	if (winner == 1)
		exit_code = SendGameResultsMessage(opponent_name, opponent_move, player_move, client->userinfo, client->socket);
	else
		exit_code = SendGameResultsMessage(opponent_name, opponent_move, player_move, opponent_name, client->socket);

	if (exit_code != SERVER_SUCCESS)
		return exit_code;

	if (is_session_owner)
	{
		// Delete the game session file
		exit_code = RemoveFile(GAME_SESSION_FILENAME);
		if (exit_code != SERVER_SUCCESS)
		{
			return exit_code;
		}
	}

	return SERVER_SUCCESS;
}

int UpdatePlayerMove(const char* game_session_path, const char* username, MOVE_TYPE move, HANDLE write_event)
{
	int exit_code;

	exit_code = WriteMoveToGameSession(game_session_path, move, username);
	if (exit_code != SERVER_SUCCESS)
	{
		return exit_code;
	}

	if (!SetEvent(write_event))
	{
		printf("SetEvent failed (%d)\n", GetLastError());
		return SERVER_SET_EVENT_FAILED;
	}

	return SERVER_SUCCESS;
}

int GetOponentMove(const char* game_session_path, const char* opponent_name, MOVE_TYPE* opponent_move, HANDLE write_event)
{
	DWORD wait_result;
	int exit_code;

	printf("Waiting for %s to write move...\n", opponent_name);
	wait_result = WaitForSingleObject(write_event, INFINITE);
	if (wait_result != WAIT_OBJECT_0)
	{
		return SERVER_WAIT_FOR_EVENT_FAILED;
	}

	printf("Reading %s's move...\n", opponent_name);
	exit_code = ReadOponnentMoveFromGameSession(GAME_SESSION_FILENAME, opponent_name, opponent_move);
	if (exit_code != SERVER_SUCCESS)
	{
		return exit_code;
	}
}

int OpenGameSession(HANDLE game_session_mutex)
{
	int exit_code;
	DWORD wait_result;

	wait_result = WaitForSingleObject(game_session_mutex, INFINITE);
	if (wait_result != WAIT_OBJECT_0)
	{
		return SERVER_ACQUIRE_MUTEX_FAILED;
	}

	// Open game session
	exit_code = OpenNewFile(GAME_SESSION_FILENAME);

	if (!ReleaseMutex(game_session_mutex))
	{
		return SERVER_MUTEX_RELEASE_FAILED;
	}

	return exit_code;
}

int FindOponent(int client_id)
{
	int oponent_id;

	oponent_id = 1 - client_id;

	printf("Waiting to another player to connect...\n");
	Sleep(15000);

	if (connected_clients[oponent_id] != NULL)
	{
		if (connected_clients[oponent_id]->request_to_play)
			return oponent_id;
	}

	return -1;
}

int HandlePlayer(client_info_t* client)
{
	message_t* message = NULL;
	int receive_result;
	int exit_code = SERVER_SUCCESS;
	MAIN_MENU_OPTIONS user_choice;

	while (TRUE)
	{
		exit_code = SendMainMenuMessage(client->socket);
		if (exit_code != SERVER_SUCCESS)
		{
			return exit_code;
		}

		exit_code = GetPlayerMainMenuChoice(client->socket, &user_choice);
		if (exit_code != SERVER_SUCCESS)
		{
			return exit_code;
		}

		switch (user_choice)
		{
			case CLIENT_CPU:
				exit_code = Play(client);
				break;
			case CLIENT_VERSUS:
				client->request_to_play = TRUE;
				exit_code = PlayerVersusPlayer(client);
				break;
			case QUIT:
				return SERVER_CLIENT_DISCONNECTED;
				break;
			default:
				break;
		}

		if (exit_code != SERVER_SUCCESS)
		{
			return exit_code;
		}
	}
}

int Play(client_info_t* client)
{
	int exit_code = SERVER_SUCCESS;
	BOOL end_game = FALSE;
	MOVE_TYPE computer_move;
	MOVE_TYPE player_move;
	int winner;
	GAME_OVER_MENU_OPTIONS user_choice = OPT_REPLAY;

	while (!end_game)
	{
		computer_move = GetComputerMove();

		exit_code = SendPlayerMoveRequestMessage(client->socket);
		if (exit_code != SERVER_SUCCESS)
			return exit_code;

		// Wait for the player's move CLIENT_PLAYER_MOVE
		exit_code = GetPlayerMove(client->socket, &player_move);
		if (exit_code != SERVER_SUCCESS)
		{
			return exit_code;
		}

		// Send SERVER_GAME_RESULTS
		winner = CheckWinner(player_move, computer_move);
		if (winner == 1)
			exit_code = SendGameResultsMessage("Server", computer_move, player_move, client->userinfo, client->socket);
		// TODO: Send player username
		else
			exit_code = SendGameResultsMessage("Server", computer_move, player_move, "Server", client->socket);
		
		if (exit_code != SERVER_SUCCESS)
			return exit_code;

		// Wait for client's choice 
		exit_code = SendGameOverMenu(client->socket);
		if (exit_code != SERVER_SUCCESS)
			return exit_code;

		exit_code = GetPlayerGameOverMenuChoice(client->socket, &user_choice);
		if (exit_code != SERVER_SUCCESS)
		{
			return exit_code;
		}
		if (user_choice != OPT_REPLAY)
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
	MOVE_TYPE move;

	move = rand() % 5;
	printf("Server is playing: %d\n", move);
	return move;
}

int ClientDisconnected(int client_id)
{
	// Close the socket
	closesocket(connected_clients[client_id]->socket);

	// Clear client's info
	free(connected_clients[client_id]);
	connected_clients[client_id] = NULL;

	return SERVER_CLIENT_DISCONNECTED;
}