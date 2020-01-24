#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include "MessageTools.h"
#include "../Shared/StringTools.h"
#include "../Shared/socketS.h"

param_node_t* AddParameter(param_node_t* head, const char* param_value, int value_length);
param_node_t* CreateParameter(const char* param_value, int value_length);

int ReceiveMessageWithTimeout(SOCKET socket, message_t** message, long timeout_seconds)
{
	TransferResult_t receive_result;
	char* accepted_string = NULL;

	*message = (message_t*)malloc(sizeof(message_t));
	if (*message == NULL)
	{
		printf("Failed to allocate memory.\n");
		return MSG_MEM_ALLOC_FAILED;
	}

	// Waiting for CLIENT_REQUEST message
	receive_result = ReceiveStringWithTimeout(&accepted_string, socket, timeout_seconds);
	if (receive_result == TRNS_FAILED)
	{
		printf("Target disconnected. Ending communication.\n");
		return MSG_TRANS_FAILED;
	}
	else if (receive_result == TRNS_DISCONNECTED)
	{
		printf("Target disconnected. Ending communication.\n");
		return MSG_TRANS_FAILED;
	}
	else if (receive_result == TRNS_TIMEOUT)
	{
		printf("Timeout while waiting for response.\n");
		return MSG_TIMEOUT;
	}

	printf("Received message: %s\n", accepted_string);
	GetMessageStruct(*message, accepted_string);

	free(accepted_string);
	return MSG_SUCCESS;
}

int ReceiveMessage(SOCKET socket, message_t** message)
{
	TransferResult_t receive_result;
	char* accepted_string = NULL;

	*message = (message_t*)malloc(sizeof(message_t));
	if (*message == NULL)
	{
		printf("Failed to allocate memory.\n");
		return MSG_MEM_ALLOC_FAILED;
	}

	// Waiting for CLIENT_REQUEST message
	receive_result = ReceiveString(&accepted_string, socket);
	if (receive_result == TRNS_FAILED)
	{
		printf("Target disconnected. Ending communication.\n");
		return MSG_TRANS_FAILED;
	}
	else if (receive_result == TRNS_DISCONNECTED)
	{
		printf("Target disconnected. Ending communication.\n");
		return MSG_TRANS_FAILED;
	}
	else if (receive_result == TRNS_TIMEOUT)
	{
		printf("Timeout while waiting for response.\n");
		return MSG_TIMEOUT;
	}

	printf("Received message: %s\n", accepted_string);
	GetMessageStruct(*message, accepted_string);

	free(accepted_string);
	return MSG_SUCCESS;
}

//Function that gets the raw message and break it into the Message struct without :/;
int GetMessageStruct(message_t *message, const char *raw_string)
{
	int exit_code = 0;
	const char* message_type_delim = ":";
	const char* param_delim = ";";
	char* raw_message_copy = NULL;
	char* token;
	char* next_token;
	int message_type_length;
	char* termination_char;
	int param_length;

	raw_message_copy = CopyString(raw_string);

	// Initialize param linked list
	message->parameters = NULL;

	token = strtok_s(raw_message_copy, message_type_delim, &next_token);
	
	// Check if the message has parameters
	if (strlen(token) == strlen(raw_string))
		message_type_length = strlen(token); // Copy without '\n'
	else
		message_type_length = strlen(token) + 1;
	
	message->message_type = (char*)malloc(sizeof(char)*message_type_length);
	strncpy_s(message->message_type, message_type_length, token, message_type_length - 1);

	token = strtok_s(NULL, param_delim, &next_token);
	while (token)
	{
		// Add a new parameter node
		// TODO: Handle the '\n' termination -> remove from the last parameter
		termination_char = strchr(token, '\n');

		// Check if this is the last parameter
		if (termination_char != NULL)
			param_length = strlen(token);
		else
			param_length = strlen(token) + 1;

		message->parameters = AddParameter(message->parameters, token, param_length);
		if (message->parameters == NULL)
		{
			// TODO: Allocation failed
		}

		token = strtok_s(NULL, param_delim, &next_token);
	}

	return exit_code;
}

param_node_t* AddParameter(param_node_t* head, const char* param_value, int value_length)
{
	param_node_t* tail;
	param_node_t* new_node;

	new_node = CreateParameter(param_value, value_length);
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

param_node_t* CreateParameter(const char* param_value, int value_length)
{
	param_node_t* new_node = (param_node_t*)malloc(sizeof(param_node_t));
	if (new_node != NULL)
	{
		new_node->param_value = (char*)malloc(sizeof(char)*value_length);
		strncpy_s(new_node->param_value, value_length, param_value, value_length - 1);

		new_node->next = NULL;
	}
	return new_node;
}