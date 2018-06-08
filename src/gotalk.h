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
#include "message_enums.h"
	END_TALK
} msg_types;

/* replace message() text with blank whitespace. Enum creation is handled by 
 * external script 
 */
#define message(x)

/*****************************************************************************/
/* Begin User Facing Functions */

// start the gotalk messaging system
chan start_message_center();

// stop the gotalk messaging system and delete all listeners
void stop_message_center(chan ch);

// send a message
void say(chan msg_center, msg_types type, void* payload);
void say(chan msg_center, msg_types type, unsigned int source, ...);

// send a message with a specific source id. This can be an object address.
void say(chan msg_center, unsigned int source, msg_types type, void* payload);

// register a callback for a message
void listen(chan msg_center, msg_types type, void (*callback)(void* payload));
void listen(chan msg_center, 
            unsigned int source, 
            msg_types type, 
            unsigned int destination, 
            ...);

// register a callback for a message with a specific destination and source 
// id's. These can be an object addresses. As this is raw C the objects won't be 
// directly called with the provided callback function. *However*, the callback
// function itself can use the provided object addresses and call any function
// it wishes from there. The callback is only called if the listener's source
// and the say message's source match.
void listen(chan msg_center,
             unsigned int source, 
             msg_types type,
             unsigned int destination,
			 void (*callback)(void* payload));

// unregister all callbacks for a message type with specified destination.
// A 'NULL' destination only matches listeners with the specified message type
// that also have a 'NULL' destination, it does *not* match all listeners.
// Returns a confirmation channel for the calling function. It sends a
// confirmation integer '1' when the unregistration is complete.
chan unlisten(chan msg_center, msg_types type, unsigned int destination);

/* End User Facing Functions */
/*****************************************************************************/
