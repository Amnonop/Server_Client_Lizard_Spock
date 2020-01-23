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

void PrintBoard(int board[][BOARD_WIDTH]);
char* ConstructMessage(char *str, char *type);
int MessageType(char *input);
void thread_terminator(char *type);
void PrintToLogFile(FILE *ptr, char *format, char *message);
int MainClient(char* server_ip, int port_number, char* username);

#endif