#ifndef SERVER_GET_MESSAGES_H
#define SERVER_GET_MESSAGES_H

#include <WinSock2.h>
#include "../Shared/ClientSrvCommons.h"

int GetPlayerMainMenuChoice(SOCKET socket, MAIN_MENU_OPTIONS* user_choice);
int GetPlayerGameOverMenuChoice(SOCKET client_socket, GAME_OVER_MENU_OPTIONS* user_choice);
int GetPlayerMove(SOCKET client_socket, MOVE_TYPE* player_move);

#endif
