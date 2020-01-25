#ifndef CLIENT_MESSAGES_H
#define CLIENT_MESSAGES_H

#include "MessageQueue.h"
#include "../Shared/ClientSrvCommons.h"

int SendClientRequestMessage(char* username, message_queue_t* message_queue);
int SendClientCPUMessage(message_queue_t* message_queue);
int SendClientVersusMessage(message_queue_t* message_queue);
int SendClientLeaderBoardMessage(message_queue_t* message_queue);
int SendClientQuitMessage(message_queue_t* message_queue);
int SendPlayerMoveMessage(MOVE_TYPE player_move, message_queue_t* message_queue);
int SendClientReplayMessage(message_queue_t* message_queue);
int SendMainMenuMessage(message_queue_t* message_queue);
int SendClientMessageWithoutParams(const char* message_name, message_queue_t* message_queue);
int SendLeaderboardMessage(message_queue_t* message_queue);

#endif
