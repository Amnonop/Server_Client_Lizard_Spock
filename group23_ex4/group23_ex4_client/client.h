#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <Windows.h>

#define SERVER_PORT 2345
#define STRINGS_ARE_EQUAL( Str1, Str2 ) ( strcmp( (Str1), (Str2) ) == 0 )

#define MAX_BUFFER 5
#define MAX_LINE 256
#define QUEUE_ERROR -10

#define RED_PLAYER 1
#define YELLOW_PLAYER 2

#define BOARD_HEIGHT 6
#define BOARD_WIDTH  7

#define BLACK  15
#define RED    204
#define YELLOW 238

// Struct which contains the data to send to the threads: <file pointer to logfile> <input mode> <file pointer to input file>
typedef struct client_thread_params 
{
	char* username;
	char* server_ip;
	int server_port;
} client_thread_params_t;

// Struct which contains a message (in MsgQueue) sent from user application thread to send thread: <char pointer data - string> <pointer to next>
typedef struct MsgNode {
	char *data;
	struct MsgNode *next;
}MsgNode;

// Struct which contains the message queue sent from user application thread to send thread: <head pointer to MsgNode message > <handels to mutexs and semaphores>
typedef struct MsgQueue {
	struct MsgNode *head;
	HANDLE access_mutex;
	HANDLE msgs_count_semaphore;
	HANDLE queue_empty_event;
	HANDLE stop_event;
}MsgQueue;

void PrintBoard(int board[][BOARD_WIDTH]);
char* ConstructMessage(char *str, char *type);
int MessageType(char *input);
MsgQueue *msg_queue_creator();
void EnqueueMsg(MsgQueue *msg_queue, char *msg);
char *DequeueMsg(MsgQueue *msg_queue);
void thread_terminator(char *type);
void PrintToLogFile(FILE *ptr, char *format, char *message);
int MainClient(char* server_ip, int port_number, char* username);

#endif