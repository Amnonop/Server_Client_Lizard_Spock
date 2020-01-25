#include <stdio.h>
#include "GameSession.h"
#include "Commons.h"
#include "../Shared/ClientSrvCommons.h"

int OpenNewFile(const char* path)
{
	FILE* file;

	// Check if the file exists
	file = fopen_s(&file, path, "r");
	if (file == NULL)
	{
		// Open the file
		file = fopen_s(&file, path, "w");
		if (file == NULL)
		{
			return SERVER_FILE_OPEN_FAILED;
		}

		fclose(file);
		return SERVER_SUCCESS;
	}

	fclose(file);
	return SERVER_FILE_EXISTS;
}

int WriteMoveToGameSession(const char* game_session_path, MOVE_TYPE move, const char* username)
{
	FILE* file;
	char* move_string;

	// Check if the file exists
	file = fopen_s(&file, game_session_path, "a");
	if (file == NULL)
	{
		return SERVER_FILE_OPEN_FAILED;
	}

	move_string = MoveTypeToString(move);
	fprintf_s(file, "%s:%s\n", username, move_string);

	fclose(file);
	return SERVER_SUCCESS;
}

int ReadOponnentMoveFromGameSession(const char* game_session_path, const char* oponnent_name, MOVE_TYPE* move)
{
	FILE* file;
	char move_line[45];
	const char* delimiter = ":";
	char* move_line_copy;
	char* token;
	char* next_token;

	// Check if the file exists
	file = fopen_s(&file, game_session_path, "a");
	if (file == NULL)
	{
		return SERVER_FILE_OPEN_FAILED;
	}

	while (fgets(move_line, sizeof(move_line), file) != NULL)
	{
		move_line_copy = CopyString(move_line);
		if (move_line_copy == NULL)
		{
			fclose(file);
			return SERVER_MEM_ALLOC_FAILED;
		}

		token = strtok_s(move_line_copy, delimiter, &next_token);
		if (STRINGS_ARE_EQUAL(token, oponnent_name))
		{
			token = strtok_s(NULL, delimiter, &next_token);
			*move = StringToMoveType(token);
			free(move_line_copy);
			break;
		}
	}

	fclose(file);
	return SERVER_SUCCESS;
}