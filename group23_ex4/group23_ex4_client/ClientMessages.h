#ifndef CLIENT_MESSAGES_H
#define CLIENT_MESSAGES_H

#include "MessageQueue.h"

int SendClientRequestMessage(char* username, message_queue_t* message_queue);

#endif
