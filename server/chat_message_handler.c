
// Local
#include "chat_message_handler.h" // For our own declarations and constants

#include "protocol.h" // For command types, message length constants

// Library
#include <ctype.h> // For isdigit
#include <ctype.h>
#include <errno.h>      // For errno, EAGAIN, EWOULDBLOCK
#include <pthread.h>    // For pthread_mutex_lock/unlock
#include <stdbool.h>    // For bool type
#include <stdio.h>      // For sprintf, snprintf, perror
#include <stdlib.h>     // For atoi
#include <string.h>     // For strstr, strcpy, strlen, memset
#include <sys/epoll.h>  // For epoll_ctl
#include <sys/socket.h> // For recv, send
#include <unistd.h>     // For close

static void send_message_to_client(Client *client, char cmd_type, const char *message);
static void handle_client_disconnection(Client *client, Worker_Thread *thread_context);
static void handle_awaiting_username(Client *client);
static void handle_in_chat_lobby(Client *client);
static void handle_in_chat_room(Client *client);
static void route_client_command(Client *client, Worker_Thread *thread_context);
static void send_avail_rooms(Client *client);
static void create_chat_room(Client *client);
static void join_chat_room(Client *client);
static void cleanup_client(Client *client, Worker_Thread *thread_context);
static bool validate_msg_format(Client *client);
static void broadcast_message_in_room(const char *msg, int room_index, Client* client);
static void leave_room(Client *client, int room_index);
static void handle_in_chat_room(Client *client);
/**
 * @brief Processes incoming messages from a client socket.
 *
 * This function reads data from the client's socket, processes complete
 * messages terminated by "\r\n", and routes them for handling. Incomplete
 * messages are stored in the client's message buffer for later completion.
 * Handles disconnection if recv fails
 *
 * @param client            Pointer to the Client structure representing the
 *                          connected client. Contains the socket fd and the buffer for messages.
 * @param thread_context    Pointer to theWorker thread context containing data about the
 *                          thread hadnling the client
 *
 */
void process_client_message(Client *client, Worker_Thread *thread_context) {
    char read_buffer[MAX_MESSAGE_LEN_TO_SERVER + 1];
    ssize_t bytes_recieved = recv(client->client_fd, read_buffer, MAX_MESSAGE_LEN_TO_SERVER, 0);
    if (bytes_recieved <= 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        handle_client_disconnection(client, thread_context);
        return;
    }

    read_buffer[bytes_recieved] = '\0';

    char *temp = read_buffer;
    char *msg_term = strstr(read_buffer, "\r\n");

    // Process each complete message (ending in \r\n)
    while (msg_term != NULL) {
        *msg_term = '\0';
        strcat(client->current_msg, temp);
        route_client_command(client, thread_context); // Handle complete messages
        memset(client->current_msg, 0, sizeof(client->current_msg));
        temp = msg_term + 2;
        msg_term = strstr(temp, "\r\n"); // find the next message
    }

    // Saving the partial incomplete message
    if (*temp != '\0') {
        strcat(client->current_msg, temp);
    }
}
/**
 * @brief Sends a message to a client formatted to the specification in protcol.h
 *
 * Constructs a message with a command type, content, and terminator, then sends it
 * to the specified client's socket and logs the message.
 *
 * @param client    Pointer to the Client struct to send message to
 * @param cmd_type  Command character to prefix the message.
 * @param message   Message content to be sent to the client.
 *
 * @see protocol.h for the message protocol
 */
static void send_message_to_client(Client *client, char cmd_type, const char *message) {
    char message_buffer[MAX_MESSAGE_LEN_FROM_SERVER] = {};
    sprintf(message_buffer, "%c %s%s", cmd_type, message, MSG_TERMINATOR);

    send(client->client_fd, message_buffer, strlen(message_buffer), 0);

    log_message(message_buffer);
}

/**
 * @brief Validates client message format against protocol requirement
 *
 * @param client Pointer to the Client structure with the message to validate.
 * @return true if the message format is valid, false otherwise
 */
