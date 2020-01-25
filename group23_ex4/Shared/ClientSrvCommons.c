#include <string.h>
#include "ClientSrvCommons.h"

char* MoveTypeToString(MOVE_TYPE move)
{
	switch (move)
	{
	case (ROCK):
		return "ROCK";
		break;
	case (PAPER):
		return "PAPER";
		break;
	case(SCISSORS):
		return "SCISSORS";
		break;
	case (LIZARD):
		return "LIZARD";
		break;
	case (SPOCK):
		return "SPOCK";
		break;
	default:
		break;
	}
}

MOVE_TYPE StringToMoveType(const char* move_type_string)
{
	if (STRINGS_ARE_EQUAL(move_type_string, "ROCK"))
	{
		return ROCK;
	}
	else if (STRINGS_ARE_EQUAL(move_type_string, "PAPER"))
	{
		return PAPER;
	}
	else if (STRINGS_ARE_EQUAL(move_type_string, "SCISSORS"))
	{
		return SCISSORS;
	}
	else if (STRINGS_ARE_EQUAL(move_type_string, "LIZARD"))
	{
		return LIZARD;
	}
	else if (STRINGS_ARE_EQUAL(move_type_string, "SPOCK"))
	{
		return SPOCK;
	}
	else
	{
		printf("Invalid move type %s.\n", move_type_string);
		return MOVE_TYPE_NONE;
	}
}