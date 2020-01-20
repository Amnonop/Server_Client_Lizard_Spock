#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <Windows.h>
#include "client.h"
#include "others.h"
#include "SocketS.h"
#include "Commons.h"

#define SERVER_ADDRESS_STR "127.0.0.1"


//--> Global parameters
SOCKET m_socket; // comuunication socket
static my_username[MAX_LINE];
static HANDLE logfile_mutex; //log file mutex
HANDLE hThread[3]; // main threads
int game_started = 0, exit_signal = 0, user_accepted = 0, play_accepted = 0;
MsgQueue *msg_queue = NULL; // pointer to send message buffer

CLIENT_STATE client_state = FIRST_CONNECTION;

int PrintMainMenu();

BOOL StringsEqual(const char* string_1, const char* string_2)
{
	return strcmp(string_1, string_2) == 0;
}

param_node_t* CreateNode(char* param_value)
{
	int value_length;

	param_node_t* new_node = (param_node_t*)malloc(sizeof(param_node_t));
	if (new_node != NULL)
	{
		value_length = strlen(param_value) + 1;
		new_node->param_value = (char*)malloc(sizeof(char)*value_length);
		strcpy_s(new_node->param_value, value_length, param_value);

		new_node->next = NULL;
	}
	return new_node;
}

param_node_t* AddNode(param_node_t* head, char* param_value)
{
	param_node_t* tail;
	param_node_t* new_node;

	new_node = CreateNode(param_value);
	if (new_node == NULL)
		return NULL;

	// List is empty
	if (head == NULL)
		return new_node;

	tail = head;
	while (tail->next != NULL)
		tail = tail->next;
	
	tail->next = new_node;
	return head;
}

//Function that gets the raw message and break it into the Message struct without :/;
int GetMessageStruct(message_t *message, char *raw_string) 
{
	int exit_code = CLIENT_SUCCESS;
	const char* message_type_delim = ":";
	const char* param_delim = ";";
	char* token;
	char* next_token;
	int message_type_length;

	token = strtok_s(raw_string, message_type_delim, &next_token);
	message_type_length = strlen(token) + 1;
	message->message_type = (char*)malloc(sizeof(char)*message_type_length);
	strcpy_s(message->message_type, message_type_length, token);

	// Initialize param linked list
	message->parameters = NULL;

	token = strtok_s(NULL, param_delim, &next_token);
	while (token)
	{
		// Add a new parameter node
		// TODO: Handle the '\n' termination -> remove from the last parameter
		message->parameters = AddNode(message->parameters, token);
		if (message->parameters == NULL)
		{
			// TODO: Allocation failed
		}

		token = strtok_s(NULL, param_delim, &next_token);
	}
}

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


int computePlayerMove(char* player_move)
{
	if (STRINGS_ARE_EQUAL(player_move, "ROCK"))
		return 0;
	else if (STRINGS_ARE_EQUAL(player_move, "PAPER"))
		return 1;
	else if (STRINGS_ARE_EQUAL(player_move, "SCISSORS"))
		return 2;
	else if (STRINGS_ARE_EQUAL(player_move, "LIZARD"))
		return 3;
	else if (STRINGS_ARE_EQUAL(player_move, "SPOCK"))
		return 4;
	else
	{
		printf("Wrong input, exiting.\n");
		return (-1);
	}
}

