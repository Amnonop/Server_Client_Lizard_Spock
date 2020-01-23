#ifndef CLIENT_MESSAGES_H
#define CLIENT_MESSAGES_H

#include "MessageQueue.h"
#include "../Shared/ClientSrvCommons.h"

int SendClientRequestMessage(char* username, message_queue_t* message_queue);
int SendClientCPUMessage(message_queue_t* message_queue);
int SendPlayerMoveMessage(MOVE_TYPE player_move, message_queue_t* message_queue);

#endif
