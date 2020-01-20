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

typedef struct parameters {
	char *param_value;
	struct parameters *next;
} parameters_t;

typedef struct message {
	char *message_type;
	parameters_t *parameters;
} message_t;

void MessageEval(message_t *m, char *AcceptedStr);

#endif