static bool validate_msg_format(Client *client) {
    // Check if message legnth is less than the minimum
    if (strlen(client->current_msg) < 3) {
        send_message_to_client(client, ERR_PROTOCOL_INVALID_FORMAT,
                               "Message too short\nCorrect format:[command "
                               "char][space][message content][MSG_TERMINATOR]\n");
        return false;
    }
    // Check if space is missing
    if (client->current_msg[1] != ' ') {
        send_message_to_client(client, ERR_PROTOCOL_INVALID_FORMAT,
                               "Missing space after command.\nCorrect format: [command "
                               "char][space][message content][MSG_TERMINATOR]\n");
        return false;
    }

    // Check if command is not valid
    if (client->current_msg[0] < CMD_EXIT || client->current_msg[0] > CMD_ROOM_MESSAGE_SEND) {
        send_message_to_client(client, ERR_PROTOCOL_INVALID_FORMAT,
                               "Command not found\nCorrect format: [command "
                               "char][space][message content][MSG_TERMINATOR]\n");
        return false;
    }

    // Check if content is empty
    char *content = &client->current_msg[2];
    while (*content == ' ' && *content != '\0') {
        content++;
    }
    if (*content == '\0') {
        send_message_to_client(client, ERR_MSG_EMPTY_CONTENT,
                               "Content is Empty\nCorrect format: [command "
                               "char][space][message content][MSG_TERMINATOR]\n");
        return false;
    }

    return true;
}

/**
 * @brief Routes the client's command based on their current state.
 *
 * Validates the client's message format and processes commands for depending
 * on the client's state
 *
 * @param client            Pointer to the Client structure which sent the commadn with the message
 * @param thread_context    Pointer to the Worker thread context containing data about the
 *                          thread hadnling the client
 */
static void route_client_command(Client *client, Worker_Thread *thread_context) {
    if (!validate_msg_format(client)) {
        return;
    }

    if (client->current_msg[0] == CMD_EXIT) {
        handle_client_disconnection(client, thread_context);
        return;
    }
    switch (client->state) {
        case AWAITING_USERNAME:
            handle_awaiting_username(client);
            break;
        case IN_CHAT_LOBBY:
            handle_in_chat_lobby(client);
            break;
        case IN_CHAT_ROOM:
            handle_in_chat_room(client);
            break;
    }
}

/**
 * @brief Handles the client's username submission when in the awaiting username state.
 *
 * Validates the command corresponds to the current state, assigns the username,
 * and transitions the client to the lobby state.
 *
 * @param client Pointer to the Client structure submitting the username. The user name is in the
 *              current msg of the client.
 */

static void handle_awaiting_username(Client *client) {
    if (client->current_msg[0] != CMD_USERNAME_SUBMIT) {
        send_message_to_client(client, ERR_PROTOCOL_INVALID_STATE_CMD,
                               "CMD not correct for client in awaiting username state\n");
        return;
    }

    strcpy(client->name, &client->current_msg[2]);
    client->state = IN_CHAT_LOBBY;
    send_avail_rooms(client);
}

/**
 * @brief Processes commands for clients in the chat lobby state.
 *
 * Validates the command corresponds to the current state, if correct, uses
 * helper funciton to do one fo the following - create a room, join a room or list the current
 * available rooms
 *
 * @param client Pointer to the Client structure in the lobby state.
 */

static void handle_in_chat_lobby(Client *client) {
    switch (client->current_msg[0]) {
        case CMD_ROOM_CREATE_REQUEST:
            create_chat_room(client);
            break;
        case CMD_ROOM_JOIN_REQUEST:
            join_chat_room(client);
            break;
        case CMD_ROOM_LIST_REQUEST:
            send_avail_rooms(client);
            break;
        default:
            send_message_to_client(client, ERR_PROTOCOL_INVALID_STATE_CMD,
                                   "Invalid command for lobby state\n");
            break;
    }
}

/**
 * @brief Helper function to parse and validate the room number from the client's message.
 *
 * Extracts and checks the room number format to ensure it's valid. IT shoudl be between 0 - 99.
 *
 * @param client Pointer to the Client structure in the lobby state requesting to join the room
 *
 * @return The parsed room number, or -1 if the format is invalid.
 */
static int parse_room_number(Client *client) {
    char *room_index = &client->current_msg[2];

    if (!isdigit(*room_index)) {
        return -1;
    }

    if (room_index[1] != '\0') {
        if (!isdigit(room_index[1]) || room_index[2] != '\0') {
            return -1;
        }
    }
    return atoi(room_index);
}

