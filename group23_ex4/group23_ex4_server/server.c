#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h>

#include "others.h" 
#include "server.h"
#include "socketS.h"


// global variables used in defferent parts, threads and functions of the program
User user_names[2];
int num_of_players = 0, global_num_of_players = 0;
int board[BOARD_HEIGHT][BOARD_WIDTH] = { 0 };
int Turn;
int Ind;
int num_of_moves = 0;
int line, column_num;
int we_can_start = 0, game_ended = 0;
int print_the_board = 0;
char regular_message[MAX_MESSAGE_SIZE];
BOOL starting_game = FALSE;
SOCKET players_sockets[2];
FILE *file = NULL;

// events handles 
HANDLE StartingGameHandle;
HANDLE GameManagerHandle;
HANDLE MessagesHandle;
HANDLE DiscconectThem;

// mutexes handles
HANDLE PrintToFile;
HANDLE GameEnded;

// threads handles
HANDLE ThreadHandles[NUM_OF_WORKER_THREADS];
SOCKET ThreadInputs[NUM_OF_WORKER_THREADS];
HANDLE ThreadManager;
HANDLE ThreadMessages;
HANDLE ThreadDisconnect;

static int FindFirstUnusedThreadSlot();
static void CleanupWorkerThreads();
static DWORD ServiceThread(int num);


void MainServer(char *log_file, char *port_number)
{
	DWORD wait_code;
	int Loop;
	SOCKET MainSocket = INVALID_SOCKET;
	unsigned long Address;
	SOCKADDR_IN service;
	int bindRes;
	int ListenRes;
	int port = atoi(port_number);

	// Initialize Winsock.
	WSADATA wsaData;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (StartupRes != NO_ERROR)
	{
		printf("error %ld at WSAStartup( ), ending program.\n", WSAGetLastError());
		// Tell the user that we could not find a usable WinSock DLL.                                  
		return;
	}

	// opening log file for writing
	file = fopen(log_file, "w");

	////////////////////////// events and mutexes ////////////////////////////////////
// StartingGameHandle- event for starting game- will be rleased when two players are connected to the game.
	StartingGameHandle = CreateEvent(
		NULL, /* default security attributes */
		FALSE,       /* manual-reset event */
		FALSE,      /* initial state is non-signaled */
		"Starting_Game_Handle");         /* name */

	if (NULL == StartingGameHandle)
	{
		printf("Couldn't create thread\n");
		exit(ERROR_CODE);
	}

	// GameManagerHandle- event which will use for everytime we need to switch turns, send the board, and ending the game.
	GameManagerHandle = CreateEvent(
		NULL, /* default security attributes */
		TRUE,       /* manual-reset event */
		FALSE,      /* initial state is non-signaled */
		"Game_Manager_Handle");         /* name */

	if (NULL == GameManagerHandle)
	{
		printf("Couldn't create thread\n");
		exit(ERROR_CODE);
	}

	// MessagesHandle- event which will use for regular messages transfers
	MessagesHandle = CreateEvent(
		NULL, /* default security attributes */
		TRUE,       /* manual-reset event */
		FALSE,      /* initial state is non-signaled */
		"Messages_Handle");         /* name */

	if (NULL == MessagesHandle)
	{
		printf("Couldn't create thread\n");
		exit(ERROR_CODE);
	}

	// DiscconectThem- when one of the players disconnects, or the game is finished, this event will close all relevant threds
	// and reset all varibles for the next game.
	DiscconectThem = CreateEvent(
		NULL, /* default security attributes */
		TRUE,       /* manual-reset event */
		FALSE,      /* initial state is non-signaled */
		"Discconect_Them");         /* name */

	if (NULL == DiscconectThem)
	{
		printf("Couldn't create thread\n");
		exit(ERROR_CODE);
	}

	// PrintToFile- will be used when writing into files- can happen from different threads- need a mutex to settle it
	PrintToFile = CreateMutex(
		NULL,
		NULL,
		"Print_To_Log"
	);

	if (NULL == PrintToFile)
	{
		printf("Couldn't create thread\n");
		exit(ERROR_CODE);
	}

	// GameEnded- when sending to both players that the game has ended, the message suppose to be sent to both of them.
	GameEnded = CreateMutex(
		NULL,
		NULL,
		"Game_Ended"
	);

	if (NULL == GameEnded)
	{
		printf("Couldn't create thread\n");
		exit(ERROR_CODE);
	}
	////////////////////////// events and mutexes ////////////////////////////////////


	// Create a socket.    
	MainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (MainSocket == INVALID_SOCKET)
	{
		printf("Error at socket( ): %ld\n", WSAGetLastError());
		goto server_cleanup_1;
	}

	Address = inet_addr(SERVER_ADDRESS_STR);
	if (Address == INADDR_NONE)
	{
		printf("The string \"%s\" cannot be converted into an ip address. ending program.\n",
			SERVER_ADDRESS_STR);
		goto server_cleanup_2;
	}

	service.sin_family = AF_INET;
	service.sin_addr.s_addr = Address;
	service.sin_port = htons(port);

	bindRes = bind(MainSocket, (SOCKADDR*)&service, sizeof(service));
	if (bindRes == SOCKET_ERROR)
	{
		printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
		goto server_cleanup_2;
	}

	// Listen on the Socket.
	ListenRes = listen(MainSocket, SOMAXCONN);
	if (ListenRes == SOCKET_ERROR)
	{
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		goto server_cleanup_2;
	}

	// Initialize all thread handles to NULL, to mark that they have not been initialized
	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
		ThreadHandles[Ind] = NULL;

	printf("Waiting for a client to connect...\n");

	/////////////////////////////// threads ///////////////////////////////////////

// ThreadManager- controled by GameManagerHandle- waiting for it to be released and manage the game
	ThreadManager = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)ManagerThread,
		NULL,
		0,
		NULL);

	if (NULL == ThreadManager)
	{
		printf("Couldn't create thread\n");
		exit(ERROR_CODE);
	}
	// ThreadMessages- controled by MessagesHandle- waiting for it to be released and send the message to the other player
	ThreadMessages = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)RegularMessages,
		NULL,
		0,
		NULL);

	if (NULL == ThreadMessages)
	{
		printf("Couldn't create thread\n");
		exit(ERROR_CODE);
	}
	//ThreadDisconnect- controled by DiscconectThem- will be released when disconnection of the players is needed
	ThreadDisconnect = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)Disconnection,
		NULL,
		0,
		NULL);

	if (NULL == ThreadDisconnect)
	{
		printf("Couldn't create thread\n");
		exit(ERROR_CODE);
	}
	/////////////////////////////// threads ///////////////////////////////////////