//User application Thread (managing all user inputs to the client)
static DWORD ApplicationThread(LPVOID lpParam)
{
	char input[MAX_LINE], *send_message = NULL;
	int run = 0, type = 0;
	client_thread_params_t* thread_params;
	DWORD wait_code;
	BOOL release_res;
	int user_choice = -1;
	char *user_move = NULL;

	thread_params = (client_thread_params_t*)lpParam;

	while (1)
	{
		switch (client_state)
		{
			case FIRST_CONNECTION:
				// Send CLIENT_REQUEST message with username to server
				client_state = WAITING_SERVER_APPROVAL;
				SendClientRequestMessage(thread_params->username);
				break;
			case SERVER_APPROVED:
				//printf("Connected to server on %s:%d", thread_params->server_ip, thread_params->server_port);
				break;
			case MAIN_MENU:
				PrintMainMenu();
				scanf_s("%d", &user_choice);
				switch (user_choice)
				{
					case PLAY_VS_COMPUTER:
						client_state = WAITING_TO_START_GAME;
						SendClientCPUMessage();
						break;
					case PLAY_MOVE:
						playMoveMenuMessage();
						scanf_s("%s", user_move);
						int player_move = computePlayerMove(user_move);
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
		
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
	}
	return 0;
}

//Main thread that inisilatize all the sockets, programs and opens the send, recive and API threads
int MainClient(char* server_ip, int port_number, char* username)
{
	int exit_code;
	SOCKADDR_IN client_service;
	HANDLE hThread[2];
	WSADATA wsaData; 
	int startup_result;

	client_thread_params_t thread_params;
	
	startup_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (startup_result != NO_ERROR)
		printf("Error at WSAStartup()\n");

	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return;
	}

	client_service.sin_family = AF_INET;
	client_service.sin_addr.s_addr = inet_addr(server_ip);
	client_service.sin_port = htons(port_number);

	if (connect(m_socket, (SOCKADDR*)&client_service, sizeof(client_service)) == SOCKET_ERROR) 
	{
		printf("Failed connecting to server on %s:%d. Exiting\n", server_ip, port_number);
		WSACleanup();
		exit(0x555);
	}
	else
		//printf("Connected to server on %s:%s\n", SERVER_ADDRESS_STR, port_number);

	thread_params.username = username;
	thread_params.server_ip = server_ip;
	thread_params.server_port = port_number;

	//--> Creating mutex for logfile writing
	logfile_mutex = CreateMutex(
		NULL,	/* default security attributes */
		FALSE,	/* initially not owned */
		NULL);	/* unnamed mutex */
	if (NULL == logfile_mutex)
	{
		printf("Error when creating logfile mutex\n", GetLastError());
		exit(0x555);
	}

	//--> Creating message queue
	msg_queue = msg_queue_creator();
	if (msg_queue == NULL) {
		printf("Error creating msg_queue at MainClient function");
		exit(0x555);
	}

	//--> Opening threads
	hThread[0] = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)SendDataThread,
		NULL,
		0,
		NULL
	);
	hThread[1] = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)RecvDataThread,
		(LPVOID)&thread_params,
		0,
		NULL
	);
	hThread[2] = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)ApplicationThread,
		(LPVOID)&thread_params,
		0,
		NULL
	);

	WaitForMultipleObjects(3, hThread, FALSE, INFINITE);

	GetExitCodeThread(hThread[0], &exit_code); // Getting the error code from threads

	//---> Closing handles and free memory

	CloseHandle(hThread[0]); // SendDataThread
	CloseHandle(hThread[1]); // RecvDataThread
	CloseHandle(hThread[2]); // ApplicationThread

	closesocket(m_socket); // Close socket

	WSACleanup();

	if (logfile_mutex != NULL) CloseHandle(logfile_mutex); // Close logfile mutex handle

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

int SendClientRequestMessage(char* username)
{
	char* message_name = "CLIENT_REQUEST";
	int message_length;
	char* message_string;
	
	// Build message string
	message_length = strlen(message_name) + 1 + strlen(username) + 2;
	message_string = (char*)malloc(sizeof(char)*message_length);
	// TODO: Check malloc
	sprintf_s(message_string, message_length, "%s:%s\n", message_name, username);

	// Send the message
	EnqueueMsg(msg_queue, message_string);
}

int SendClientCPUMessage()
{
	char* message_name = "CLIENT_CPU";
	int message_length;
	char* message_string;

	// Build message string
	message_length = strlen(message_name) + 2;
	message_string = (char*)malloc(sizeof(char)*message_length);
	// TODO: Check malloc
	sprintf_s(message_string, message_length, "%s\n", message_name);

	// Send the message
	EnqueueMsg(msg_queue, message_string);
}

void playMoveMenuMessage()
{
	printf("Choose a move from the list: Rock, Papar, Scissors, Lizard or Spock:\n");
}

