#ifndef PLAYER_VS_PLAYER_H
#define PLAYER_VS_PLAYER_H

#include <WinSock2.h>
#include "MessageQueue.h"

int PlayerVsPlayer(SOCKET socket, message_queue_t* msg_queue);

#endif
