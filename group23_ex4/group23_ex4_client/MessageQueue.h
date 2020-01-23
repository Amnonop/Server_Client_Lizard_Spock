#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <Windows.h>

#define QUEUE_ERROR -10

// A struct representing a message in the message queue
typedef struct message_queue_node
{
	char* data;
	struct message_queue_node* next;
} message_queue_node_t;

// Struct which contains the message queue sent from user application 
// thread to send thread
// The message queue itself is a linked list
typedef struct message_queue {
	struct message_queue_node* head;
	HANDLE access_mutex;
	HANDLE msgs_count_semaphore;
	HANDLE queue_empty_event;
	HANDLE stop_event;
} message_queue_t;

message_queue_t* CreateMessageQueue();
int EnqueueMsg(message_queue_t* msg_queue, char *msg);
char* DequeueMsg(message_queue_t* msg_queue);

#endif
