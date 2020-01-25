#ifndef SERVER_MESSAGES_H
#define SERVER_MESSAGES_H

#include <WinSock2.h>
#include "../Shared/ClientSrvCommons.h"
#include "../Shared/MessageTools.h"

int SendServerApprovedMessage(SOCKET socket);
int SendDeniedMessage(SOCKET socket);
int SendMainMenuMessage(SOCKET socket);
int SendPlayerMoveRequestMessage(SOCKET socket);
int ParsePlayerMoveMessage(message_t* message, MOVE_TYPE* player_move);
int SendGameResultsMessage(const char* oponent_username, MOVE_TYPE oponent_move, MOVE_TYPE player_move, const char* winner_name, SOCKET socket);
int SendGameOverMenu(SOCKET socket);
int SendNoOponentsMessage(SOCKET socket);
int SendServerInviteMessage(char* oponent_name, SOCKET socket);
char* MoveTypeToString(MOVE_TYPE move);
int SendOpponentQuitMessage(const char* oponnent_name, SOCKET socket);

#endif