// starting an infinite loop that will always waiting for new players, and accepts only two players conncted at a time
	while (TRUE)
	{
		// getting the socket for the player
		SOCKET AcceptSocket = accept(MainSocket, NULL, NULL);
		if (AcceptSocket == INVALID_SOCKET)
		{
			printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
			goto server_cleanup_3;
		}

		printf("Client Connected.\n");

		//checking for available place.
		Ind = FindFirstUnusedThreadSlot();

		// saving player's socket
		user_names[num_of_players].user_socket = AcceptSocket;

		num_of_players += 1;
		if (num_of_players == 1) { //if this is the first player connected
			user_names[num_of_players - 1].user_color = RED_PLAYER;
			players_sockets[num_of_players - 1] = AcceptSocket;

			// creating thread for first player- he will wait there for the second player to connect
			ThreadHandles[num_of_players - 1] = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)ServiceThread,
				num_of_players,
				0,
				NULL);

			if (NULL == ThreadHandles[num_of_players - 1])
			{
				printf("Couldn't create thread\n");
				exit(ERROR_CODE);
			}
		}
		else if (num_of_players == 2) {//if this is the second player connected
			user_names[num_of_players - 1].user_color = YELLOW_PLAYER;
			players_sockets[num_of_players - 1] = AcceptSocket;

			// creating thread for second player- after this connection the game will start
			ThreadHandles[num_of_players - 1] = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)ServiceThread,
				num_of_players,
				0,
				NULL);

			if (NULL == ThreadHandles[num_of_players - 1])
			{
				printf("Couldn't create thread\n");
				exit(ERROR_CODE);
			}
		}
		if (num_of_players == 3) { // if another player tries to connect- he will be informed he can't and the relevand socket will be closed
			printf("No slots available for client, dropping the connection.\n");
			closesocket(AcceptSocket);
			num_of_players -= 1;
		}
	} // while(TRUE)

