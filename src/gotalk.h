#define TALK_MESSAGE_CENTER_BUFFER_SIZE 1000
#define TALK_MESSAGE_MANAGER_BUFFER_SIZE 5

/*
 Override this define with a multiline (end each line with a \), 
 comma separated list of custom message names

Example:
#ifdef TALK_MESSAGES 
#undef TALK_MESSAGES
#define TALK_MESSAGES message1, \
                      message2, \
                      message3,
#endif

 The library user is responsible for keeping track of the intended payload 
 for each message type.
*/
#define TALK_MESSAGES

enum msg_types {
	START_TALK = 0,
	REGISTER_RECEIVER,
	UNREGISTER_RECEIVER,
	TALK_MESSAGES 
	END_TALK
};
static const int g_total_msg_types_size = END_TALK - START_TALK;

/*****************************************************************************/
/* Begin User Facing Functions */

// start the gotalk messaging system
void start_message_center();

// stop the gotalk messaging system and delete all receivers
void stop_message_center();

// send a message
void emit(msg_types type, void* payload);

// send a message with a specific id
void emit(unsigned int source, msg_types type, void* payload);

// register a callback for a message
void receive(enum msg_types type, void (*callback)(void* payload));

// register a callback for a message with a specific id
void receive(unsigned int source, 
             enum msg_types type,
             unsigned int destination,
			 void (*callback)(void* payload));

/* End User Facing Functions */
/*****************************************************************************/
