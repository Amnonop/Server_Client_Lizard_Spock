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