// waiting for all running threads to finish their jobs. 
	CleanupWorkerThreads(); //will close players threads
	wait_code = WaitForSingleObject(1, ThreadManager, TRUE, INFINITE);
	wait_code = WaitForSingleObject(1, ThreadMessages, TRUE, INFINITE);
	wait_code = WaitForSingleObject(1, ThreadDisconnect, TRUE, INFINITE);

	//closing log file
	close(file);
server_cleanup_3:
	FreeAndClean();
server_cleanup_2:
	if (closesocket(MainSocket) == SOCKET_ERROR)
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
	FreeAndClean();

server_cleanup_1:
	if (WSACleanup() == SOCKET_ERROR)
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
	FreeAndClean();
}

DWORD ServiceThread(int num) {
	char SendStr[SEND_STR_SIZE];
	BOOL Done = FALSE;
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	int player_number = num - 1;
	int found_place;
	char str_to_send[MAX_MESSAGE_SIZE];
	char *AcceptedStr = NULL;

	Message *m = malloc(sizeof(Message));
	if (NULL == m) {
		printf("memory allocation failed");
		exit(ERROR_CODE);
	}

	// waiting for new message
	RecvRes = ReceiveString(&AcceptedStr, players_sockets[player_number]);
	if (RecvRes == TRNS_FAILED) {
		printf("Player disconnected. Ending communication.\n");
		WaitForSingleObject(PrintToFile, INFINITE);
		fprintf(file, "Player disconnected. Ending communication.\n");
		ReleaseMutex(PrintToFile);
		closesocket(players_sockets[player_number]);
		SetEvent(DiscconectThem);
		return -1;
	}
	else if (RecvRes == TRNS_DISCONNECTED) {
		printf("Player disconnected. Ending communication.\n");
		WaitForSingleObject(PrintToFile, INFINITE);
		fprintf(file, "Player disconnected. Ending communication.\n");
		ReleaseMutex(PrintToFile);
		closesocket(players_sockets[player_number]);
		SetEvent(DiscconectThem);
		return -1;
	}

	// breake message into message type and it's other parts
	else MessageEval(m, AcceptedStr);

	if (STRINGS_ARE_EQUAL(m->message_type, "NEW_USER_REQUEST")) {// if it is a new request to join the game
		if (CheckUserName(m->parameters->parameter, players_sockets[global_num_of_players]) == 1) { //player has the same name
			SendRes = SendString("NEW_USER_DECLINED", players_sockets[global_num_of_players]);
			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(players_sockets[player_number]);
				FreeAndClean();
			}
			closesocket(players_sockets[player_number]);
			Ind -= 1;
			num_of_players -= 1;
			return -1;
		}
		else { // accepting the new player 
			sprintf(str_to_send, "NEW_USER_ACCEPTED:%d", global_num_of_players);
			SendRes = SendString(str_to_send, players_sockets[player_number]);
			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(players_sockets[player_number]);
				FreeAndClean();
			}
			if (global_num_of_players == 1) { // if this is the first player- wait forever for the second one
				WaitForSingleObject(GameManagerHandle, INFINITE);
			}
			else if (global_num_of_players == 2) { // if it is the second player- update that the game can start and release the first player that waits
				we_can_start = 1;
				SetEvent(GameManagerHandle);
			}
		}
	}
	free(m);
	// entering to infinite loop that will be ended only when the game is ended.
	while (!Done)
	{
		AcceptedStr = NULL;
		m = malloc(sizeof(Message));
		if (NULL == m) {
			printf("memory allocation failed");
			exit(ERROR_CODE);
		}
		// waiting for new message
		RecvRes = ReceiveString(&AcceptedStr, players_sockets[player_number]);
		if (RecvRes == TRNS_FAILED) {
			printf("Player disconnected. Ending communication.\n");
			WaitForSingleObject(PrintToFile, INFINITE);
			fprintf(file, "Player disconnected. Ending communication.\n");
			ReleaseMutex(PrintToFile);
			closesocket(players_sockets[player_number]);
			SetEvent(DiscconectThem);
			return -1;
		}
		else if (RecvRes == TRNS_DISCONNECTED) {
			printf("Player disconnected. Ending communication.\n");
			WaitForSingleObject(PrintToFile, INFINITE);
			fprintf(file, "Player disconnected. Ending communication.\n");
			ReleaseMutex(PrintToFile);
			closesocket(players_sockets[player_number]);
			SetEvent(DiscconectThem);
			return -1;
		}

		// breake message into message type and it's other parts
		else MessageEval(m, AcceptedStr);

		if (STRINGS_ARE_EQUAL(m->message_type, "SEND_MESSAGE")) { // if one of the players sends message to the other
			int length = strlen(AcceptedStr) - 13;
			if (length >= 0) {
				strncpy(regular_message, AcceptedStr + 13, length);
				regular_message[length] = '\0';
			}
			Turn = player_number;
			SetEvent(MessagesHandle); // releasing the event handling the messages transformations
		}

		if (STRINGS_ARE_EQUAL(m->message_type, "PLAY_REQUEST")) {// if a player wants to make a move
			if (we_can_start == 1) { // if two players are connected to game
				if (user_names[!player_number].my_turn == TRUE) { // if it's the current player turn
					column_num = atoi(m->parameters->parameter);
					if (column_num >= 0 && column_num <= 6) { // if player asked for reasonable column
						for (line = 0; line < 6; line++) {
							found_place = 0;
							if (board[line][column_num] == 0) {
								if (user_names[!player_number].my_turn == TRUE)
									board[line][column_num] = user_names[!player_number].user_color;
								found_place = 1;
								break;
							}
						}
						if (found_place == 1) { // if his move is legit
							num_of_moves += 1;
							SendRes = SendString("PLAY_ACCEPTED", players_sockets[player_number]);
							if (SendRes == TRNS_FAILED)
							{
								printf("Service socket error while writing, closing thread.\n");
								closesocket(players_sockets[player_number]);
								FreeAndClean();
							}
							SetEvent(GameManagerHandle);
						}
						else SendRes = SendString("PLAY_DECLINED:Illegal; ;move", players_sockets[player_number]);
					}
					else SendRes = SendString("PLAY_DECLINED:Illegal; ;move", players_sockets[player_number]);
				}
				else SendRes = SendString("PLAY_DECLINED:Not; ;your; ;turn", players_sockets[player_number]);
			}
			else SendRes = SendString("PLAY_DECLINED:Game; ;has; ;not; ;started", players_sockets[player_number]);
		}

		if (SendRes == TRNS_FAILED)
		{
			printf("Service socket error while writing, closing thread.\n");
			closesocket(players_sockets[player_number]);
			FreeAndClean();
		}

		// free alocated memory
		free(m);
		free(AcceptedStr);
	}
	return 0;
}

