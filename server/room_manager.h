#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include "server_config.h"
void create_chat_room(Client *client);

void join_chat_room(Client *client);
void send_avail_rooms(const Client *client);
void broadcast_message_in_room(const char *msg, int room_index, const Client *client);
void leave_room(Client *client, int room_index);
#endif