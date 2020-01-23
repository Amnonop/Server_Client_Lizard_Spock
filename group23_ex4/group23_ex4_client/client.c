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
#include "../Shared/ClientSrvCommons.h"
#include "../Shared/MessageTools.h"

#define SERVER_ADDRESS_STR "127.0.0.1"


//--> Global parameters
SOCKET m_socket; // comuunication socket
static my_username[MAX_LINE];
static HANDLE logfile_mutex; //log file mutex
HANDLE hThread[3]; // main threads
int game_started = 0, exit_signal = 0, user_accepted = 0, play_accepted = 0;
message_queue_t* msg_queue = NULL; // pointer to send message buffer

CLIENT_STATE client_state = FIRST_CONNECTION;

int HandleClientRequest(char* username, SOCKET socket);
int GetMainMenuMessage(SOCKET socket);
int PrintMainMenu();
int Play(SOCKET socket);
int ShowPlayMoveMenuMessage();
MOVE_TYPE ParsePlayerMove(const char* player_move);
int GetPlayerMoveRequestMessage(SOCKET socket);
void ShowGameResults(game_results_t* game_results);
GAME_OVER_MENU_OPTIONS GetGameOverMenuChoice();

//Reading data coming from the server Thread
static DWORD RecvDataThread(LPVOID lpParam)
{
	TransferResult_t receive_result;
	char* accepted_string = NULL;
	message_t* message = NULL;
	int board[BOARD_HEIGHT][BOARD_WIDTH] = { 0 }, type = 0;
	char input[MAX_LINE], *send_message = NULL;
	client_thread_params_t *info = NULL;
	DWORD wait_code;
	BOOL release_res;

	info = (client_thread_params_t*)lpParam;

	message = (message_t*)malloc(sizeof(message_t));

	while (1)
	{
		accepted_string = NULL;
		receive_result = ReceiveString(&accepted_string, m_socket);

		if (receive_result == TRNS_FAILED) 
		{ 
			// if the server disconnected
			printf("server disconnected. exiting.\n");

			thread_terminator("clean"); // exit the programm with error
			return 0;
		}
		else if (receive_result == TRNS_DISCONNECTED) 
		{
			printf("Server disconnected. Exiting.\n");// If the server disconnected
			return 0;
		}
		else
		{
			printf("Received a message from the server: %s\n", accepted_string);

			// Check the received message
			GetMessageStruct(message, accepted_string);

			if (STRINGS_ARE_EQUAL("SERVER_APPROVED", message->message_type))
			{
				client_state = SERVER_APPROVED;
			}
			else if (STRINGS_ARE_EQUAL("SERVER_MAIN_MENU", message->message_type))
			{
				client_state = MAIN_MENU;
			}
			else if (STRINGS_ARE_EQUAL("SERVER_PLAYER_MOVE_REQUEST", message->message_type))
			{
				client_state = PLAY_MOVE;
			}
		}
	//	else { // We recived a new message
	//		MessageEval(message, AcceptedStr); // Deconstructing the message (AcceptedStr)
	//		//---> Writing to logfile
	//		//PrintToLogFile(info->LogFile_ptr, "Received from server", AcceptedStr);

	//		if (game_started == 0) {// Game NOT Started!
	//			if (STRINGS_ARE_EQUAL("GAME_STARTED", message->message_type)) {
	//				printf("Game is on!\n");
	//				game_started = 1;
	//			}
	//			if (STRINGS_ARE_EQUAL("NEW_USER_DECLINED", message->message_type)) {
	//				printf("Request to join was refused\n");
	//				exit_signal = 1;
	//				thread_terminator("clean"); // exit the programm with error
	//				return 0;
	//			}
	//			if (STRINGS_ARE_EQUAL("NEW_USER_ACCEPTED", message->message_type)) {
	//				printf("Username accepted\n");
	//				printf("Number of players: %s\n", message->parameters->parameter);
	//				user_accepted = 1;
	//			}
	//		}
	//		else { // Game Started!
	//			if (STRINGS_ARE_EQUAL("BOARD_VIEW", message->message_type)) {
	//				if (message->parameters->next != NULL) { // If we got board update
	//					if (STRINGS_ARE_EQUAL("RED", message->parameters->next->next->parameter)) // Checking the player who uptated the board
	//						board[atoi(message->parameters->parameter)][atoi(message->parameters->next->parameter)] = RED_PLAYER; // Updating the board with RED
	//					if (STRINGS_ARE_EQUAL("YELLOW", message->parameters->next->next->parameter)) // Checking the player who uptated the board
	//						board[atoi(message->parameters->parameter)][atoi(message->parameters->next->parameter)] = YELLOW_PLAYER; // Updating the board with YELLOW
	//				}
	//				PrintBoard(board);
	//			}
	//			if (STRINGS_ARE_EQUAL("TURN_SWITCH", message->message_type)) {
	//				printf("%s's turn\n", message->parameters->parameter);
	//				if (STRINGS_ARE_EQUAL(message->parameters->parameter, my_username)) { // If it is MY TURN
	//					//---> file mode option
	//					if (STRINGS_ARE_EQUAL(info->input_mode, "file")) { // If we are in file mode
	//						type = 0; //Reseting type
	//						while (type != 1) { //While we didn't read from file play message
	//							fgets(input, MAX_LINE, info->InputFile_ptr); //Reading a string from file
	//							if (STRINGS_ARE_EQUAL(input, "\n")) // If we read a blank line
	//								continue;
	//							printf("%s", input);
	//							type = MessageType(input); // Checking the type of the input (play or message)

	//							//----> Play input
	//							if (type == 1) {
	//								send_message = ConstructMessage(input, "play"); //Constructing the message to send to the server
	//								//-> send play to send buffer
	//								EnqueueMsg(msg_queue, send_message);
	//							}
	//							//----> Exit input
	//							if (STRINGS_ARE_EQUAL(input, "exit\n") || STRINGS_ARE_EQUAL(input, "exit")) {
	//								//-> send exit to send buffer
	//								EnqueueMsg(msg_queue, send_message);

	//								//---> Writing to logfile
	//								PrintToLogFile(info->LogFile_ptr, "Custom message", "Player entered exit input...");

	//								thread_terminator("clean"); // exit the programm with error
	//								return 0; // Exit the while loop
	//							}
	//							//----> Message input
	//							if (type == 2) {
	//								send_message = ConstructMessage(input, "message"); //Constructing the message to send to the server
	//								//-> send play to send buffer
	//								EnqueueMsg(msg_queue, send_message);
	//							}
	//							//---> Writing to logfile
	//							if (STRINGS_ARE_EQUAL("Error: Illegal command", send_message))
	//								PrintToLogFile(info->LogFile_ptr, "Custom message", send_message);
	//							else
	//								PrintToLogFile(info->LogFile_ptr, "Sent to server", send_message);
	//						}
	//					}
	//				}
	//			}
	//			if (STRINGS_ARE_EQUAL("PLAY_ACCEPTED", message->message_type)) {
	//				printf("Well played\n");
	//				play_accepted = 1;
	//			}
	//			if (STRINGS_ARE_EQUAL("PLAY_DECLINED", message->message_type)) {
	//				printf("Error: ");
	//				while (message->parameters->parameter != NULL) {
	//					printf("%s", message->parameters->parameter);
	//					message->parameters = message->parameters->next;
	//				}
	//				printf("\n");
	//				play_accepted = 0;
	//			}
	//			if (STRINGS_ARE_EQUAL("GAME_ENDED", message->message_type)) {
	//				if (STRINGS_ARE_EQUAL(message->parameters->parameter, "tied")) // We got tie
	//					printf("Game ended. Everybody wins!\n");
	//				else
	//					printf("Game ended. The winner is %s!\n", message->parameters->parameter); //Print the winner name
	//				exit_signal = 1;

	//				//---> Writing to logfile
	//				PrintToLogFile(info->LogFile_ptr, "Received from server", AcceptedStr);
	//				PrintToLogFile(info->LogFile_ptr, "Custom message", "Game ended. Exiting...");

	//				thread_terminator("clean"); // Clean exit the programm
	//				return 0; // Exit the while loop
	//			}
	//			if (STRINGS_ARE_EQUAL("RECEIVE_MESSAGE", message->message_type)) {
	//				printf("%s: ", message->parameters->parameter);
	//				message->parameters = message->parameters->next;
	//				while (message->parameters->parameter != NULL) {
	//					printf("%s", message->parameters->parameter);
	//					message->parameters = message->parameters->next;
	//				}
	//				printf("\n");
	//			}
	//		}
	//	}
	//	//free(send_message);
	}
	//free(message);
	return 0;
}

