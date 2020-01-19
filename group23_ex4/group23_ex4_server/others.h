#ifndef MAIN_H
#define MAIN_H

//#include "client.h"
#include "server.h"

#include <windows.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdbool.h>
#include<math.h>

#define MAX_MSG 256

typedef struct Parameters {
	char *parameter;
	struct Parameters *next;
}Parameters;

typedef struct Message {
	char *message_type;
	Parameters *parameters;
}Message;

void MessageEval(Message *m, char *AcceptedStr);

typedef struct Messages_1 {
	char *message_type;
	char *text;
	char *parameters
}Messages_1;

#endif