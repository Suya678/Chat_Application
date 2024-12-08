#include "room_manager.h"

#include <ctype.h>   // For isdigit()
#include <stdbool.h> // For bool type
#include <stdio.h>
#include <stdlib.h> // For atoi()
#include <string.h> // For strcpy(), strlen()

#include "client_state_manager.h"
#include "logger.h"

/**
 * @brief Helper function to parse and validate the room number from the
 * client's message.
 *
 * Extracts and checks the room number format to ensure it's valid.
 *
 * @param client Pointer to the Client structure in the lobby state requesting
 * to join the room
 *
 * @return The parsed room number, or -1 if the format is invalid.
 * @NOTE THis function would need to be changed slight, if the max number of rooms exceeds 99 as it can only correctly
 * verify up to 2-digit numbers.
 */
static int parse_room_number(Client *client) {
    char *room_index = &client->current_msg[2];

    if (!isdigit(*room_index)) {
        return -1;
    }
    // Checks if the number is double-digit
    if (room_index[1] != '\0') {
        if (!isdigit(room_index[1]) || room_index[2] != '\0') {
            return -1;
        }
    }
    return atoi(room_index);
}
/**
 * @brief handles a client's request to create a room
 *
 * Initializes a room with the client in it if the following checks do not fail -
 * 1. The room name is less than or equal to MAX_ROOM_NAME_LEN
 * 2. There is currently space for a creation of a room in the array - SERVER_ROOMS
 *
 * @param client Pointer to the Client structure requesting the 'creation' of a
 *                room
 */
void create_chat_room(Client *client) {
    char success_msg[MAX_MESSAGE_LEN_FROM_SERVER];
    sprintf(success_msg, "Room created successfully: %s\n", &client->current_msg[2]);

    const char *room_name = &client->current_msg[2];
    LOG_INFO("Client %s (fd %d) attempting to create room: %s\n", client->name, client->client_fd, room_name);
    if (strlen(room_name) > MAX_ROOM_NAME_LEN) {
        LOG_USER_ERROR("Client %s (fd %d) provided invalid room name length: %zu\n", client->name, client->client_fd,
                       strlen(room_name));
        send_message_to_client(client->client_fd, ERR_ROOM_NAME_INVALID,
                               "Room creation failed: Room name length invalid\n");
        return;
    }

    for (int i = 0; i < MAX_ROOMS; i++) {
        pthread_mutex_lock(&SERVER_ROOMS[i].room_lock);

        if (!SERVER_ROOMS[i].in_use) {
            SERVER_ROOMS[i].in_use = true;
            SERVER_ROOMS[i].num_clients = 1;
            strcpy(SERVER_ROOMS[i].room_name, room_name);
            client->room_index = i;
            SERVER_ROOMS[i].clients[0] = client;
            client->state = IN_CHAT_ROOM;
            send_message_to_client(client->client_fd, CMD_ROOM_CREATE_OK, success_msg);
            LOG_INFO("Room %d: %s - create dby client %s (fd %d)\n", i, room_name, client->name, client->client_fd);
            pthread_mutex_unlock(&SERVER_ROOMS[i].room_lock);
            return;
        }
        pthread_mutex_unlock(&SERVER_ROOMS[i].room_lock);
    }
    send_message_to_client(client->client_fd, ERR_ROOM_CAPACITY_FULL,
                           "Room creation failed: Maximum number of rooms reached\n");
}

/**
 * @brief Sends a list of the currently available(running) rooms on the server
 * that the client can join
 *
 * @param client Pointer to the Client structure requesting the list of rooms
 */
void send_avail_rooms(const Client *client) {
    char room_list_msg[MAX_MESSAGE_LEN_FROM_SERVER] = "=== Available Chat Rooms ===\n\n";
    bool rooms_avail = false;
    LOG_INFO("Sending the list of rooms to client %s (fd %d)\n", client->name, client->client_fd);

    for (int i = 0; i < MAX_ROOMS; i++) {
        pthread_mutex_lock(&SERVER_ROOMS[i].room_lock);
        if (SERVER_ROOMS[i].in_use == true) {
            char room_entry[100];
            sprintf(room_entry, "Room %d: %s\n", i, SERVER_ROOMS[i].room_name);
            strcat(room_list_msg, room_entry);
            rooms_avail = true;
        }
        pthread_mutex_unlock(&SERVER_ROOMS[i].room_lock);
    }

    if (!rooms_avail) {
        LOG_INFO("Sending empty room list to client %s (fd %d)\n", client->name, client->client_fd);
        sprintf(room_list_msg + strlen(room_list_msg), "No chat rooms available!\nUse the create room command to start "
                                                       "your own chat room.\n");
    }
    LOG_INFO("Sending Room list: %s \nto client%s (fd %d)\n", room_list_msg, client->name, client->client_fd);
    send_message_to_client(client->client_fd, CMD_ROOM_LIST_RESPONSE, room_list_msg);
}

/**
 * @brief Removes a client from a specified room and updates the room's state.
 *
 * This function removes the client from the room, broadcasts a message
 * notifying other clients in the room, and cleans up the room's resources there
 * are no more clients in it.
 *
 * @param client Pointer to the Client structure being removed from the room.
 * @param room_index The index of the room in the SERVER_ROOMS array.
 *
 * @note The caller must ensure `room_index` is valid and corresponds to an
 * existing room.
 */
