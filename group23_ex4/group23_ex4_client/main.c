/*Authors – Alon Moses - 308177815, Ido Debi - 204301758.
Project – exercise4.*/
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "others.h"

//Function that gets the raw message and break it into the Message struct without :/;
void MessageEval(Message *m, char *AcceptedStr) {
	char AcceptedStrHelp[MAX_MSG];
	strcpy(AcceptedStrHelp, AcceptedStr);
	Parameters *iter = malloc(sizeof(Parameters));
	m->message_type = malloc(sizeof(char) * 100);
	m->parameters = malloc(sizeof(Parameters));
	m->parameters->parameter = malloc(sizeof(char) * 100);
	m->parameters->next = NULL;
	char *message = strtok(AcceptedStrHelp, ":");
	strcpy(m->message_type, message);
	iter = m->parameters;
	while (message != NULL) {
		message = strtok(NULL, ";");
		if (message == NULL)
		{
			iter->parameter = NULL;
			continue;
		}
		strcpy(iter->parameter, message);
		iter->next = malloc(sizeof(Parameters));
		iter = iter->next;
		iter->parameter = malloc(sizeof(char) * 100);
	}
}

int main(int argc, char* argv[]) {
	int exitcode = -1;

	if (STRINGS_ARE_EQUAL(argv[1], "server")) // If we recived server, starting MainServer
		MainServer(argv[2], argv[3]);

	if (STRINGS_ARE_EQUAL(argv[1], "client")) // If we recived client, starting MainClient
		exitcode = MainClient(argv[2], argv[3], argv[4], argv[5]);

	return exitcode; // Returns 0 if returned sucsessfuly, 0x555 else
}