//The function gets the raw string from user and the type of message to send to server: username/play/message, and returns the formated message to send to the server
char* ConstructMessage(char *str, char *type)
{
	char *return_string = NULL, temp[10];
	int i = 0, j = 0;

	return_string = (char*)malloc(MAX_LINE * sizeof(char));
	if (return_string == NULL) {
		printf("Malloc Error in ConstructMessage function\n");
		exit(-1);
	}

	if (STRINGS_ARE_EQUAL(type, "play")) {
		strcpy(return_string, "PLAY_REQUEST");
		return_string[12] = ':';
		j = 13;
		i = 5;
		while (str[i] != '\0') {
			if (str[i] == '\n')
				break;
			return_string[j] = str[i];
			j++;
			i++;
		}
		return_string[j] = '\0';
	}
	if (STRINGS_ARE_EQUAL(type, "message")) {
		strcpy(return_string, "SEND_MESSAGE");
		return_string[12] = ':';
		j = 13;
		i = 8;
		while (str[i] != '\0') {
			if (str[i] == '\n')
				break;
			if (str[i] == ' ') {
				return_string[j] = ';';
				return_string[j + 1] = ' ';
				return_string[j + 2] = ';';
				i++;
				j = j + 3;
			}
			else {
				return_string[j] = str[i];
				j++;
				i++;
			}
		}
		return_string[j] = '\0';
	}
	if (STRINGS_ARE_EQUAL(type, "username")) {
		strcpy(return_string, "NEW_USER_REQUEST");
		return_string[16] = ':';
		j = 17;
		i = 0;
		while (str[i] != '\0') {
			if (str[i] == '\n')
				break;
			return_string[j] = str[i];
			j++;
			i++;
		}
		return_string[j] = '\0';
	}

	return return_string;
}

//The function detrmins if the string from the user (keyboard or file) is 'play' message (returns 1) or 'message' message (returns 2) or nither (returns 0)
int MessageType(char *input) {
	int type = 0;
	char temp_input_play[MAX_LINE], temp_input_message[MAX_LINE];

	// Coping the input to temps in order to check input type
	temp_input_play[0] = input[0];
	temp_input_message[0] = input[0];
	temp_input_play[1] = input[1];
	temp_input_message[1] = input[1];
	temp_input_play[2] = input[2];
	temp_input_message[2] = input[2];
	temp_input_play[3] = input[3];
	temp_input_message[3] = input[3];
	temp_input_play[4] = '\0';
	temp_input_message[4] = input[4];
	temp_input_message[5] = input[5];
	temp_input_message[6] = input[6];
	temp_input_message[7] = '\0';

	if (STRINGS_ARE_EQUAL(temp_input_play, "play") && (input[4] == ' ')) // It is a 'play' message
		type = 1;
	if (STRINGS_ARE_EQUAL(temp_input_message, "message") && (input[7] == ' ')) // It is a 'message' message
		type = 2;

	return type;
}

//The function allocate the msg_queue and creates all mutexs and shemaphors
MsgQueue *msg_queue_creator() {
	DWORD last_error;
	MsgQueue *temp = NULL;

	temp = (struct MsgQueue*)malloc(sizeof(struct MsgQueue)); // Malloc the msg_queue
	if (temp == NULL) {
		printf("Error allocating msg_queue at msg_queue_creator function\n");
		exit(-1);
	}

	temp->head = NULL;

	temp->access_mutex = CreateMutex(
		NULL,	/* default security attributes */
		FALSE,	/* initially not owned */
		NULL);	/* unnamed mutex */
	if (NULL == temp->access_mutex)
	{
		printf("Error when creating lmsg_queue mutexat msg_queue_creator function\n", GetLastError());
		exit(-1);
	}

	temp->msgs_count_semaphore = CreateSemaphore(
		NULL,	/* Default security attributes */
		0,		/* Initial Count - all slots are empty */
		30,		/* Maximum Count */
		NULL); /* un-named */
	if (temp->msgs_count_semaphore == NULL) {
		printf("Error when creating msgs_count semaphore: %d at msg_queue_creator function\n", GetLastError());
		exit(-1);
	}

	temp->queue_empty_event = CreateEvent(
		NULL, /* default security attributes */
		TRUE,       /* manual-reset event */
		TRUE,      /* initial state is non-signaled */
		NULL);         /* name */
	/* Check if succeeded and handle errors */

	last_error = GetLastError();
	/* If last_error is ERROR_SUCCESS, then it means that the event was created.
	   If last_error is ERROR_ALREADY_EXISTS, then it means that the event already exists */

	temp->stop_event = CreateEvent(
		NULL, /* default security attributes */
		TRUE,       /* manual-reset event */
		FALSE,      /* initial state is non-signaled */
		NULL);         /* name */
	/* Check if succeeded and handle errors */

	last_error = GetLastError();
	/* If last_error is ERROR_SUCCESS, then it means that the event was created.
	   If last_error is ERROR_ALREADY_EXISTS, then it means that the event already exists */
	return temp;
}