void leave_room(Client *client, int room_index) {
    LOG_INFO("Client %s (fd %d) left room %d (%s)\n", client->name, client->client_fd, room_index,
             SERVER_ROOMS[room_index].room_name);
    pthread_mutex_lock(&SERVER_ROOMS[room_index].room_lock);

    char client_left_msg[MAX_MESSAGE_LEN_FROM_SERVER];
    sprintf(client_left_msg, "%s left the room\n", client->name);

    for (int i = 0; i < MAX_CLIENTS_ROOM; i++) {
        if (SERVER_ROOMS[room_index].clients[i] == client) {
            SERVER_ROOMS[room_index].clients[i] = NULL;
            SERVER_ROOMS[room_index].num_clients--;
            break;
        }
    }
    LOG_INFO("Removed client %s (fd %d) from room %d, %d clients remaining\n", client->name, client->client_fd,
             room_index, SERVER_ROOMS[room_index].num_clients);
    broadcast_message_in_room(client_left_msg, room_index, client);

    if (SERVER_ROOMS[room_index].num_clients == 0) {
        LOG_INFO("Room %d (%s) is empty, cleaning up\n", room_index, SERVER_ROOMS[room_index].room_name);
        memset(SERVER_ROOMS[room_index].room_name, 0, sizeof(SERVER_ROOMS[room_index].room_name));
        memset(SERVER_ROOMS[room_index].clients, 0, sizeof(SERVER_ROOMS[room_index].clients));
        SERVER_ROOMS[room_index].in_use = false;
    }
    pthread_mutex_unlock(&SERVER_ROOMS[room_index].room_lock);
}

/**
 * @brief Broadcasts a message to all clients in the specified chat room.
 *
 * @param msg        The message to broadcast
 * @param room_index The index of the chat room in the SERVER_ROOMS array.
 * @param client      Client the message is being sent from. The message being
 * broadcast is not send to this client.
 *
 * @note The caller must acquire the room's lock
 * (SERVER_ROOMS[room_index].room_lock) before calling this function to ensure
 * thread safety.
 */
void broadcast_message_in_room(const char *msg, const int room_index, const Client *client) {
    LOG_INFO("Broadcasting message in room %d (%s): %s\n", room_index, SERVER_ROOMS[room_index].room_name, msg);

    for (int i = 0; i < MAX_CLIENTS_ROOM; i++) {
        if (SERVER_ROOMS[room_index].clients[i] != NULL && SERVER_ROOMS[room_index].clients[i] != client) {
            send_message_to_client(SERVER_ROOMS[room_index].clients[i]->client_fd, CMD_ROOM_MSG, msg);
        }
    }
    LOG_INFO("Message broadcasted to all clients in room %d\n", room_index);
}

/**
 * @brief Handles a client's request to join a chat room.
 *
 * Parses the requested room number, validates the room's existence and
 * availability, and adds the client to the room. Broadcasts a join message to
 * other clients in the room and notifies the client of success or failure.
 *
 * @param client Pointer to the Client structure representing the client
 * requesting to join.
 */
void join_chat_room(Client *client) {
    char client_room_join_msg[MAX_MESSAGE_LEN_FROM_SERVER];
    sprintf(client_room_join_msg, "%s has entered the room\n", client->name);
    int room_index = parse_room_number(client);

    if (room_index == -1) {
        LOG_USER_ERROR("Client %s (fd %d) provided invalid room number for joining\n", client->name, client->client_fd);
        send_message_to_client(client->client_fd, ERR_ROOM_NOT_FOUND,
                               "Invalid room number format. Must be a number between 0-99\n");
        return;
    }
    LOG_INFO("Client %s (fd %d) requested to join room %d\n", client->name, client->client_fd, room_index);

    pthread_mutex_lock(&SERVER_ROOMS[room_index].room_lock);
    if (SERVER_ROOMS[room_index].in_use == false) {
        LOG_USER_ERROR("Client %s (fd %d) attempted to join non-existent room %d\n", client->name, client->client_fd,
                       room_index);
        send_message_to_client(client->client_fd, ERR_ROOM_NOT_FOUND, "Room does not exist\n");
        pthread_mutex_unlock(&SERVER_ROOMS[room_index].room_lock);
        return;
    }

    if (SERVER_ROOMS[room_index].num_clients == MAX_CLIENTS_ROOM) {
        LOG_USER_ERROR("Client %s (fd %d) attempted to join a full room - %d: %s , "
                       "Number of clients "
                       "currently in the room = %d\n",
                       client->name, client->client_fd, room_index, SERVER_ROOMS[room_index].room_name,
                       SERVER_ROOMS[room_index].num_clients);
        send_message_to_client(client->client_fd, ERR_ROOM_CAPACITY_FULL, "Cannot join room: Room is full\n");
        pthread_mutex_unlock(&SERVER_ROOMS[room_index].room_lock);
        return;
    }

    for (int i = 0; i < MAX_CLIENTS_ROOM; i++) {
        if (SERVER_ROOMS[room_index].clients[i] == NULL) {
            SERVER_ROOMS[room_index].clients[i] = client;
            SERVER_ROOMS[room_index].num_clients++;
            LOG_INFO("Client %s (fd %d) joined room- %d: (%s)\n", client->name, client->client_fd, room_index,
                     SERVER_ROOMS[room_index].room_name);
            broadcast_message_in_room(client_room_join_msg, room_index, client);
            send_message_to_client(client->client_fd, CMD_ROOM_JOIN_OK, "Successfully joined room\n");
            client->state = IN_CHAT_ROOM;
            client->room_index = room_index;
            break;
        }
    }
    pthread_mutex_unlock(&SERVER_ROOMS[room_index].room_lock);
}
