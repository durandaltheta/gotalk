#include <stdio.h>
#include <math.h>
#include <libmill.h>
#include "hash_table.h"
#include "gotalk.h"

typedef struct {
    enum msg_types type;
	unsigned int id;
    void *payload;
} talk_msg;

typedef struct {
    enum msg_types type;
    unsigned int id;
	void (*callback)(talk_msg);
} talk_registration;

chan g_msg_channel = chmake(talk_msg, TALK_MESSAGE_CENTER_BUFFER_SIZE);


/*****************************************************************************/
/* Begin Forward Declarations */
char* get_key(int integer_key);
chan retrieve_msg_channel(ht_hash_table* table, char* key);

// only one instance of this routine will exist
coroutine void message_center(chan main_ch);
void setup_message_center(&msg_manager_channels);
void cleanup_message_center(chan msg_manager_channels[]);
void forward_msg(ht_hash_table* table, talk_msg msg);
void forward_receive_registration(chan channels[], talk_msg msg);

// an instance of this routine will exist for each message type
coroutine void message_manager(chan incoming_msgs);
void cleanup_message_manager(ht_hash_table* table, const int total_handlers);
void register_receiver(ht_hash_table* receiver_table, talk_msg msg);
void forward_msg_to_handler(ht_hash_table* table, talk_msg msg, int integer_key);

// an instance of this routine will exist for each registered receiver
coroutine void callback_handler(chan incoming_msgs, talk_registration receiver);
/* End Forward Declarations */
/*****************************************************************************/


/*****************************************************************************/
/* Begin Convenience Functions */
// get the key string for the hash table
char* get_key(int integer_key) {
    // get the number of digits in the provided integer (+1 for \0)
    int size = floor (log10 (abs (integer_key))) + 2;
    char* key[size];
    sprintf(key, "%d\0",integer_key);
    return key;
}

// retrieve a talk_msg channel from provide ht hash table
chan retrieve_msg_channel(ht_hash_table* table, char* key) {
    void* item = ht_search(table, key);
    if(item) {
        return chan ch = (chan)(*item);
    } else {
        return NULL;
    }
}
/* End Convenience Functions */
/*****************************************************************************/


/*****************************************************************************/
/* Begin Message Center */
// main routine to handle emitted messages. Forwards messages to handler 
// routines (1 per message type) that manage the callback handler routines
coroutine void message_center(chan main_ch) {
    bool start = false;
    bool exit = false;
    chan msg_manager_channels[g_total_msg_types_size];
    while(1) {
        choose {
        in(main_ch, talk_msg, msg):
            if(msg.type == START_TALK) {
                start = true;
                setup_message_center(msg_manager_channels);
            } else if(msg.type == END_TALK) {
                exit = true;
                cleanup_message_center(msg_manager_channels);
            } else if(msg.type == REGISTER_RECEIVER) {
                if(start) forward_registration(msg_manager_channels, msg);
            } else {
                if(start) forward_msg(msg_manager_channels, msg);
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

void forward_msg(chan channels[], talk_msg msg) {
    enum msg_types type;

    type = msg.type;

    chan ch = channels[type];
    if(ch) {
        chs(ch, talk_msg, msg);
    }
    return;
}

void forward_msg_to_handler(ht_hash_table* table, talk_msg msg, int integer_key) {
    char* key = get_key(integer_key);

    chan ch = retrieve_msg_channel(table, key);
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
        struct talk_msg start_msg = {START_TALK, NULL, NULL};
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
        struct talk_msg end_msg = {END_TALK, NULL, NULL};
        chs(ch, talk_msg, end_msg);
	}
	chclose(g_msg_channel);
    return;
}

/* End Message Center */
/*****************************************************************************/


/*****************************************************************************/
/* Begin Message Manager */
coroutine void message_manager(chan incoming_msgs) {
    bool start = false;
    bool exit = false;
    ht_hash_table* receiver_table = ht_new();
    unsigned int num_callbacks = 0;
    while(1) {
        choose {
        in(incoming_msgs, talk_msg, msg):
            if(msg.type == START_TALK) {
                start = true;
            } else if(msg.type == END_TALK) {
                exit = true;
                cleanup_message_manager(receiver_table);
            } else if(msg.type == REGISTER_RECEIVER) {
                register_receiver(receiver_table, msg, num_callbacks);
                num_callbacks++;
            } else {
                if(start) {
                    for(int i = 0, i < num_callbacks; i++) {
                        forward_msg_to_handler(receiver_table, msg, i);
                    }
                }
            }
        end
        }
        if(exit == true) break;
    }
    return;
}

void register_receiver(ht_hash_table* receiver_table, talk_msg msg, int integer_key) {
    enum msg_types type;
    talk_registration reg;

    reg = (talk_registration)(*payload);

    char* key = get_key(integer_key);

    chan ch = chmake(talk_msg, TALK_MESSAGE_MANAGER_BUFFER_SIZE);

    ht_insert(receiver_table, key, &ch);
    go(callback_handler(ch, reg);
    struct talk_msg start_msg = {START_TALK, NULL, NULL};
    chs(ch, talk_msg, start_msg);
}

void cleanup_message_manager(ht_hash_table* table, const int total_handlers) {
	for(int i = 0; i < total_handlers; g++) {
		//retrieve handler communication channel
		chan ch = retrieve_msg_channel(table, get_key(i));
		if(ch) {
		    // send exit message to msg handler routines
            struct talk_msg end_msg = {END_TALK, NULL, NULL};
            chs(ch, talk_msg, end_msg);
        }
	}
    chclose(incoming_msgs);
    return;
}
/* End Message Manager */
/*****************************************************************************/


/*****************************************************************************/
/* Begin Callback Handler */
coroutine void callback_handler(chan incoming_msgs, talk_registration receiver) {
    bool start = false;
    bool exit = false;
    while(1) {
        choose {
        in(incoming_msgs, talk_msg, msg):
            if(msg.type == START_TALK) {
                start = true;
            } else if(msg.type == END_TALK) {
                exit = true;
            } else {
                if(start) {
                    if(msg.id == receiver.id 
                            || receiver.id == NULL) {
                        (*(receiver.callback))(msg);
                    }
                }
            }
        end
        }
        if(exit == true) break;
    }
    chclose(incoming_msgs);
    return;
}
/* End Callback Handler */
/*****************************************************************************/


/*****************************************************************************/
/* Start User Facing Functions */
void start_message_center() {
	struct talk_msg start_msg = {START_TALK, NULL, NULL};
	go(g_msg_channel);
	chs(g_msg_channel, talk_msg, start_msg);
	return;
}

void stop_message_center() {
	struct talk_msg start_msg = {END_TALK, NULL, NULL};
	go(g_msg_channel);
	chs(g_msg_channel, talk_msg, start_msg);
	return;
}

void emit(msg_types type, void* payload) {
    emit(type, NULL, payload);
    return;
}

void emit(msg_types type, unsigned int id, void* payload) {
    talk_msg msg = {type, id, payload};
    chs(g_msg_channel, talk_msg, msg);
    return;
}

void receive(enum msg_types type, void (*callback)(void* payload)) {
    receive(type, NULL, callback);
    return;
}

void receive(unsigned int id, enum msg_types type,
			 void (*callback)(void* payload)) {
	talk_registration receive_reg = {id, type, callback};
	talk_msg rec_msg = {REGISTER_RECEIVER, NULL, &receive_reg};
	emit(rec_msg);
	return;
}
/* End User Facing Functions */
/*****************************************************************************/
