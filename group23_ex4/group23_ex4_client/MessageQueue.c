#include <stdlib.h>
#include <Windows.h>
#include "MessageQueue.h"

//The function allocate the msg_queue and creates all mutexs and shemaphors
message_queue_t* CreateMessageQueue()
{
	DWORD last_error;
	message_queue_t* message_queue = NULL;

	message_queue = (message_queue_t*)malloc(sizeof(message_queue_t)); // Malloc the msg_queue
	if (message_queue == NULL) {
		printf("Error allocating msg_queue at CreateMessageQueue function.\n");
		return NULL;
	}

	message_queue->head = NULL;

	message_queue->access_mutex = CreateMutex(
		NULL,	/* default security attributes */
		FALSE,	/* initially not owned */
		NULL);	/* unnamed mutex */
	if (NULL == message_queue->access_mutex)
	{
		printf("Error when creating lmsg_queue mutexat msg_queue_creator function\n", GetLastError());
		free(message_queue);
		return NULL;
	}

	message_queue->msgs_count_semaphore = CreateSemaphore(
		NULL,	/* Default security attributes */
		0,		/* Initial Count - all slots are empty */
		30,		/* Maximum Count */
		NULL); /* un-named */
	if (message_queue->msgs_count_semaphore == NULL) {
		printf("Error when creating msgs_count semaphore: %d at msg_queue_creator function\n", GetLastError());
		free(message_queue->access_mutex);
		free(message_queue);
		return NULL;
	}

	message_queue->queue_empty_event = CreateEvent(
		NULL, /* default security attributes */
		TRUE,       /* manual-reset event */
		TRUE,      /* initial state is non-signaled */
		NULL);         /* name */
	/* Check if succeeded and handle errors */

	last_error = GetLastError();
	/* If last_error is ERROR_SUCCESS, then it means that the event was created.
	   If last_error is ERROR_ALREADY_EXISTS, then it means that the event already exists */

	message_queue->stop_event = CreateEvent(
		NULL, /* default security attributes */
		TRUE,       /* manual-reset event */
		FALSE,      /* initial state is non-signaled */
		NULL);         /* name */
	/* Check if succeeded and handle errors */

	last_error = GetLastError();
	/* If last_error is ERROR_SUCCESS, then it means that the event was created.
	   If last_error is ERROR_ALREADY_EXISTS, then it means that the event already exists */
	return message_queue;
}

//The function gets pointer to message queue and a message string and put it in the buffer
int EnqueueMsg(message_queue_t* msg_queue, char *msg)
{
	message_queue_node_t *node, *cur;

	if (!msg_queue) 
	{
		printf("Error at EnqueueMsg function\n");
		return(QUEUE_ERROR);
	}

	node = (message_queue_node_t*)malloc(sizeof(message_queue_node_t));
	if (!node) {
		printf("Malloc error at EnqueueMsg function\n");
		exit(QUEUE_ERROR);
	}
	node->next = NULL;
	node->data = msg;

	if (WAIT_OBJECT_0 != WaitForSingleObject(msg_queue->access_mutex, INFINITE)) {
		printf("WaitForSingleObject error at EnqueueMsg function\n");
		return(QUEUE_ERROR);
	}
	if (msg_queue->head == NULL)
	{
		msg_queue->head = node;
	}
	else {
		cur = msg_queue->head;
		while (cur->next != NULL)
			cur = cur->next;
		cur->next = node;
	}

	if (!ReleaseSemaphore(msg_queue->msgs_count_semaphore, 1, NULL)) {
		printf("ReleaseSemaphore failed (%ld) at EnqueueMsg function\n", GetLastError());
		return(QUEUE_ERROR);
	}
	if (!ReleaseMutex(msg_queue->access_mutex)) {
		printf("ReleaseMutex failed (%ld) at EnqueueMsg function\n", GetLastError());
		return(QUEUE_ERROR);
	}

	if (!ResetEvent(msg_queue->queue_empty_event)) {
		printf("ResetEvent failed (%ld) at EnqueueMsg function\n", GetLastError());
		return(QUEUE_ERROR);
	}

	return QUEUE_SUCCESS;
}

//The function gets pointer to message and returns a message string from the buffer
char* DequeueMsg(message_queue_t* msg_queue)
{
	char *new_msg = NULL;
	message_queue_node_t* temp_head;
	DWORD ret;
	HANDLE wait_for[2];

	wait_for[0] = msg_queue->stop_event;
	wait_for[1] = msg_queue->msgs_count_semaphore;

	/* checking if queue is empty */
	ret = WaitForSingleObject(msg_queue->msgs_count_semaphore, INFINITE);
	if (ret == WAIT_TIMEOUT)
	{
		SetEvent(msg_queue->queue_empty_event);
		ret = WaitForMultipleObjects(2, wait_for, FALSE, INFINITE);
		if (ret == WAIT_OBJECT_0) {
			return(QUEUE_ERROR);
		}
		if (ret != WAIT_OBJECT_0 + 1) {
			return(QUEUE_ERROR);
		}
	}
	else if (ret != WAIT_OBJECT_0) {
		return;
	}

	if (WAIT_OBJECT_0 != WaitForSingleObject(msg_queue->access_mutex, INFINITE))
		return GetLastError();

	temp_head = msg_queue->head;
	msg_queue->head = msg_queue->head->next;
	if (!ReleaseMutex(msg_queue->access_mutex))
		return GetLastError();

	new_msg = temp_head->data;
	free(temp_head);

	return new_msg;
}