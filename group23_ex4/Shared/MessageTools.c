#include <string.h>
#include <stdio.h>
#include "MessageTools.h"
#include "../Shared/StringTools.h"

param_node_t* AddParameter(param_node_t* head, const char* param_value, int value_length);
param_node_t* CreateParameter(const char* param_value, int value_length);

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

	exit_code = CopyString(raw_string, &raw_message_copy);

	// Initialize param linked list
	message->parameters = NULL;

	token = strtok_s(raw_message_copy, message_type_delim, &next_token);
	
	// Check if the message has parameters
	if (strlen(token) == strlen(raw_message_copy))
		message_type_length = strlen(token); // Copy without '\n'
	else
		message_type_length = strlen(token) + 1;
	
	message->message_type = (char*)malloc(sizeof(char)*message_type_length);
	strcpy_s(message->message_type, message_type_length, token);

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
		strcpy_s(new_node->param_value, value_length, param_value);

		new_node->next = NULL;
	}
	return new_node;
}