//Sending data to the server Thread
static DWORD SendDataThread(void)
{
	char *new_msg = NULL;
	TransferResult_t SendRes;

	while (1)
	{
		new_msg = DequeueMsg(msg_queue); // Dequeue message from buffer to send to the server

		if (STRINGS_ARE_EQUAL(new_msg, "exit"))
			return 0; //"exit" signals an exit from the client side

		printf("Sending message: %s\n", new_msg);
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
				client_state = WAITING_TO_START_GAME;
				exit_code = SendClientCPUMessage(msg_queue);
				if (exit_code != MSG_SUCCESS)
					return exit_code;

				exit_code = Play(thread_params->socket);
				if (exit_code != CLIENT_SUCCESS)
					return exit_code;
			break;
			default:
				break;
		}
	}

	//while (1)
	//{
	//	switch (client_state)
	//	{
	//		case FIRST_CONNECTION:
	//			// Send CLIENT_REQUEST message with username to server
	//			client_state = WAITING_SERVER_APPROVAL;
	//			exit_code = SendClientRequestMessage(thread_params->username, msg_queue);
	//			if (exit_code != QUEUE_SUCCESS)
	//			{
	//				return CLIENT_SEND_MSG_FAILED;
	//			}
	//			break;
	//		case SERVER_APPROVED:
	//			//printf("Connected to server on %s:%d", thread_params->server_ip, thread_params->server_port);
	//			break;
	//		case MAIN_MENU:
	//			PrintMainMenu();
	//			scanf_s("%d", &user_choice);
	//			switch (user_choice)
	//			{
	//				case CLIENT_CPU:
	//					client_state = WAITING_TO_START_GAME;
	//					SendClientCPUMessage(msg_queue);
	//					break;
	//				default:
	//					break;
	//			}
	//			break;
	//		case PLAY_MOVE:
	//			PlayMove();
	//			break;
	//		default:
	//			break;
	//	}
		
		//if ((game_started == 0) && (user_accepted == 0) && (run == 0)) // Username input for user
		//{
		//	//run++;
		//	//strcpy(my_username, input); //Saving my username
		//	//send_message = ConstructMessage(input, "username"); //Constructing the message to send to the server
		//	////----->sending to buffer
		//	//EnqueueMsg(msg_queue, send_message);

		//	////---> Writing to logfile
		//	//PrintToLogFile(info->LogFile_ptr, "Sent to server", send_message);

		//}
		//if ((game_started == 1) && (user_accepted == 1)) { //Game started!
		//	if (STRINGS_ARE_EQUAL(info->input_mode, "human")) { // If we are in human mode
		//		gets_s(input, sizeof(input)); //Reading a string from the keyboard

		//		type = MessageType(input); // Checking the type of the input (play or message)

		//		//----> Play input
		//		if (type == 1) {
		//			send_message = ConstructMessage(input, "play"); //Constructing the message to send to the server
		//			//-> send play to send buffer
		//			EnqueueMsg(msg_queue, send_message);
		//		}
		//		//----> Exit input
		//		else if (STRINGS_ARE_EQUAL(input, "exit")) {
		//			//-> send exit to send buffer
		//			EnqueueMsg(msg_queue, send_message);

		//			//---> Writing to logfile
		//			PrintToLogFile(info->LogFile_ptr, "Custom message", "Player entered exit input...");

		//			thread_terminator("clean"); // Clean exit the programm
		//			return 0;
		//		}
		//		//----> Message input
		//		else if (type == 2) {
		//			send_message = ConstructMessage(input, "message"); //Constructing the message to send to the server
		//			//-> send play to send buffer
		//			EnqueueMsg(msg_queue, send_message);
		//		}
		//		else {
		//			printf("Error: Illegal command\n");
		//			send_message = "Error: Illegal command";
		//		}
		//		//---> Writing to logfile
		//		if (STRINGS_ARE_EQUAL("Error: Illegal command", send_message))
		//			PrintToLogFile(info->LogFile_ptr, "Custom message: User input error", input);
		//		else
		//			PrintToLogFile(info->LogFile_ptr, "Sent to server", send_message); //Writing to logfile

		//	}
		//}
		//free(send_message);
	//}
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
		player_move = ParsePlayerMove(user_move);

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

