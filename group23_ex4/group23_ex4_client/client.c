#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <Windows.h>
#include "client.h"
#include "others.h"
#include "SocketS.h"


//--> Global parameters
SOCKET m_socket; // comuunication socket
static my_username[MAX_LINE];
static HANDLE logfile_mutex; //log file mutex
HANDLE hThread[3]; // main threads
int game_started = 0, exit_signal = 0, user_accepted = 0, play_accepted = 0;
MsgQueue *msg_queue = NULL; // pointer to send message buffer

//Print Board function
void PrintBoard(int board[][BOARD_WIDTH])
{
	//This handle allows us to change the console's color
	HANDLE  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	int row, column;

	//Draw the board
	for (row = 0; row < BOARD_HEIGHT; row++)
	{
		for (column = 0; column < BOARD_WIDTH; column++)
		{
			printf("| ");
			if (board[row][column] == RED_PLAYER)
				SetConsoleTextAttribute(hConsole, RED);

			else if (board[row][column] == YELLOW_PLAYER)
				SetConsoleTextAttribute(hConsole, YELLOW);

			printf("O");

			SetConsoleTextAttribute(hConsole, BLACK);
			printf(" ");
		}
		printf("\n");

		//Draw dividing line between the rows
		for (column = 0; column < BOARD_WIDTH; column++)
		{
			printf("----");
		}
		printf("\n");
	}

	//free the handle
	//CloseHandle(hConsole);

}

