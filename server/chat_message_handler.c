#include "chat_message_handler.h"


void handle_awaiting_username(Client* client);
void handle_in_chat_lobby(Client* client);
void handle_in_chat_room(Client* client);
void send_response_to_client(Client* client, const char* response, size_t length);
void broadcast_message_to_room(Room* room, const char* message, Client* sender);
void send_error_message(Client* client, const char* error_msg);
bool validate_command_state_and_format(Client *client);
bool check_command_format(Client* client);
void route_command(Client *client);



void process_client_message(Client* client, Worker_Thread* thread_context){
    char read_buffer[MAX_MESSAGE_LEN_TO_SERVER + 1];
    ssize_t bytes_recieved;
    while(1) {
        bytes_recieved = recv(client->client_fd,read_buffer,MAX_MESSAGE_LEN_TO_SERVER, 0);
        if(bytes_recieved == 0) {
            handle_client_disconnection(client);
            return;
        }
        if(bytes_recieved < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;  
            }
            handle_client_disconnection(client);
            return;
        }
        read_buffer[bytes_recieved] = '\0';

        strcat(client->current_msg,read_buffer);
        char *msg_term = strstr(client->current_msg,"\r\n");
        if(msg_term == NULL ) {
            continue;
        }
        *msg_term = '\0';
        route_client_command(client);
            
        memset(client->current_msg, 0, MAX_MESSAGE_LEN_TO_SERVER);

        return;
    }


}


void send_message_to_client(Client* client, char cmd_type, const char* message) {
    char message_buffer[MAX_MESSAGE_LEN_FROM_SERVER] = {};
    sprintf(message_buffer, "%c%s%s", cmd_type, message, MSG_TERMINATOR);
    ssize_t bytes_sent = send(client->client_fd, message_buffer, strlen(message_buffer), 0);
    if (bytes_sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        handle_client_disconnect(client);
    }
}




bool validate_msg_format(Client *client){
   if(strlen(client->current_msg) < 3) {
        send_message_to_client(client, ERR_PROTOCOL_INVALID_FORMAT,
        "Message too short\nCorrect format:[command char][space][message content][MSG_TERMINATOR]\n");
        return false;
   }

   if(client->current_msg[1] != ' ') {
        send_message_to_client(client, ERR_PROTOCOL_INVALID_FORMAT,
        "Missing space after command.\nCorrect format: [command char][space][message content][MSG_TERMINATOR]\n");
        return false;
   }

    if(client->current_msg[0] < CMD_EXIT || client->current_msg[0] > CMD_ROOM_MESSAGE_SEND) {
        send_message_to_client(client, ERR_PROTOCOL_INVALID_FORMAT,
        "Command not found\nCorrect format: [command char][space][message content][MSG_TERMINATOR]\n");
        return false;
    }

   return true;
}




void route_client_command(Client *client) {
    if(!validate_msg_format){
        return;
    }

    if(client->current_msg[0] == CMD_EXIT{
        handle_client_disconnection(client);
        return;
    }

    switch(client->state) {
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

    switch(client->state) {
        case AWAITING_USERNAME:
            if(command_type != CMD_USERNAME_SUBMIT ) {
                sprintf(error_msg, "%c Username required first\n%s", 
                    , MSG_TERMINATOR);
                send(client->client_fd, error_msg, strlen(error_msg), 0);
            }
            return false;
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



void handle_awaiting_username(Client* client) {
    char room_list_msg[MAX_MESSAGE_LEN_FROM_SERVER] = {};
    bool rooms_avail = false;

    strcpy(client->name,client->current_msg);
    client->state = IN_LOBBY;

    log_message("MSG Recieved %s\n",client->current_msg);
    sprintf(room_list_msg, "%c Welcome %s!\nAvailable Rooms:\n", 
            CMD_ROOM_LIST, client->current_msg);

    for(int i = 0; i < MAX_ROOMS; i++) {
        if(SERVER_ROOMS[i].in_use == true) {
            sprintf(room_list_msg + strlen(room_list_msg), "Room %d: %s\n", i, SERVER_ROOMS[i].room_name);
            rooms_avail = true;
        }
    }

    if(!rooms_avail) {
        strcat(room_list_msg, "No rooms available. Create your own!\r\n");
    } else {
        strcat(room_list_msg, "\r\n");
    }

    send(client->client_fd, room_list_msg, strlen(room_list_msg),0);



}

