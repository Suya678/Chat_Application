#include "chat_message_handler.h"


void handle_awaiting_username(Client* client, Worker_Thread* thread_context);
void handle_in_chat_lobby(Client* client, Worker_Thread* thread_context);
void handle_in_chat_room(Client* client, Worker_Thread* thread_context);
void broadcast_message_to_room(Client* sender,Worker_Thread* thread_context);
void route_client_command(Client *client, Worker_Thread* thread_context);
void send_avail_rooms(Client *client, Worker_Thread* thread_context);
void create_chat_room(Client *client, Worker_Thread *thread_context);


void process_client_message(Client* client, Worker_Thread* thread_context){
    char read_buffer[MAX_MESSAGE_LEN_TO_SERVER + 1];
    ssize_t bytes_recieved;
    while(1) {
        bytes_recieved = recv(client->client_fd,read_buffer,MAX_MESSAGE_LEN_TO_SERVER, 0);
        if(bytes_recieved == 0) {
            handle_client_disconnection(client, thread_context);
            return;
        }
        if(bytes_recieved < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;  
            }
            handle_client_disconnection(client, thread_context);
            return;
        }
        read_buffer[bytes_recieved] = '\0';

        strcat(client->current_msg,read_buffer);
        char *msg_term = strstr(client->current_msg,"\r\n");
        if(msg_term == NULL ) {
            continue;
        }
        *msg_term = '\0';
        route_client_command(client,thread_context);
            
        memset(client->current_msg, 0, MAX_MESSAGE_LEN_TO_SERVER);

        return;
    }


}


void send_message_to_client(Client* client, char cmd_type, const char* message, Worker_Thread* thread_context) {
    char message_buffer[MAX_MESSAGE_LEN_FROM_SERVER] = {};
    sprintf(message_buffer, "%c %s%s", cmd_type, message, MSG_TERMINATOR);
    ssize_t bytes_sent = send(client->client_fd, message_buffer, strlen(message_buffer), 0);
    if (bytes_sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        handle_client_disconnection(client, thread_context);
    }
}


bool validate_msg_format(Client *client, Worker_Thread* thread_context){
    //Check if message legnth is less than the minimum
   if(strlen(client->current_msg) < 3) {
        send_message_to_client(client, ERR_PROTOCOL_INVALID_FORMAT,
        "Message too short\nCorrect format:[command char][space][message content][MSG_TERMINATOR]\n", thread_context);
        return false;
   }
    //Check if space is missing
   if(client->current_msg[1] != ' ') {
        send_message_to_client(client, ERR_PROTOCOL_INVALID_FORMAT,
        "Missing space after command.\nCorrect format: [command char][space][message content][MSG_TERMINATOR]\n", thread_context);
        return false;
   }
    //Check if command is not valid
    if(client->current_msg[0] < CMD_EXIT || client->current_msg[0] > CMD_ROOM_MESSAGE_SEND) {
        send_message_to_client(client, ERR_PROTOCOL_INVALID_FORMAT,
        "Command not found\nCorrect format: [command char][space][message content][MSG_TERMINATOR]\n", thread_context);
        return false;
    }

    //Check if content is empty
    char *content = &client->current_msg[2];
    while(*content == ' ' && *content != '\0') {
        content++;
    }

    if(*content == '\0') {
        send_message_to_client(client, ERR_MSG_EMPTY_CONTENT,
        "Content is Empty\nCorrect format: [command char][space][message content][MSG_TERMINATOR]\n", thread_context);
        return false;
    }

   return true;
}

void route_client_command(Client *client, Worker_Thread* thread_context) {
    if(!validate_msg_format(client,thread_context)){
        return;
    }

    if(client->current_msg[0] == CMD_EXIT){
        handle_client_disconnection(client, thread_context);
        return;
    }

    switch(client->state) {
        case AWAITING_USERNAME:
            handle_awaiting_username(client, thread_context);
            break;
        case IN_CHAT_LOBBY:
            handle_in_chat_lobby(client, thread_context);
            break;
        case IN_CHAT_ROOM:
            handle_in_chat_room(client, thread_context);
            break;

    }
}


