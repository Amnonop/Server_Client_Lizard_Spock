/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
/*
 This file was written for instruction purposes for the
 course "Introduction to Systems Programming" at Tel-Aviv
 University, School of Electrical Engineering, Winter 2011,
 by Amnon Drory, based on example code by Johnson M. Hart.
*/
/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

#include "socketS.h"

#include <stdio.h>
#include <string.h>

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

typedef enum
{
	SELECT_ERROR_OCCURRED = -1,
	SELECT_TIMEOUT = 0,
	SELECT_DATA_READY
};

TransferResult_t SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd)
{
	const char* CurPlacePtr = Buffer;
	int BytesTransferred;
	int RemainingBytesToSend = BytesToSend;

	while (RemainingBytesToSend > 0)
	{
		/* send does not guarantee that the entire message is sent */
		BytesTransferred = send(sd, CurPlacePtr, RemainingBytesToSend, 0);
		if (BytesTransferred == SOCKET_ERROR)
		{
			printf("send() failed, error %d\n", WSAGetLastError());
			return TRNS_FAILED;
		}

		RemainingBytesToSend -= BytesTransferred;
		CurPlacePtr += BytesTransferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

TransferResult_t SendString(const char *Str, SOCKET sd)
{
	/* Send the the request to the server on socket sd */
	int TotalStringSizeInBytes;
	TransferResult_t SendRes;

	/* The request is sent in two parts. First the Length of the string (stored in
	   an int variable ), then the string itself. */

	TotalStringSizeInBytes = (int)(strlen(Str) + 1); // terminating zero also sent	

	SendRes = SendBuffer(
		(const char *)(&TotalStringSizeInBytes),
		(int)(sizeof(TotalStringSizeInBytes)), // sizeof(int) 
		sd);

	if (SendRes != TRNS_SUCCEEDED) return SendRes;

	SendRes = SendBuffer(
		(const char *)(Str),
		(int)(TotalStringSizeInBytes),
		sd);

	return SendRes;
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

TransferResult_t ReceiveBuffer(char* OutputBuffer, int BytesToReceive, SOCKET sd)
{
	char* CurPlacePtr = OutputBuffer;
	int BytesJustTransferred;
	int RemainingBytesToReceive = BytesToReceive;

	while (RemainingBytesToReceive > 0)
	{
		/* send does not guarantee that the entire message is sent */
		BytesJustTransferred = recv(sd, CurPlacePtr, RemainingBytesToReceive, 0);
		if (BytesJustTransferred == SOCKET_ERROR)
		{
			return TRNS_FAILED;
		}
		else if (BytesJustTransferred == 0)
			return TRNS_DISCONNECTED; // recv() returns zero if connection was gracefully disconnected.

		RemainingBytesToReceive -= BytesJustTransferred;
		CurPlacePtr += BytesJustTransferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

TransferResult_t ReceiveString(char** output_string, SOCKET socket)
{
	int select_timing;

	if ((output_string == NULL) || (*output_string != NULL))
	{
		printf("The first input to ReceiveString() must be "
			"a pointer to a char pointer that is initialized to NULL. For example:\n"
			"\tchar* Buffer = NULL;\n"
			"\tReceiveString( &Buffer, ___ )\n");
		return TRNS_FAILED;
	}

	/* The request is received in two parts. First the Length of the string (stored in
	   an int variable ), then the string itself. */
	select_timing = SetReceiveTimeout(socket, RESPONSE_TIMEOUT_SEC);
	switch (select_timing)
	{
		case SELECT_ERROR_OCCURRED:
			return TRNS_FAILED;
		case SELECT_TIMEOUT:
			return TRNS_TIMEOUT;
		case SELECT_DATA_READY:
		default:
			break;
	}

	return ReadMessage(output_string, socket);
}

TransferResult_t ReceiveStringWithTimeout(char** output_string, SOCKET socket, long timeout_seconds)
{
	int select_timing;

	if ((output_string == NULL) || (*output_string != NULL))
	{
		printf("The first input to ReceiveString() must be "
			"a pointer to a char pointer that is initialized to NULL. For example:\n"
			"\tchar* Buffer = NULL;\n"
			"\tReceiveString( &Buffer, ___ )\n");
		return TRNS_FAILED;
	}

	/* The request is received in two parts. First the Length of the string (stored in
	   an int variable ), then the string itself. */
	if (timeout_seconds != TIMEOUT_INFINITE)
	{
		select_timing = SetReceiveTimeout(socket, timeout_seconds);
		switch (select_timing)
		{
		case SELECT_ERROR_OCCURRED:
			return TRNS_FAILED;
		case SELECT_TIMEOUT:
			return TRNS_TIMEOUT;
		case SELECT_DATA_READY:
		default:
			break;
		}
	}

	return ReadMessage(output_string, socket);
}

TransferResult_t ReadMessage(char** output_string, SOCKET socket)
{
	int message_size_in_bytes;
	TransferResult_t receive_result;
	char* string_buffer = NULL;

	// Get the length of the message
	receive_result = ReceiveBuffer(
		(char *)(&message_size_in_bytes),
		(int)(sizeof(message_size_in_bytes)), // 4 bytes
		socket);

	if (receive_result != TRNS_SUCCEEDED) return receive_result;

	// Get the message
	string_buffer = (char*)malloc(message_size_in_bytes * sizeof(char));

	if (string_buffer == NULL)
		return TRNS_FAILED;

	receive_result = ReceiveBuffer(
		(char *)(string_buffer),
		(int)(message_size_in_bytes),
		socket);

	if (receive_result == TRNS_SUCCEEDED)
	{
		*output_string = string_buffer;
	}
	else
	{
		free(string_buffer);
	}

	return receive_result;
}

int SetReceiveTimeout(SOCKET socket, long seconds)
{
	struct timeval timeout;
	struct fd_set fds;

	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(socket, &fds);

	return select(0, &fds, 0, 0, &timeout);
}