#include <string.h>
#include <stdio.h>
#include "StringTools.h"

int CopyString(const char* source, char** destination)
{
	int copy_length;

	copy_length = strlen(source) + 1;
	*destination = (char*)malloc(sizeof(char)*copy_length);
	if (*destination == NULL)
	{
		printf("Allocation failed.\n");
		return STR_ALLOCATION_ERROR;
	}

	strcpy_s(*destination, copy_length, source);
	return 0;
}