int GetPlayerMoveRequestMessage(SOCKET socket)
{
	int exit_code;
	message_t* message = NULL;

	printf("Waiting for SERVER_PLAYER_MOVE_REQUEST.\n");
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
	printf("%s won!\n", game_results->winner);
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
	if (msg_queue == NULL) {
		printf("Error initializing the message queue.\n");
		return CLIENT_MSG_QUEUE_INIT_FAILED;
	}

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
	
	startup_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (startup_result != NO_ERROR)
	{
		printf("Error at WSAStartup()\n");
		// TODO: Free message queue, Send Data thread handler
		return CLIENT_WSA_STARTUP_ERROR;
	}

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
			printf("Failed connected to server on %s:%d\n", server_ip, port_number);
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
	//if (connect(m_socket, (SOCKADDR*)&client_service, sizeof(client_service)) == SOCKET_ERROR) 
	//{
	//	printf("Failed connecting to server on %s:%d. Exiting\n", server_ip, port_number);
	//	WSACleanup();
	//	exit(0x555);
	//}
	//else
	//	//printf("Connected to server on %s:%s\n", SERVER_ADDRESS_STR, port_number);

	thread_params.username = username;
	thread_params.server_ip = server_ip;
	thread_params.server_port = port_number;
	thread_params.socket = m_socket;
	
	/*hThread[1] = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)RecvDataThread,
		(LPVOID)&thread_params,
		0,
		NULL
	);*/
	hThread[1] = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)ApplicationThread,
		(LPVOID)&thread_params,
		0,
		NULL
	);

	WaitForMultipleObjects(2, hThread, FALSE, INFINITE);

	GetExitCodeThread(hThread[0], &exit_code); // Getting the error code from threads

	//---> Closing handles and free memory

	CloseHandle(hThread[0]); // SendDataThread
	CloseHandle(hThread[1]); // RecvDataThread
	//CloseHandle(hThread[2]); // ApplicationThread

	closesocket(m_socket); // Close socket

	WSACleanup();

	CloseHandle(msg_queue->access_mutex); // Message queue access_mutex
	CloseHandle(msg_queue->msgs_count_semaphore); // Message queue msgs_count_semaphore
	CloseHandle(msg_queue->queue_empty_event); // Message queue queue_empty_event
	CloseHandle(msg_queue->stop_event); // Message queue stop_event
	free(msg_queue); // Free Message queue

	if (exit_code == 0) // The programm ended clean
		return 0;
	else // The programm got an error
		return 0x555;
}

//The function terminats all threads. For type "clean" -> termination with code 0, and returns 0. For type "dirty" -> termination with code 0x555, and returns 0x555.
void thread_terminator(char *type) {

	if (STRINGS_ARE_EQUAL(type, "clean")) {
		TerminateThread(hThread[0], 0);
		TerminateThread(hThread[1], 0);
		TerminateThread(hThread[2], 0);
	}
	if (STRINGS_ARE_EQUAL(type, "dirty")) {
		TerminateThread(hThread[0], 0x555);
		TerminateThread(hThread[1], 0x555);
		TerminateThread(hThread[2], 0x555);
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