void handle_awaiting_username(Client* client, Worker_Thread* thread_context) {

    if(client->current_msg[0] != CMD_USERNAME_SUBMIT){
        send_message_to_client(client,ERR_PROTOCOL_INVALID_STATE_CMD, "CMD not correct for client in awaiting username state\n",
        thread_context);
        return;
    }

    strcpy(client->name,&client->current_msg[2]);

    client->state = IN_CHAT_LOBBY;
    send_avail_rooms(client, thread_context);

}


  /*
        case IN_CHAT_LOBBY:
            if(command_type != CMD_ROOM_CREATE_REQUEST && 
               command_type != CMD_ROOM_JOIN_REQUEST &&
               command_type != CMD_ROOM_LIST_REQUEST) {
                sprintf(error_msg, "%c Invalid lobby command. Available commands: Create Room, Join Room, List Rooms \n%s",
                    , MSG_TERMINATOR);
                send(client->client_fd, error_msg, strlen(error_msg), 0);
            }
            return false;

        case IN_CHAT_ROOM:
            if(command_type != CMD_ROOM) {
                sprintf(error_msg, "%c Invalid room command\n%s",
                    , MSG_TERMINATOR);
                send(client->client_fd, error_msg, strlen(error_msg), 0);
            }
            return false;
    }

    if(client->current_msg[1] != ' ') {

    }
    return true;

*/


void handle_client_disconnection(Client *client, Worker_Thread* thread_context) {

}
void handle_in_chat_lobby(Client* client, Worker_Thread* thread_context) {
    char room_list_msg[MAX_MESSAGE_LEN_FROM_SERVER] = {};
    bool rooms_avail = false;
    char command = client->current_msg[0];

    switch(command) {
        case CMD_ROOM_CREATE_REQUEST:
            create_chat_room(client,thread_context);
            break;
        case CMD_ROOM_JOIN_REQUEST:
            break;
        case CMD_ROOM_LIST_REQUEST:
            send_avail_rooms(client, thread_context);
            break;
        case CMD_EXIT:
            handle_client_disconnection(client, thread_context);
            break;
        default:
            send_message_to_client(client, ERR_PROTOCOL_INVALID_STATE_CMD, 
                "Invalid command for lobby state\n", thread_context);
            break;
    }

}

void create_chat_room(Client *client, Worker_Thread *thread_context){
    char success_msg[MAX_MESSAGE_LEN_FROM_SERVER];
    bool rooms_avail = false;

    for(int i = 0; i < MAX_ROOMS; i++) {
        pthread_mutex_lock(&SERVER_ROOMS[i].room_lock);
        if(SERVER_ROOMS[i].in_use == false) {
            SERVER_ROOMS[i].in_use = true;
            SERVER_ROOMS[i].first_client = client;
            SERVER_ROOMS[i].last_client = client;
            SERVER_ROOMS[i].num_clients = 1;
            strcpy(SERVER_ROOMS[i].room_name, &client->current_msg[2]);
            client->room_index = i;
            rooms_avail = true;
            pthread_mutex_unlock(&SERVER_ROOMS[i].room_lock);
            break; 
        }
        pthread_mutex_unlock(&SERVER_ROOMS[i].room_lock);
    }

    if(!rooms_avail) {
        send_message_to_client(client, ERR_SERVER_ROOM_FULL,
            "Room creation failed: Maximum number of rooms reached\n", thread_context);
    } else {
        client->state = IN_CHAT_ROOM;
            snprintf(success_msg, sizeof(success_msg), 
            "Room created successfully: %s\n", &client->current_msg[2]);
        send_message_to_client(client, CMD_ROOM_CREATE_OK, success_msg, thread_context);
    }
}


void send_avail_rooms(Client *client, Worker_Thread* thread_context) {

    char room_list_msg[MAX_MESSAGE_LEN_FROM_SERVER] = "=== Available Chat Rooms ===\n\n";
    bool rooms_avail = false;

    log_message("MSG Recieved %s\n",client->current_msg);

    for(int i = 0; i < MAX_ROOMS; i++) {
        pthread_mutex_lock(&SERVER_ROOMS[i].room_lock);
        if(SERVER_ROOMS[i].in_use == true) {
            log_message("here \n");

            sprintf(room_list_msg + strlen(room_list_msg), "Room %d: %s\n", i, SERVER_ROOMS[i].room_name);
            rooms_avail = true;
        }
        pthread_mutex_unlock(&SERVER_ROOMS[i].room_lock);

    }

    if(!rooms_avail) {
        sprintf(room_list_msg + strlen(room_list_msg),"No chat rooms available!\n","Use the create room command to start your own chat room.\n");
    }

    send_message_to_client(client,CMD_ROOM_LIST_RESPONSE,room_list_msg, thread_context);
}



void handle_in_chat_room(Client* client, Worker_Thread* thread_context){

}
void broadcast_message_to_room(Client* sender,Worker_Thread* thread_context){

}