void FreeAndClean() {

	//threads clean 
	if (ThreadManager != NULL) CloseHandle(ThreadManager);
	if (ThreadMessages != NULL) CloseHandle(ThreadMessages);
	if (ThreadDisconnect != NULL) CloseHandle(ThreadDisconnect);

	//events clean
	if (StartingGameHandle != NULL) CloseHandle(StartingGameHandle);
	if (GameManagerHandle != NULL) CloseHandle(GameManagerHandle);
	if (MessagesHandle != NULL) CloseHandle(MessagesHandle);
	if (DiscconectThem != NULL) CloseHandle(DiscconectThem);

	//mutexes clean
	if (PrintToFile != NULL) CloseHandle(PrintToFile);
	if (GameEnded != NULL) CloseHandle(GameEnded);

	//exit(ERROR_CODE);
}

static int FindFirstUnusedThreadSlot()
{
	int Ind;

	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
	{
		if (ThreadHandles[Ind] == NULL)
			break;
		else
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);

			if (Res == WAIT_OBJECT_0) // this thread finished running
			{
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
		}
	}
	return Ind;
}

static void CleanupWorkerThreads()
{
	int Ind;

	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
	{
		if (ThreadHandles[Ind] != NULL)
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], INFINITE);

			if (Res == WAIT_OBJECT_0)
			{
				closesocket(ThreadInputs[Ind]);
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
			else
			{
				printf("Waiting for thread failed. Ending program\n");
				return;
			}
		}
	}
}

