#ifndef GAME_SESSION_H
#define GAME_SESSION_H

#include "../Shared/ClientSrvCommons.h"

int OpenNewFile(const char* path);
int RemoveFile(const char* path);
int WriteMoveToGameSession(const char* game_session_path, MOVE_TYPE move, const char* username);
int ReadOponnentMoveFromGameSession(const char* game_session_path, const char* oponnent_name, MOVE_TYPE* move);

#endif