/**
 * @brief Cleans up the client's resources and removes them from the thread context.
 *
 * Removes the client's fd from epoll, closes the socket, clears the client structure,
 * and decrements the client count in the thread context.
 *
 * @param client Pointer to the Client structure to clean up.
 * @param thread_context Pointer to the Worker_Thread handling the client.
 */

static void cleanup_client(Client *client, Worker_Thread *thread_context) {
    if (epoll_ctl(thread_context->epoll_fd, EPOLL_CTL_DEL, client->client_fd, NULL) == -1) {
        perror("epoll_ctl removing client fd failed");
    }
    close(client->client_fd);
    memset(client, 0, sizeof(Client));
    thread_context->num_of_clients--;
}

/**
 * @brief Handles the disconnection of a client.
 *
 * Cleans up the client,and if client is in a room, calls leave_room()notifies other clients in the
 * same room, and frees up room resources if the client was the the last client in the room.
 *
 * @param client Pointer to the Client structure that has/(requesting to be) disconnected.
 * @param thread_context Pointer to the Worker_Thread handling the client.
 */
static void handle_client_disconnection(Client *client, Worker_Thread *thread_context) {
    ClIENT_STATE state = client->state;
    if (state == IN_CHAT_ROOM) {
        int room_index = client->room_index;
        leave_room(client, room_index);
    }

    cleanup_client(client, thread_context);
    
}

/**
 * @brief Removes a client from a specified room and updates the room's state.
 *
 * This function removes the client from the room, broadcasts a message notifying
 * other clients in the room, and cleans up the room's resources there are no more clients in it.
 *
 * @param client Pointer to the Client structure being removed from the room.
 * @param room_index The index of the room in the SERVER_ROOMS array.
 *
 * @note The caller must ensure `room_index` is valid and corresponds to an existing room.
 */
