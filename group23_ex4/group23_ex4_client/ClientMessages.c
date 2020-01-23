#include <string.h>
#include <stdlib.h>
#include "ClientMessages.h"
#include "MessageQueue.h"

int SendClientRequestMessage(char* username, message_queue_t* message_queue)
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
	EnqueueMsg(message_queue, message_string);
}