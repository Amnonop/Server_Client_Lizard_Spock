#ifndef CLIENT_GET_MESSAGES_H
#define CLIENT_GET_MESSAGES_H

typedef struct game_results
{
	char* oponent_name;
	char* oponent_move;
	char* player_move;
	char* winner;
} game_results_t;

int GetGameResultsMessage(SOCKET socket, game_results_t** game_results);
int GetGameOverMenuMessage(SOCKET socket);
void FreeGameResults(game_results_t* game_results);
int GetStartGameMessage(BOOL* start_game, char** oponent_name, SOCKET socket);
int GetReplayStatus(BOOL* replay, SOCKET socket);

#endif
