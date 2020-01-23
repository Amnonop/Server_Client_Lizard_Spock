#ifndef MESSAGE_TOOLS_H
#define MESSAGE_TOOLS_H

typedef enum
{
	MSG_SUCCESS,
	MSG_MEM_ALLOC_FAILED,
	MSG_TRANS_FAILED
} MESSAGE_CODES;

typedef struct param_node {
	char *param_value;
	struct param_node_t *next;
} param_node_t;

typedef struct message {
	char *message_type;
	param_node_t *parameters;
} message_t;

int GetMessageStruct(message_t *message, char *raw_string);

#endif
