#include <stdio.h>
#include <math.h>
#include <libmill.h>
#include "hash_table.h"

#define TALK_MESSAGE_CENTER_BUFFER_SIZE 1000
#define TALK_MESSAGE_MANAGER_BUFFER_SIZE 5

// Override this define with a multiline, comma separated list of 
// custom message names
#define TALK_MESSAGES
enum msg_types {
	START_TALK = 0,
	REGISTER_RECEIVER,
	TALK_MESSAGES 
	END_TALK
};
static const int g_total_msg_types_size = END_TALK - START_TALK;

// user facing functions
void start_message_center();
void close_message_center();
void emit(msg_types type, void* payload) {
void emit(msg_types type, unsigned int id, void* payload) {
void receive(enum msg_types type, void (*callback)(void* payload));
void receive(unsigned int id, enum msg_types type,
			 void (*callback)(void* payload));
