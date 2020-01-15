#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h>
#include <Windows.h>

#define SERVER_ADDRESS_STR "127.0.0.1"

#define RED_PLAYER 1
#define YELLOW_PLAYER 2

#define BOARD_HEIGHT 6
#define BOARD_WIDTH  7

#define MAX_MESSAGE_SIZE 100
#define DISCONNECTED -1
#define NUM_OF_WORKER_THREADS 2
#define SEND_STR_SIZE 35

#define ERROR_CODE -100

#define STRINGS_ARE_EQUAL( Str1, Str2 ) ( strcmp( (Str1), (Str2) ) == 0 )


// User- will help save all player's relevant information- who's turn it is, his name, his color and his socket number
typedef struct User {
	char user_name[30];
	int user_color;
	bool my_turn;
	SOCKET user_socket;
}User;

/* MainServer- will be called from main thread in case of server arguments. it will raise up all threads, all events and mutexes helping in
playing the game peacfully. it will not be closed by any reason except outsize interrupt*/
void MainServer(char *log_file, char *port_number);

/*FindFirstUnusedThreadSlot- will help in finding the first place in threads array when a new user will ask to join*/
static int FindFirstUnusedThreadSlot();

/*CleanupWorkerThreads- when finishing the program this function will rlrase the users threads.*/
static void CleanupWorkerThreads();

/*ManagerThread- will be used after every move. controled by event which is released after a player move, it will send board status, it will
switch turns and finish the game in case of tie or a winner*/
DWORD ManagerThread(LPVOID lpParam);

/*RegularMessages- will be used each time a regular message is sent. controled by event which is released when a player sends message, and send
it to the other player.*/
DWORD RegularMessages(LPVOID lpParam);

/*Disconnection- in case one of the players decides to disconnect, this thread which is controled by an event will come into play.
it will reset all the game aspects, threads and global variables, for the server to wait for a new game.*/
DWORD Disconnection(LPVOID lpParam);

/*CheckUserName- will get a new user name request and check his availability. In case it is accepted, i will save all relevant information of
the new player in a global user array- 'user_names'*/
int CheckUserName(char *user_name, SOCKET t_socket);

/*DetermineIfWinnerShot- this finction will be executed after each move from 'ManagerThread' to check if the game is finished.*/
int DetermineIfWinnerShot();

/*ServiceThread- this thread is opened twice in each game by 'MainServer'. it will controls the game=- checks the message type and handling it.
it releases events, and controls the start of the game.*/
DWORD ServiceThread(int num);

/*FreeAndClean- will be used to clean up all threads, mutexes and events handles that where opened in case of a fatal error.*/
void FreeAndClean();

#endif // SERVER_H