#include <stdio.h>
#include <math.h>
#include <libmill.h>
#include "gotalk.h"

static const int g_total_msg_types_size = END_TALK - START_TALK;

typedef struct {
    void *data;
    struct List *next;
} List;

typedef struct {
    enum msg_types type;
	unsigned int source;
    void *payload;
} talk_msg;

typedef struct {
    enum msg_types type;
    unsigned int source;
    unsigned int destination;
	void (*callback)(talk_msg);
} talk_registration;

typedef struct {
    enum msg_types type;
    unsigned int source;
    unsigned int destination;
    chan conf_ch;
} talk_unregistration;

chan g_msg_channel = chmake(talk_msg, TALK_MESSAGE_CENTER_BUFFER_SIZE);

/*****************************************************************************/
/* Begin Forward Declarations */

/* List Forward Declarations */
List* create_list();
List* next(List *node);
void* get(List *node, int index);
bool insert(List *node, List *new_item, int index);
void* remove(List *node, int index);
/* End List Forward Declarations */

// only one instance of this routine will exist
coroutine void message_center(chan main_ch);
void setup_message_center(&msg_manager_channels);
void cleanup_message_center(chan msg_manager_channels[]);
void forward_registration(chan channels[], talk_msg msg);
void forward_unregistration(chan channels[], talk_msg msg);
void forward_msg(chan channels[], talk_msg msg);
void forward_listen_registration(chan channels[], talk_msg msg);

// an instance of this routine will exist for each message type
coroutine void message_manager(chan incoming_msgs);
void cleanup_message_manager(List* list);
void register_listenr(List* listenr_list, talk_msg msg);
void unregister_listenr(listenr_list, chan ch, talk_msg msg);

// an instance of this routine will exist for each registered listenr
coroutine void callback_handler(chan incoming_msgs, talk_registration* tmp_reg);
/* End Forward Declarations */
/*****************************************************************************/


/*****************************************************************************/
/* Begin List Functions */
List* create_list() {
    List *list = malloc(sizeof(List));
    (*list).data = NULL;
    (*list).next = NULL;
    return list;
}

