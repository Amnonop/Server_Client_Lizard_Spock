#include <string.h>
#include <stdio.h>
#include "MessageTools.h"

param_node_t* AddNode(param_node_t* head, char* param_value);
param_node_t* CreateNode(char* param_value);



//Function that gets the raw message and break it into the Message struct without :/;
int GetMessageStruct(message_t *message, char *raw_string)
{
	int exit_code = 0;
	const char* message_type_delim = ":";
	const char* param_delim = ";";
	char* token;
	char* next_token;
	int message_type_length;
	char* end;
	int end_index;
	int i = 0;
	int message_type_index = -1;
	int lase_param_index = -1;

	char* param_value;
	int param_start_index;
	int param_end_index;
	int param_length;

	char* param_delim_loc;
	int param_delim_index;

	end = strchr(raw_string, '\n');
	end_index = end - raw_string;

	// Initialize param linked list
	message->parameters = NULL;

	while (raw_string[i] != '\n')
	{
		if (raw_string[i] == ':')
		{
			// Message type
			message_type_index = i;
		}
	}

	// Check if the message contains parameters
	if (message_type_index != -1)
	{
		// Find the first occurence of ';'
		param_delim_loc = strchr(raw_string, ';');
		if (param_delim_loc != NULL)
		{

		}
	}

	token = strtok_s(raw_string, message_type_delim, &next_token);
	message_type_length = strlen(token) + 1;
	message->message_type = (char*)malloc(sizeof(char)*message_type_length);
	strcpy_s(message->message_type, message_type_length, token);

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

	return exit_code;
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