DWORD Disconnection(LPVOID lpParam) {
	while (TRUE) {
		WaitForSingleObject(DiscconectThem, INFINITE);

		// closing all players threads 
		CloseHandle(ThreadHandles[0]);
		CloseHandle(ThreadHandles[1]);
		TerminateThread(ThreadHandles[0], 0);
		TerminateThread(ThreadHandles[1], 0);
		ThreadHandles[0] = NULL;
		ThreadHandles[1] = NULL;

		closesocket(players_sockets[0]); //closing all sockets
		closesocket(players_sockets[1]);

		strcpy(user_names[0].user_name, ""); //reset players names
		strcpy(user_names[1].user_name, "");

		// reseting globals
		num_of_players = 0;
		game_ended = 0;
		global_num_of_players = 0;
		Ind = 0;
		num_of_moves = 0;
		num_of_players = 0;
		for (int i = 0; i <= 6; i++) // reset board
			for (int j = 0; j <= 5; j++)
				board[j][i] = 0;
		ResetEvent(DiscconectThem);
	}
}

DWORD RegularMessages(LPVOID lpParam) {
	char str_to_send[MAX_MESSAGE_SIZE];
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	BOOL Done = FALSE;
	while (!Done) {
		WaitForSingleObject(MessagesHandle, INFINITE);
		sprintf(str_to_send, "RECEIVE_MESSAGE:%s;%s", user_names[Turn].user_name, regular_message); // a message will be sent to the other player
		SendRes = SendString(str_to_send, players_sockets[!Turn]);
		if (SendRes == TRNS_FAILED)
		{
			printf("Service socket error while writing, closing thread.\n");
			closesocket(players_sockets[!Turn]);
			FreeAndClean();
		}
		ResetEvent(MessagesHandle);
	}
}

DWORD ManagerThread(LPVOID lpParam) {
	char str_to_send[MAX_MESSAGE_SIZE];
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	BOOL Done = FALSE;

	while (!Done) {
		WaitForSingleObject(GameManagerHandle, INFINITE);
		if (num_of_moves == 0) { // if no moves happenden yet- inform everyone the game has started and clean board
			SendRes = SendString("GAME_STARTED", players_sockets[0]);
			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(players_sockets[0]);
			}
			SendRes = SendString("GAME_STARTED", players_sockets[1]);
			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(players_sockets[1]);
			}
			sprintf(str_to_send, "BOARD_VIEW");
		}
		else { // if it's not the first round, send an updated version of the board
			if (user_names[0].my_turn == FALSE)
				sprintf(str_to_send, "BOARD_VIEW:%d;%d;%s", 5 - line, column_num, "RED");
			else
				sprintf(str_to_send, "BOARD_VIEW:%d;%d;%s", 5 - line, column_num, "YELLOW");
		}

		SendRes = SendString(str_to_send, players_sockets[0]);
		if (SendRes == TRNS_FAILED)
		{
			printf("Service socket error while writing, closing thread.\n");
			closesocket(players_sockets[0]);
		}
		SendRes = SendString(str_to_send, players_sockets[1]);
		if (SendRes == TRNS_FAILED)
		{
			printf("Service socket error while writing, closing thread.\n");
			closesocket(players_sockets[1]);
		}

		if (DetermineIfWinnerShot() == 1) { // after each move, we check if it was a winner.
			global_num_of_players = 0;
			game_ended = 1;
			if (user_names[0].my_turn == FALSE)
				sprintf(str_to_send, "GAME_ENDED:%s", user_names[0].user_name);
			else
				sprintf(str_to_send, "GAME_ENDED:%s", user_names[1].user_name);
			// sending that the game has ended to both of the players
			WaitForSingleObject(GameEnded, INFINITE);
			SendRes = SendString(str_to_send, players_sockets[0]);
			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(players_sockets[0]);
			}
			ReleaseMutex(GameEnded);
			WaitForSingleObject(GameEnded, INFINITE);
			SendRes = SendString(str_to_send, players_sockets[1]);
			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(players_sockets[1]);
			}
			ReleaseMutex(GameEnded);
		}
		else if (DetermineIfWinnerShot() == 2) {// after each move, we check if it was a tie.
			global_num_of_players = 0;
			game_ended = 1;
			sprintf(str_to_send, "GAME_ENDED:tied");

			// sending that the game has ended to both of the players
			WaitForSingleObject(GameEnded, INFINITE);
			SendRes = SendString(str_to_send, players_sockets[0]);
			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(players_sockets[0]);
			}
			ReleaseMutex(GameEnded);
			Sleep(10);
			WaitForSingleObject(GameEnded, INFINITE);
			SendRes = SendString(str_to_send, players_sockets[1]);
			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(players_sockets[1]);
			}
			ReleaseMutex(GameEnded);
		}
		if (game_ended == 0) {
			// switching turns
			if (user_names[0].my_turn == TRUE) {
				sprintf(str_to_send, "TURN_SWITCH:%s", user_names[0].user_name);
				user_names[0].my_turn = FALSE;
				user_names[1].my_turn = TRUE;
			}
			else {
				sprintf(str_to_send, "TURN_SWITCH:%s", user_names[1].user_name);
				user_names[1].my_turn = FALSE;
				user_names[0].my_turn = TRUE;
			}

			SendRes = SendString(str_to_send, players_sockets[0]);
			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(players_sockets[0]);
			}
			SendRes = SendString(str_to_send, players_sockets[1]);
			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(players_sockets[1]);
			}
		}

		ResetEvent(GameManagerHandle);
	}
}

