#ifndef SERVER_MESSAGES_H
#define SERVER_MESSAGES_H

#include <WinSock2.h>
#include "../Shared/ClientSrvCommons.h"
#include "../Shared/MessageTools.h"

int GetPlayerMove(SOCKET client_socket, MOVE_TYPE* player_move);
int ParsePlayerMoveMessage(message_t* message, MOVE_TYPE* player_move);
int SendGameResultsMessage(const char* oponent_username, MOVE_TYPE oponent_move, MOVE_TYPE player_move, const char* winner_name, SOCKET socket);
int SendGameOverMenu(SOCKET socket);
int ReceiveMessage(SOCKET client_socket, message_t* message);

#endif
