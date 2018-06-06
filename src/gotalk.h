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

typedef enum {
	START_TALK = 0,
	REGISTER_LISTENER,
	UNREGISTER_LISTENER,
#ifdef TALK_MESSAGES
	TALK_MESSAGES 
#endif
	END_TALK
} msg_types;

/*****************************************************************************/
/* Begin User Facing Functions */

// start the gotalk messaging system
void start_message_center();

// stop the gotalk messaging system and delete all listeners
void stop_message_center();

// send a message
void say(msg_types type, void* payload);

// send a message with a specific source id. This can be an object address.
void say(unsigned int source, msg_types type, void* payload);

// register a callback for a message
void listen(msg_types type, void (*callback)(void* payload));

// register a callback for a message with a specific destination and source 
// id's. These can be an object addresses. As this is raw C the objects won't be 
// directly called with the provided callback function. *However*, the callback
// function itself can use the provided object addresses and call any function
// it wishes from there. The callback is only called if the listener's source
// and the say message's source match.
void listen(unsigned int source, 
             msg_types type,
             unsigned int destination,
			 void (*callback)(void* payload));

// unregister all callbacks for a message type with specified destination.
// A 'NULL' destination only matches listeners with the specified message type
// that also have a 'NULL' destination, it does *not* match all listeners.
// Returns a confirmation channel for the calling function. It sends a
// confirmation integer '1' when the unregistration is complete.
chan unlisten(msg_types type, unsigned int destination);

/* End User Facing Functions */
/*****************************************************************************/