//Reading data coming from the server Thread
static DWORD RecvDataThread(LPVOID lpParam)
{
	TransferResult_t RecvRes;
	struct Message *message = NULL;
	int board[BOARD_HEIGHT][BOARD_WIDTH] = { 0 }, type = 0;
	char input[MAX_LINE], *send_message = NULL;
	Data *info = NULL;
	DWORD wait_code;
	BOOL release_res;

	info = (Data *)lpParam;

	message = (struct Message*)malloc(sizeof(struct Message));

	while (1)
	{
		char *AcceptedStr = NULL;
		RecvRes = ReceiveString(&AcceptedStr, m_socket);

		if (RecvRes == TRNS_FAILED) { // If the server disconnected
			printf("Server disconnected. Exiting.\n");
			//---> Writing to logfile
			wait_code = WaitForSingleObject(logfile_mutex, INFINITE);
			if (wait_code != WAIT_OBJECT_0) printf("Fail while waiting for logfile mutex");

			//critical region
			fprintf(info->LogFile_ptr, "Server disconnected. Exiting.\n"); //Writing to logfile

			//end of critical region
			release_res = ReleaseMutex(logfile_mutex);
			if (release_res == FALSE) printf("Fail releasing logfile mutex");
			thread_terminator("clean"); // exit the programm with error
			return 0;
		}
		else if (RecvRes == TRNS_DISCONNECTED) {
			printf("Server disconnected. Exiting.\n");// If the server disconnected
			return 0;
		}
		else { // We recived a new message
			MessageEval(message, AcceptedStr); // Deconstructing the message (AcceptedStr)
			//---> Writing to logfile
			PrintToLogFile(info->LogFile_ptr, "Received from server", AcceptedStr);

			if (game_started == 0) {// Game NOT Started!
				if (STRINGS_ARE_EQUAL("GAME_STARTED", message->message_type)) {
					printf("Game is on!\n");
					game_started = 1;
				}
				if (STRINGS_ARE_EQUAL("NEW_USER_DECLINED", message->message_type)) {
					printf("Request to join was refused\n");
					exit_signal = 1;
					thread_terminator("clean"); // exit the programm with error
					return 0;
				}
				if (STRINGS_ARE_EQUAL("NEW_USER_ACCEPTED", message->message_type)) {
					printf("Username accepted\n");
					printf("Number of players: %s\n", message->parameters->parameter);
					user_accepted = 1;
				}
			}
			else { // Game Started!
				if (STRINGS_ARE_EQUAL("BOARD_VIEW", message->message_type)) {
					if (message->parameters->next != NULL) { // If we got board update
						if (STRINGS_ARE_EQUAL("RED", message->parameters->next->next->parameter)) // Checking the player who uptated the board
							board[atoi(message->parameters->parameter)][atoi(message->parameters->next->parameter)] = RED_PLAYER; // Updating the board with RED
						if (STRINGS_ARE_EQUAL("YELLOW", message->parameters->next->next->parameter)) // Checking the player who uptated the board
							board[atoi(message->parameters->parameter)][atoi(message->parameters->next->parameter)] = YELLOW_PLAYER; // Updating the board with YELLOW
					}
					PrintBoard(board);
				}
				if (STRINGS_ARE_EQUAL("TURN_SWITCH", message->message_type)) {
					printf("%s's turn\n", message->parameters->parameter);
					if (STRINGS_ARE_EQUAL(message->parameters->parameter, my_username)) { // If it is MY TURN
						//---> file mode option
						if (STRINGS_ARE_EQUAL(info->input_mode, "file")) { // If we are in file mode
							type = 0; //Reseting type
							while (type != 1) { //While we didn't read from file play message
								fgets(input, MAX_LINE, info->InputFile_ptr); //Reading a string from file
								if (STRINGS_ARE_EQUAL(input, "\n")) // If we read a blank line
									continue;
								printf("%s", input);
								type = MessageType(input); // Checking the type of the input (play or message)

								//----> Play input
								if (type == 1) {
									send_message = ConstructMessage(input, "play"); //Constructing the message to send to the server
									//-> send play to send buffer
									EnqueueMsg(msg_queue, send_message);
								}
								//----> Exit input
								if (STRINGS_ARE_EQUAL(input, "exit\n") || STRINGS_ARE_EQUAL(input, "exit")) {
									//-> send exit to send buffer
									EnqueueMsg(msg_queue, send_message);

									//---> Writing to logfile
									PrintToLogFile(info->LogFile_ptr, "Custom message", "Player entered exit input...");

									thread_terminator("clean"); // exit the programm with error
									return 0; // Exit the while loop
								}
								//----> Message input
								if (type == 2) {
									send_message = ConstructMessage(input, "message"); //Constructing the message to send to the server
									//-> send play to send buffer
									EnqueueMsg(msg_queue, send_message);
								}
								//---> Writing to logfile
								if (STRINGS_ARE_EQUAL("Error: Illegal command", send_message))
									PrintToLogFile(info->LogFile_ptr, "Custom message", send_message);
								else
									PrintToLogFile(info->LogFile_ptr, "Sent to server", send_message);
							}
						}
					}
				}
				if (STRINGS_ARE_EQUAL("PLAY_ACCEPTED", message->message_type)) {
					printf("Well played\n");
					play_accepted = 1;
				}
				if (STRINGS_ARE_EQUAL("PLAY_DECLINED", message->message_type)) {
					printf("Error: ");
					while (message->parameters->parameter != NULL) {
						printf("%s", message->parameters->parameter);
						message->parameters = message->parameters->next;
					}
					printf("\n");
					play_accepted = 0;
				}
				if (STRINGS_ARE_EQUAL("GAME_ENDED", message->message_type)) {
					if (STRINGS_ARE_EQUAL(message->parameters->parameter, "tied")) // We got tie
						printf("Game ended. Everybody wins!\n");
					else
						printf("Game ended. The winner is %s!\n", message->parameters->parameter); //Print the winner name
					exit_signal = 1;

					//---> Writing to logfile
					PrintToLogFile(info->LogFile_ptr, "Received from server", AcceptedStr);
					PrintToLogFile(info->LogFile_ptr, "Custom message", "Game ended. Exiting...");

					thread_terminator("clean"); // Clean exit the programm
					return 0; // Exit the while loop
				}
				if (STRINGS_ARE_EQUAL("RECEIVE_MESSAGE", message->message_type)) {
					printf("%s: ", message->parameters->parameter);
					message->parameters = message->parameters->next;
					while (message->parameters->parameter != NULL) {
						printf("%s", message->parameters->parameter);
						message->parameters = message->parameters->next;
					}
					printf("\n");
				}
			}
		}
		//free(send_message);
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
	char input[MAX_LINE], *send_message = NULL;
	int run = 0, type = 0;
	Data *info = NULL;
	DWORD wait_code;
	BOOL release_res;

	info = (Data *)lpParam;

	while (1)
	{
		if ((game_started == 0) && (user_accepted == 0) && (run == 0)) // Username input for user
		{
			run++;
			printf("Please enter a new username:\n");
			if (STRINGS_ARE_EQUAL(info->input_mode, "file")) { //If we are in FILE mode
				fgets(input, MAX_LINE, info->InputFile_ptr); //Reading a string from file
				printf("%s", input);
				input[strlen(input + 1)] = '\0'; //Deleting "\n"
			}
			else { //If we are in HUMAN mode
				gets_s(input, sizeof(input)); //Reading a string from the keyboard
			}
			strcpy(my_username, input); //Saving my username
			send_message = ConstructMessage(input, "username"); //Constructing the message to send to the server
			//----->sending to buffer
			EnqueueMsg(msg_queue, send_message);

			//---> Writing to logfile
			PrintToLogFile(info->LogFile_ptr, "Sent to server", send_message);

		}
		if ((game_started == 1) && (user_accepted == 1)) { //Game started!
			if (STRINGS_ARE_EQUAL(info->input_mode, "human")) { // If we are in human mode
				gets_s(input, sizeof(input)); //Reading a string from the keyboard

				type = MessageType(input); // Checking the type of the input (play or message)

				//----> Play input
				if (type == 1) {
					send_message = ConstructMessage(input, "play"); //Constructing the message to send to the server
					//-> send play to send buffer
					EnqueueMsg(msg_queue, send_message);
				}
				//----> Exit input
				else if (STRINGS_ARE_EQUAL(input, "exit")) {
					//-> send exit to send buffer
					EnqueueMsg(msg_queue, send_message);

					//---> Writing to logfile
					PrintToLogFile(info->LogFile_ptr, "Custom message", "Player entered exit input...");

					thread_terminator("clean"); // Clean exit the programm
					return 0;
				}
				//----> Message input
				else if (type == 2) {
					send_message = ConstructMessage(input, "message"); //Constructing the message to send to the server
					//-> send play to send buffer
					EnqueueMsg(msg_queue, send_message);
				}
				else {
					printf("Error: Illegal command\n");
					send_message = "Error: Illegal command";
				}
				//---> Writing to logfile
				if (STRINGS_ARE_EQUAL("Error: Illegal command", send_message))
					PrintToLogFile(info->LogFile_ptr, "Custom message: User input error", input);
				else
					PrintToLogFile(info->LogFile_ptr, "Sent to server", send_message); //Writing to logfile

			}
		}
		//free(send_message);
	}
	return 0;
}

//Main thread that inisilatize all the sockets, programs and opens the send, recive and API threads
int MainClient(char *logfile, char *server_port, char *input_mode, char *input_file)
{
	int server_port_int = 0;
	SOCKADDR_IN clientService;
	Data *information = NULL;
	FILE *ptr = NULL, *ptr1 = NULL;
	DWORD exitcode;

	// Initialize Winsock.
	WSADATA wsaData; //Create a WSADATA object called wsaData.
	//The WSADATA structure contains information about the Windows Sockets implementation.

	//Call WSAStartup and check for errors.
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");

	//Call the socket function and return its value to the m_socket variable. 
	// For this application, use the Internet address family, streaming sockets, and the TCP/IP protocol.

	// Create a socket.
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	if (m_socket == INVALID_SOCKET) {
		printf("Failed connecting to server on %s:%s. Exiting\n", SERVER_ADDRESS_STR, server_port);
		WSACleanup();
		exit(0X555);
	}
	/*
	 The parameters passed to the socket function can be changed for different implementations.
	 Error detection is a key part of successful networking code.
	 If the socket call fails, it returns INVALID_SOCKET.
	 The if statement in the previous code is used to catch any errors that may have occurred while creating
	 the socket. WSAGetLastError returns an error number associated with the last error that occurred.
	 */


	 //For a client to communicate on a network, it must connect to a server.
	 // Connect to a server.

	server_port_int = atoi(server_port); //Converting server port to int

	 //Create a sockaddr_in object clientService and set values.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(SERVER_ADDRESS_STR); //Setting the IP address to connect to
	clientService.sin_port = htons(server_port_int); //Setting the port to connect to.

	/*
		AF_INET is the Internet address family.
	*/


	// Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	if (connect(m_socket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
		printf("Failed connecting to server on %s:%s. Exiting\n", SERVER_ADDRESS_STR, server_port);
		WSACleanup();
		exit(0x555);
	}
	else
		printf("Connected to server on %s:%s\n\n", SERVER_ADDRESS_STR, server_port);

	// Send and receive data.
	/*
		In this code, two integers are used to keep track of the number of bytes that are sent and received.
		The send and recv functions both return an integer value of the number of bytes sent or received,
		respectively, or an error. Each function also takes the same parameters:
		the active socket, a char buffer, the number of bytes to send or receive, and any flags to use.

	*/

	information = (struct Data*)malloc(sizeof(struct Data)); // Malloc the data struct to send to functions
	ptr = fopen(logfile, "w"); //Opening log file
	if (ptr == NULL) {
		printf("Error opening the client log file for writing\n");
		exit(-1);
	}
	information->LogFile_ptr = ptr;
	if (STRINGS_ARE_EQUAL("human", input_mode)) {
		information->input_mode = "human";
		information->InputFile_ptr = NULL;
	}
	if (STRINGS_ARE_EQUAL("file", input_mode)) {
		information->input_mode = "file";
		ptr1 = fopen(input_file, "r"); //Opening log file
		if (ptr1 == NULL) {
			printf("Error opening the client input file for reading\n");
			exit(-1);
		}
		information->InputFile_ptr = ptr1;
	}

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
		information,
		0,
		NULL
	);
	hThread[2] = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)ApplicationThread,
		information,
		0,
		NULL
	);

	WaitForMultipleObjects(3, hThread, FALSE, INFINITE);

	GetExitCodeThread(hThread[0], &exitcode); // Getting the error code from threads

	//---> Closing handles and free memory

	CloseHandle(hThread[0]); // SendDataThread
	CloseHandle(hThread[1]); // RecvDataThread
	CloseHandle(hThread[2]); // ApplicationThread

	closesocket(m_socket); // Close socket

	WSACleanup();

	fclose(ptr); // Close log file
	if (ptr1 != NULL)
		fclose(ptr1); // Close input file

	if (logfile_mutex != NULL) CloseHandle(logfile_mutex); // Close logfile mutex handle

	CloseHandle(msg_queue->access_mutex); // Message queue access_mutex
	CloseHandle(msg_queue->msgs_count_semaphore); // Message queue msgs_count_semaphore
	CloseHandle(msg_queue->queue_empty_event); // Message queue queue_empty_event
	CloseHandle(msg_queue->stop_event); // Message queue stop_event
	free(msg_queue); // Free Message queue

	if (exitcode == 0) // The programm ended clean
		return 0;
	else // The programm got an error
		return 0x555;
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