//The function gets pointer to message queue and a message string and put it in the buffer
void EnqueueMsg(MsgQueue *msg_queue, char *msg)
{
	MsgNode *node, *cur;

	if (!msg_queue) {
		printf("Error at EnqueueMsg function\n");
		return(QUEUE_ERROR);
	}

	node = (MsgNode*)malloc(sizeof(MsgNode));
	if (!node) {
		printf("Malloc error at EnqueueMsg function\n");
		exit(QUEUE_ERROR);
	}
	node->next = NULL;
	node->data = msg;

	if (WAIT_OBJECT_0 != WaitForSingleObject(msg_queue->access_mutex, INFINITE)) {
		printf("WaitForSingleObject error at EnqueueMsg function\n");
		return(QUEUE_ERROR);
	}
	if (msg_queue->head == NULL)
	{
		msg_queue->head = node;
	}
	else {
		cur = msg_queue->head;
		while (cur->next != NULL)
			cur = cur->next;
		cur->next = node;
	}

	if (!ReleaseSemaphore(msg_queue->msgs_count_semaphore, 1, NULL)) {
		printf("ReleaseSemaphore failed (%ld) at EnqueueMsg function\n", GetLastError());
		return(QUEUE_ERROR);
	}
	if (!ReleaseMutex(msg_queue->access_mutex)) {
		printf("ReleaseMutex failed (%ld) at EnqueueMsg function\n", GetLastError());
		return(QUEUE_ERROR);
	}

	if (!ResetEvent(msg_queue->queue_empty_event)) {
		printf("ResetEvent failed (%ld) at EnqueueMsg function\n", GetLastError());
		return(QUEUE_ERROR);
	}
}

//The function gets pointer to message and returns a message string from the buffer
char *DequeueMsg(MsgQueue *msg_queue)
{
	char *new_msg = NULL;
	MsgNode *temp_head;
	DWORD ret;
	HANDLE wait_for[2];

	wait_for[0] = msg_queue->stop_event;
	wait_for[1] = msg_queue->msgs_count_semaphore;

	/* checking if queue is empty */
	ret = WaitForSingleObject(msg_queue->msgs_count_semaphore, INFINITE);
	if (ret == WAIT_TIMEOUT)
	{
		SetEvent(msg_queue->queue_empty_event);
		ret = WaitForMultipleObjects(2, wait_for, FALSE, INFINITE);
		if (ret == WAIT_OBJECT_0) {
			return(QUEUE_ERROR);
		}
		if (ret != WAIT_OBJECT_0 + 1) {
			return(QUEUE_ERROR);
		}
	}
	else if (ret != WAIT_OBJECT_0) {
		return;
	}

	if (WAIT_OBJECT_0 != WaitForSingleObject(msg_queue->access_mutex, INFINITE))
		return GetLastError();

	temp_head = msg_queue->head;
	msg_queue->head = msg_queue->head->next;
	if (!ReleaseMutex(msg_queue->access_mutex))
		return GetLastError();

	new_msg = temp_head->data;
	free(temp_head);

	return new_msg;
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

//The function gets file pointer to logfile, format char: Received from server/Send to server, and the message itself, and write it to logfile
void PrintToLogFile(FILE *ptr, char *format, char *message) {
	DWORD wait_code;
	BOOL release_res;

	wait_code = WaitForSingleObject(logfile_mutex, INFINITE);
	if (wait_code != WAIT_OBJECT_0) printf("Fail while waiting for logfile mutex");

	//critical region
	fprintf(ptr, "%s: %s\n", format, message); //Writing to logfile
	//end of critical region

	release_res = ReleaseMutex(logfile_mutex);
	if (release_res == FALSE) printf("Fail releasing logfile mutex");
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