List* next(List *node) {
    if((*node).next != NULL) {
        return (*node).next;
    else {
        return NULL;
    }
}

// Returns true on success. Returns false when not enough index items to insert
// where specified.
bool insert(List *node, List *new_item, unsigned int index) {
    for(unsigned int i=0; i<index; i++) {
        node = next(node);
        if(node == NULL) return false;
    }
    if(index > 0) {
        List *node2 = (*node).next;
        (*node).next = new_item;
        (*new_item).next = node2;
    } else {
        // we do this juggling to ensure the head node stays the same for 
        // the caller and so that new_item doesn't disappear from under the 
        // caller either.
        List *tmp = create_list;
        (*tmp).data = (*node).data;
        (*tmp).next = (*node).next;
        (*node).data = (*new_item).data;
        (*node).next = new_item;
        (*new_item).data = (*tmp).data;
        (*new_item).next = (*tmp).next;
        free(tmp);
    }
    return true;
}

void* get(List *node, int index) {
    for(int i=0; i<index; i++) {
        node = next(node);

        // return early if no next node
        if(node == NULL) return NULL;
    }
    return (*node).data;
}

// Returns removed list data on success, returns NULL if index too large
void* remove(List *node, int index) {
    List* prevNode;
    for(int i=0; i<index; i++) {
        prevNode = node;
        node = next(node);

        // return early if no next node
        if(node == NULL) return NULL;
    }
    (*prevNode).next = (*node).next;
    void* data = (*node).data;
    free(node)
    return data;
}
/* End List Functions */
/*****************************************************************************/


/*****************************************************************************/
/* Begin Message Center */
/*
 * Entry point routine to handle sayted messages. Forwards messages to handler 
 * routines (1 per message type) that manage the callback handler routines. 
 *
 * The message center is responsible for sending a message to the correct 
 * message handler
 */
coroutine void message_center(chan main_ch) {
    bool start = false;
    bool exit = false;
    chan msg_manager_channels[g_total_msg_types_size];
    while(1) {
        choose {
        in(main_ch, talk_msg, msg):
            switch(msg.type) {
                case START_TALK:
                    start = true;
                    setup_message_center(msg_manager_channels);
                    break;
                case END_TALK:
                    exit = true;
                    cleanup_message_center(msg_manager_channels);
                    break;
                case REGISTER_listenR:
                    if(start) forward_registration(msg_manager_channels, msg);
                    break;
                case UNREGISTER_listenR:
                    if(start) forward_unregistration(msg_manager_channels, msg);
                    break;
                default:
                    if(start) forward_msg(msg_manager_channels, msg);
                    break;
            }
        end
        }
        if(exit == true) break;
    }
    return;
}

void forward_registration(chan channels[], talk_msg msg) {
    enum msg_types type;
    talk_registration reg;

    reg = (talk_registration)(*payload);
    type = reg.type;

    chan ch = channels[type];
    if(ch) {
        chs(ch, talk_msg, msg);
    }
}

void forward_unregistration(chan channels[], talk_msg msg) {
    enum msg_types type;
    talk_unregistration reg;

    reg = (talk_unregistration)(*payload);
    type = reg.type;

    chan ch = channels[type];
    if(ch) {
        chs(ch, talk_msg, msg);
    }
}

void forward_msg(chan channels[], talk_msg msg) {
    enum msg_types type;

    type = msg.type;

    chan ch = channels[type];
    if(ch) {
        chs(ch, talk_msg, msg);
    }
    return;
}

void setup_message_center(chan msg_manager_channels[]) {
	for(int i = 0; i < g_total_msg_types; g++) {
		//create handler communication channel
		chan ch = chmake(talk_msg, TALK_MESSAGE_CENTER_BUFFER_SIZE);
		msg_manager_channels[i] = ch;

	    //launch coroutine
        talk_msg start_msg = {START_TALK, NULL, NULL};
	    go(message_manager(ch));
        chs(ch, talk_msg, start_msg);
	}
	return;
}

void cleanup_message_center(chan msg_manager_channels[]) {
	for(int i = 0; i < g_total_msg_types; g++) {
		//retrieve handler communication channel
		chan ch = msg_manager_channels[i];

        // send exit message to msg handler routines
        talk_msg end_msg = {END_TALK, NULL, NULL};
        chs(ch, talk_msg, end_msg);
	}
	chclose(g_msg_channel);
    return;
}
/* End Message Center */
/*****************************************************************************/


/*****************************************************************************/
/* Begin Message Manager */
/* 
 * Message managers contain the collection of all registered callback handlers
 * for a particular message type. Thus, when a message comes in it is
 * redistributed to *all* callback handlers managed by the message manager.
 */
coroutine void message_manager(chan incoming_msgs) {
    bool start = false;
    bool exit = false;
    List* listenr_list = create_list();
    while(1) {
        choose {
        in(incoming_msgs, talk_msg, msg):
            switch(msg.type) {
                case START_TALK:
                    start = true;
                    break;
                case END_TALK:
                    exit = true;
                    cleanup_message_manager(listenr_list);
                    break;
                case REGISTER_listenR:
                    if(start) register_listenr(listenr_list, msg);
                    break;
                case UNREGISTER_listenR:
                    if(start) {
                        int i = 0
                        List *tmp_list = listenr_list;
	                    while(tmp_list != NULL) {
                            chan ch = (*(*list).data);
                            if(ch) {
                                unregister_listenr(listenr_list, i, ch, msg);
                            }
                            tmp_list = next(tmp_list);
                            i++;
                        }
                    }
                    break;
                default:
                    if(start) {
                        List *tmp_list = listenr_list;
	                    while(tmp_list != NULL) {
                            chan ch = (*(*list).data);
                            if(ch) {
                                chs(ch, talk_msg, msg);
                            }
                            tmp_list = next(tmp_list);
                        }
                    }
                    break;
            }
        end
        }
        if(exit == true) break;
    }
    return;
}

void register_listenr(List* listenr_list, talk_msg msg) {
    enum msg_types type;
    talk_registration *reg = (talk_registration*)payload;

    chan ch = chmake(talk_msg, TALK_MESSAGE_MANAGER_BUFFER_SIZE);

    List* node = create_list();
    (*node).data = &ch;

    //insert the new listenr at the head of the list
    insert(listenr_list, node, 0);

    talk_msg start_msg = {START_TALK, NULL, NULL};
    chs(ch, talk_msg, start_msg);

    //launch callback handler
    go(callback_handler(ch, reg);
    return;
}

void unregister_listenr(listenr_list, index, chan ch, talk_msg msg) {
    talk_msg unreg_msg = {UNREGISTER_listenR, NULL, NULL};
    chs(ch, talk_msg, unreg_msg);
    while(1) {
        choose {
        in(ch, talk_msg msg):
            if(msg.type == UNREGISTER_listenR) {
                // wait to delete the channel until we listen unregistration 
                // confirmation
                
                unregister_listenr *unreg = msg.payload 
                //free 1
                free(unreg);
                remove(listenr_list, index);
                chclose(ch);
                break;
            } else if(msg.type == REGISTER_listenR) {
                break;
            }
        end 
        }
    }
    return;
}

void cleanup_message_manager(List* list) {
	while(list != NULL) {
		//retrieve handler communication channel
		chan ch = (*(*list).data);
		if(ch) {
		    // send exit message to msg handler routines
            talk_msg end_msg = {END_TALK, NULL, NULL};
            chs(ch, talk_msg, end_msg);
        }
        list = next(list); 
	}
    chclose(incoming_msgs);
    return;
}
/* End Message Manager */
/*****************************************************************************/


/*****************************************************************************/
/* Begin Callback Handler */
/* 
 * Callback handlers run in their own coroutine to ensure the desired function
 * runs in its own routine and doesn't slow down the messaging system.
 */
coroutine void callback_handler(chan incoming_msgs, talk_registration* tmp_reg) {
    bool start = false;
    bool exit = false;

    talk_registration listenr;
    listenr.type = (*tmp_reg).type;
    listenr.source = (*tmp_reg).source;
    listenr.destination = (*tmp_reg).destination;
    listenr.callback = (*tmp_reg).callback;

    //free 0
    free(tmp_reg);

    while(1) {
        choose {
        in(incoming_msgs, talk_msg, msg):
            switch(msg.type) {
                case START_TALK:
                    start = true;
                    break;
                case END_TALK:
                    // we can close the channel here because the calling func 
                    // isn't waiting for a response
                    chclose(incoming_msgs);
                    exit = true;
                    break;
                case UNREGISTER_listenR:
                    unregister_listenr *unreg = msg.payload;
                    if((*unreg).source == listenr.source) {
                        chs(talk_msg, {UNREGISTER_listenR, NULL, NULL});
                        exit = true;
                        break;
                    } else {
                        chs(talk_msg, {REGISTER_listenR, NULL, NULL});
                    }
                #ifdef OBJECT_CALLBACKS
                #endif
                default:
                    if(start) {
                        // Don't care about the message type, that's handled 
                        // in the message center
                        if(msg.source == listenr.source
                                || listenr.source == NULL) {
                            (*(listenr.callback))(msg);
                        }
                    }
                    break;
            }
        end
        }
        if(exit == true) break;
    }
    return;
}
/* End Callback Handler */
/*****************************************************************************/


/*****************************************************************************/
/* Start User Facing Functions */
void start_message_center() {
	talk_msg start_msg = {START_TALK, NULL, NULL};
	go(g_msg_channel);
	chs(g_msg_channel, talk_msg, start_msg);
	return;
}

void stop_message_center() {
	talk_msg start_msg = {END_TALK, NULL, NULL};
	go(g_msg_channel);
	chs(g_msg_channel, talk_msg, start_msg);
	return;
}

void say(msg_types type, void* payload) {
    say(NULL, type, payload);
    return;
}

void say(unsigned int source, msg_types type, void* payload) {
    talk_msg msg = {type, source, payload};
    chs(g_msg_channel, talk_msg, msg);
    return;
}

void listen(enum msg_types type, void (*callback)(void* payload)) {
    listen(NULL, type, NULL, callback);
    return;
}

void listen(unsigned int source, 
             enum msg_types type,
             unsigned int destination,
			 void (*callback)(void* payload));
    //malloc 0
	talk_registration reg* = malloc(sizeof(talk_registration));
    (*reg) = {type, source, destination, callback};
	talk_msg rec_msg = {REGISTER_listenR, source, reg};
	say(rec_msg);
	return;
}

/* Message type defnie marcro
 *
//macro:
#define message(TYPE, ...); #define 
 */

/* say macro 
 *
//macro:
//say(SOURCE, TYPE, ARG0, ... ARGN);

#ifndef TYPE
#define TYPE
typedef struct {
    enum msg_types type;
	unsigned int source;
	ARG0 ARG0v;
	...
	ARGN ARGNv;
} talk_msg_TYPE;

#define TMP_MESSAGES MESSAGES
#ifdef TALK_MESSAGES
#undef TALK_MESSAGES
#define TALK_MESSAGES  TMP_MESSAGES \
                       TYPE,
#undef TMP_MESSAGES
#endif
#endif
 */

/*
 * Object member listenr macro
//macro:
//listen(SOURCE, TYPE, CLASS object, (CLASS::FUNCTION), ARG0, ..., ARGN);
//or:
//listen(SOURCE, TYPE, DESTINATION, CALLBACK, ARG0, ... ARGN);
//handling for second option just evaluates to the above text while replacing
//SOURCE, TYPE, DESTINATION, and CALLBACK with real values
#ifndef OBJECT_CALLBACKS
#define OBJECT_CALLBACKS //cases for OBJECT_CALLBACKS in callback handler
#endif

#ifndef CLASS_FUNCTION
#define CLASS_FUNCTION_ARG0 ... ARGN
typedef struct {
    enum msg_types type;
    unsigned int source;
    CLASS& object; 
    ARG0 ARG0v;
    ...
    ARGN ARGNv;
} talk_registration_CLASS_FUNCTION;

#define TMP_MESSAGES MESSAGES
#ifdef TALK_MESSAGES
#undef TALK_MESSAGES
#define TALK_MESSAGES  TMP_MESSAGES \
                       REGISTER_listenR_CLASS_FUNCTION,
#undef TMP_MESSAGES
#endif
#endif

#ifndef TYPE
#define TYPE
typedef struct {
    enum msg_types type;
	unsigned int source;
	ARG0 ARG0v;
	...
	ARGN ARGNv;
} talk_msg_TYPE;

#define TMP_MESSAGES MESSAGES
#ifdef TALK_MESSAGES
#undef TALK_MESSAGES
#define TALK_MESSAGES  TMP_MESSAGES \
                       TYPE,
#undef TMP_MESSAGES
#endif
#endif

void listen-o(unsigned int source, 
          enum msg_types type, 
          CLASS& object, 
          (CLASS::*FUNCTION)(void* payload)) {
talk_registration_CLASS_FUNCTION reg* = malloc(sizeof(talk_registration_CLASS_FUNCTION));
(*reg) = {type, source, object, FUNCTION};
talk_msg rec_msg = {REGISTER_listenR_CLASS_FUNCTION, source, reg};
say(rec_msg);
}
#endif
listen-o(source, type, CLASS object, (CLASS::FUNCTION)(void*payload));
 */

chan unlisten(enum msg_types type, unsigned int destination) {
    //malloc 1
    talk_unregistration unreg* = malloc(sizeof(talk_unregistration));
    chan conf_ch = chmake(int, 1);
    (*unreg) = {type, source, destination, conf_ch};
    talk_msg unrec_msg = {UNREGISTER_listenR, NULL, unreg};
    say(unrec_msg);
    return conf_ch;
}
/* End User Facing Functions */
/*****************************************************************************/