static void leave_room(Client *client, int room_index) {
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
    broadcast_message_in_room(client_left_msg, room_index, client);
    if (SERVER_ROOMS[room_index].num_clients == 0) {
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
 * @param client      Client the message is being sent from. The message being broadcasted is not send to this client.
 *
 * @note The caller must acquire the room's lock (SERVER_ROOMS[room_index].room_lock)
 *       before calling this function to ensure thread safety.
 */
static void broadcast_message_in_room(const char *msg, int room_index, Client* client) {
    for (int i = 0; i < MAX_CLIENTS_ROOM; i++) {
        if (SERVER_ROOMS[room_index].clients[i] != NULL && SERVER_ROOMS[room_index].clients[i] != client) {
            send_message_to_client(SERVER_ROOMS[room_index].clients[i], CMD_ROOM_MSG, msg);
        }
    }
}

/**
 * @brief Handles a client's request to join a chat room.
 *
 * Parses the requested room number, validates the room's existence and availability,
 * and adds the client to the room. Broadcasts a join message to other clients in the
 * room and notifies the client of success or failure.
 *
 * @param client Pointer to the Client structure representing the client requesting to join.
 */
static void join_chat_room(Client *client) {
    char client_room_join_msg[MAX_MESSAGE_LEN_FROM_SERVER];
    sprintf(client_room_join_msg, "%s has entered the room\n", client->name);
    int room_index = parse_room_number(client);

    if (room_index == -1) {
        send_message_to_client(client, ERR_ROOM_NOT_FOUND,
                               "Invalid room number format. Must be a number between 0-99\n");
        return;
    }

    pthread_mutex_lock(&SERVER_ROOMS[room_index].room_lock);
    if (SERVER_ROOMS[room_index].in_use == false) {
        send_message_to_client(client, ERR_ROOM_NOT_FOUND, "Room does not exist\n");
        pthread_mutex_unlock(&SERVER_ROOMS[room_index].room_lock);
        return;
    }

    if (SERVER_ROOMS[room_index].num_clients == MAX_CLIENTS_ROOM) {
        send_message_to_client(client, ERR_ROOM_CAPACITY_FULL, "Cannot join room: Room is full\n");
        pthread_mutex_unlock(&SERVER_ROOMS[room_index].room_lock);
        return;
    }

    for (int i = 0; i < MAX_CLIENTS_ROOM; i++) {
        if (SERVER_ROOMS[room_index].clients[i] == NULL) {
            SERVER_ROOMS[room_index].clients[i] = client;
            SERVER_ROOMS[room_index].num_clients++;

            broadcast_message_in_room(client_room_join_msg, room_index, client);
            send_message_to_client(client, CMD_ROOM_JOIN_OK, "Successfully joined room\n");
            client->state = IN_CHAT_ROOM;
            client->room_index = room_index;
            break;
        }
    }
    pthread_mutex_unlock(&SERVER_ROOMS[room_index].room_lock);
}

/**
 * @brief handles a client's request to create a room
 *
 * Initalizes a room with the client in it if the following checks do not fail -
 * 1. The room name is less than or equal to to MAX_ROOM_NAME_LEN
 * 2. There is currently space for a creation of a room in the arry- SERVER_ROOMS(no room is in use)
 *
 * @param client Pointer to the Client structure requesting the 'creation' of a room
 */
static void create_chat_room(Client *client) {
    char success_msg[MAX_MESSAGE_LEN_FROM_SERVER];
    sprintf(success_msg, "Room created successfully: %s\n", &client->current_msg[2]);

    const char *room_name = &client->current_msg[2];

    if (strlen(room_name) > MAX_ROOM_NAME_LEN) {
        send_message_to_client(client, ERR_ROOM_NAME_INVALID,
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
            send_message_to_client(client, CMD_ROOM_CREATE_OK, success_msg);
            pthread_mutex_unlock(&SERVER_ROOMS[i].room_lock);
            return;
        }
        pthread_mutex_unlock(&SERVER_ROOMS[i].room_lock);
    }
    send_message_to_client(client, ERR_SERVER_ROOM_FULL,
                           "Room creation failed: Maximum number of rooms reached\n");
}

/**
 * @brief Sends a list of the currently available(running) rooms on the server that the client can
 * joiing
 *
 * @param client Pointer to the Client structure requesting the list of rooms
 */
static void send_avail_rooms(Client *client) {
    char room_list_msg[MAX_MESSAGE_LEN_FROM_SERVER] = "=== Available Chat Rooms ===\n\n";
    bool rooms_avail = false;

    for (int i = 0; i < MAX_ROOMS; i++) {
        pthread_mutex_lock(&SERVER_ROOMS[i].room_lock);
        if (SERVER_ROOMS[i].in_use == true) {
            log_message("here \n");
            sprintf(room_list_msg + strlen(room_list_msg), "Room %d: %s\n", i,
                    SERVER_ROOMS[i].room_name);
            rooms_avail = true;
        }
        pthread_mutex_unlock(&SERVER_ROOMS[i].room_lock);
    }

    if (!rooms_avail) {
        sprintf(room_list_msg + strlen(room_list_msg),
                "No chat rooms available!\nUse the create room command to start "
                "your own chat room.\n");
    }

    send_message_to_client(client, CMD_ROOM_LIST_RESPONSE, room_list_msg);
}

/**
 * @brief Processes commands for clients in the chat room state
 *
 * Validates the command corresponds to the current state, if correct, uses
 * helper funciton to do one fo the following - create a room, join a room or list the current
 * available rooms
 *
 * @param client Pointer to the Client structure in the lobby state.
 */

static void handle_in_chat_room(Client *client) {
    char msg[MAX_MESSAGE_LEN_FROM_SERVER];
    char command = client->current_msg[0];

    if (command != CMD_ROOM_MESSAGE_SEND && command != CMD_LEAVE_ROOM) {
        send_message_to_client(client, ERR_PROTOCOL_INVALID_STATE_CMD,
                               "Invalid command for in chat room state\n");
    }

    int room_index = client->room_index;

    if (command == CMD_ROOM_MESSAGE_SEND) {
        sprintf(msg, "%s: %s", client->name, &client->current_msg[2]);
        pthread_mutex_lock(&SERVER_ROOMS[room_index].room_lock);
        broadcast_message_in_room(msg, room_index, client);
        pthread_mutex_unlock(&SERVER_ROOMS[room_index].room_lock);

    } else {
        sprintf(msg, "%s has left the room", client->name);
        leave_room(client, room_index);
        pthread_mutex_lock(&SERVER_ROOMS[room_index].room_lock);
        send_message_to_client(client,CMD_ROOM_LEAVE_OK,"You have left the room\n");
        pthread_mutex_unlock(&SERVER_ROOMS[room_index].room_lock);
        client->state = IN_CHAT_LOBBY;
    }
}
