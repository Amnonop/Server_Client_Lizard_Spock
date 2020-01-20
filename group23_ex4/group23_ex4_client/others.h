#ifndef MAIN_H
#define MAIN_H

#include "client.h"
//#include "server.h"

#include <windows.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdbool.h>
#include<math.h>

#define MAX_MSG 256

typedef struct param_node {
	char *param_value;
	struct param_node_t *next;
} param_node_t;

typedef struct message {
	char *message_type;
	param_node_t *parameters;
} message_t;

void MessageEval(message_t *m, char *AcceptedStr);

#endif