int CheckUserName(char *user_name, SOCKET t_socket) {
	for (int i = 0; i < 2; i++) {
		if (strcmp(user_name, user_names[i].user_name) == 0) { //if this name is already in use
			return 1;
		}
		else if (strlen(user_names[i].user_name) == 0) { // finding place for the new player and save relevant information on him
			strcpy(user_names[i].user_name, user_name);
			user_names[i].user_socket = t_socket;
			if (i == 0) {
				user_names[i].user_color = RED_PLAYER;
				user_names[i].my_turn = TRUE;
			}
			else {
				user_names[i].user_color = YELLOW_PLAYER;
				user_names[i].my_turn = FALSE;
			}
			Turn = 0;
			global_num_of_players += 1;
			return 0;
		}
	}
}

int DetermineIfWinnerShot() {

	for (int column = 0; column <= 6; column++) { // checking for straight lines.
		for (int lines = 0; lines <= 2; lines++) {
			if (((board[lines][column] == board[lines + 1][column] && board[lines + 2][column] == board[lines + 3][column] && board[lines + 3][column] == board[lines][column]) &&
				(board[lines][column] != 0 && board[lines + 1][column] != 0 && board[lines + 2][column] != 0 && board[lines + 3][column] != 0)) ||
				((board[lines][column] == board[lines][column + 1] && board[lines][column + 2] == board[lines][column + 3] && board[lines][column] == board[lines][column + 3]) &&
				(board[lines][column] != 0 && board[lines][column + 1] != 0 && board[lines][column + 2] != 0 && board[lines][column + 3] != 0))) {
				return 1;
			}
		}
	}
	for (int column = 0; column <= 3; column++) { // checking for incrementing diagonal.
		for (int lines = 0; lines <= 2; lines++) {
			if ((board[lines][column] == board[lines + 1][column + 1] && board[lines + 2][column + 2] == board[lines + 3][column + 3] != 0 && board[lines][column] == board[lines + 3][column + 3]) &&
				(board[lines][column] != 0 && board[lines + 1][column + 1] != 0 && board[lines + 2][column + 2] != 0 && board[lines + 3][column + 3] != 0)) {
				return 1;
			}
		}
	}
	for (int column = 0; column <= 3; column++) { // checking for descending diagonal.
		for (int lines = 5; lines >= 3; lines--) {
			if ((board[lines][column] == board[lines - 1][column + 1] && board[lines - 2][column + 2] == board[lines - 3][column + 3] != 0 && board[lines][column] == board[lines - 3][column + 3]) &&
				(board[lines][column] != 0 && board[lines - 1][column + 1] != 0 && board[lines - 2][column + 2] != 0 && board[lines - 3][column + 3] != 0))
				return 1;
		}
	}

	if (num_of_moves == 42) // if it is a tie
		return 2;
	return 0;
}