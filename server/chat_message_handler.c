
// Local 
#include "chat_message_handler.h"   // For our own declarations and constants
#include "protocol.h"               // For command types, message length constants

// Library
#include <stdio.h>      // For sprintf, snprintf, perror
#include <string.h>     // For strstr, strcpy, strlen, memset
#include <stdlib.h>     // For atoi
#include <ctype.h>      // For isdigit
#include <errno.h>      // For errno, EAGAIN, EWOULDBLOCK
#include <stdbool.h>    // For bool type
#include <unistd.h>     // For close
#include <sys/socket.h> // For recv, send
#include <pthread.h>    // For pthread_mutex_lock/unlock
#include <sys/epoll.h>  // For epoll_ctl
#include <ctype.h>

static void send_message_to_client(Client* client, char cmd_type, const char* message);
static void handle_client_disconnection(Client* client, Worker_Thread *thread_context);
static void handle_awaiting_username(Client* client, Worker_Thread* thread_context);
static void handle_in_chat_lobby(Client* client, Worker_Thread* thread_context);
static void handle_in_chat_room(Client* client, Worker_Thread* thread_context);
static void broadcast_message_to_room(Client* sender,Worker_Thread* thread_context);
static void route_client_command(Client *client, Worker_Thread* thread_context);
static void send_avail_rooms(Client *client, Worker_Thread* thread_context);
static void create_chat_room(Client *client, Worker_Thread *thread_context);
static void join_chat_room(Client *client, Worker_Thread *thread_context);
static void cleanup_client(Client *client, Worker_Thread* thread_context);


void process_client_message(Client* client, Worker_Thread* thread_context){
    char read_buffer[MAX_MESSAGE_LEN_TO_SERVER + 1] = {};
    ssize_t bytes_recieved;
    bytes_recieved = recv(client->client_fd,read_buffer,MAX_MESSAGE_LEN_TO_SERVER, 0);
    if(bytes_recieved == 0) {
        handle_client_disconnection(client, thread_context);
        return;
    }

    if(bytes_recieved < 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            return;  
        }
        handle_client_disconnection(client, thread_context);
        return;
    }

    read_buffer[bytes_recieved] = '\0';

    char *temp = read_buffer;
    char *msg_term = strstr(read_buffer,"\r\n");

    while(msg_term !=NULL){
        *msg_term = '\0';
        strcat(client->current_msg,temp);

        route_client_command(client,thread_context);
        memset(client->current_msg, 0, MAX_MESSAGE_LEN_TO_SERVER * 3);
        temp = msg_term + 2;
        msg_term = strstr(temp,"\r\n");
    }

    if(*temp != '\0') {
        strcat(client->current_msg,temp);
    }

}


void send_message_to_client(Client* client, char cmd_type, const char* message) {
    char message_buffer[MAX_MESSAGE_LEN_FROM_SERVER] = {};
    sprintf(message_buffer, "%c %s%s", cmd_type, message, MSG_TERMINATOR);
    send(client->client_fd, message_buffer, strlen(message_buffer), 0);
}





bool validate_msg_format(Client *client, Worker_Thread* thread_context){
    //Check if message legnth is less than the minimum
   if(strlen(client->current_msg) < 3) {
        send_message_to_client(client, ERR_PROTOCOL_INVALID_FORMAT,
        "Message too short\nCorrect format:[command char][space][message content][MSG_TERMINATOR]\n");
        return false;
   }
    //Check if space is missing
   if(client->current_msg[1] != ' ') {
        send_message_to_client(client, ERR_PROTOCOL_INVALID_FORMAT,
        "Missing space after command.\nCorrect format: [command char][space][message content][MSG_TERMINATOR]\n");
        return false;
   }

    //Check if command is not valid
    if(client->current_msg[0] < CMD_EXIT || client->current_msg[0] > CMD_ROOM_MESSAGE_SEND) {
        send_message_to_client(client, ERR_PROTOCOL_INVALID_FORMAT,
        "Command not found\nCorrect format: [command char][space][message content][MSG_TERMINATOR]\n");
        return false;
    }

    //Check if content is empty
    char *content = &client->current_msg[2];
    while(*content == ' ' && *content != '\0') {
        content++;
    }
    if(*content == '\0') {
        send_message_to_client(client, ERR_MSG_EMPTY_CONTENT,
        "Content is Empty\nCorrect format: [command char][space][message content][MSG_TERMINATOR]\n");
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
    switch
    (client->state) {
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
        send_message_to_client(client,ERR_PROTOCOL_INVALID_STATE_CMD, "CMD not correct for client in awaiting username state\n");
        return;
    }

    strcpy(client->name,&client->current_msg[2]);
    client->state = IN_CHAT_LOBBY;
    send_avail_rooms(client, thread_context);

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
            join_chat_room(client,thread_context);
            break;
        case CMD_ROOM_LIST_REQUEST:
            send_avail_rooms(client, thread_context);
            break;
        default:
            send_message_to_client(client, ERR_PROTOCOL_INVALID_STATE_CMD, 
                "Invalid command for lobby state\n");
            break;
    }

}

int parse_room_number(Client *client) {
    char *room_index = &client->current_msg[2];
    
    if(!isdigit(*room_index)) {
        return -1;
    }

    if(room_index[1] != '\0') {
        if (!isdigit(room_index[1]) || room_index[2] != '\0') {
            return -1;
        }  
    }
    return atoi(room_index);
}


void cleanup_client(Client *client, Worker_Thread* thread_context){
    if(epoll_ctl(thread_context->epoll_fd, EPOLL_CTL_DEL, client->client_fd, NULL) == -1) {
        perror("epoll_ctl removing client fd failed");
    }
    close(client->client_fd);
    memset(client,0, sizeof(Client));
    thread_context->num_of_clients--;
}

//Some repetition needs to be refactored
void handle_client_disconnection(Client *client, Worker_Thread* thread_context) {
    ClIENT_STATE state= client->state;

    if(state == AWAITING_USERNAME || state == IN_CHAT_LOBBY) {
        cleanup_client(client, thread_context);
        return;
    }
    
    //Clenup room and client
    int room_index = client->room_index;
    pthread_mutex_lock(&SERVER_ROOMS[room_index].room_lock);
    for(int i = 0; i < MAX_CLIENTS_ROOM; i++) {
        if(SERVER_ROOMS[room_index].clients[i] == client) {
            SERVER_ROOMS[room_index].clients[i] = NULL;
            SERVER_ROOMS[room_index].num_clients--;
        }
    }

    if(SERVER_ROOMS[room_index].num_clients == 0) {
        memset(SERVER_ROOMS[room_index].room_name,0, sizeof(SERVER_ROOMS[room_index].room_name));
        SERVER_ROOMS[room_index].in_use = false;
    }
    cleanup_client(client, thread_context);
    pthread_mutex_unlock(&SERVER_ROOMS[room_index].room_lock);

}


void join_chat_room(Client* client, Worker_Thread* thread_context) {
    int room_index = parse_room_number(client);
    if(room_index == -1) {
        send_message_to_client(client, ERR_ROOM_NOT_FOUND, 
            "Invalid room number format. Must be a number between 0-99\n");
        return;
    }

    pthread_mutex_lock(&SERVER_ROOMS[room_index].room_lock);
    if(SERVER_ROOMS[room_index].in_use == false) {
        send_message_to_client(client, ERR_ROOM_NOT_FOUND,
        "Room does not exist\n");
        pthread_mutex_unlock(&SERVER_ROOMS[room_index].room_lock);
        return;
    }

    if(SERVER_ROOMS[room_index].num_clients == MAX_CLIENTS_ROOM) {
        send_message_to_client(client, ERR_ROOM_CAPACITY_FULL,
        "Cannot join room: Room is full\n");
        pthread_mutex_unlock(&SERVER_ROOMS[room_index].room_lock);
        return;
    }

    for(int i = 0; i < MAX_CLIENTS_ROOM; i++) {
        if(SERVER_ROOMS[room_index].clients[i] == NULL) {
            SERVER_ROOMS[room_index].clients[i] = client;
            SERVER_ROOMS[room_index].num_clients++;
            send_message_to_client(client, CMD_ROOM_JOIN_OK, 
            "Successfully joined room\n");
            client->state = IN_CHAT_ROOM;
            client->room_index = room_index;
            break;
        }
    }
    pthread_mutex_unlock(&SERVER_ROOMS[room_index].room_lock);
}



void create_chat_room(Client *client, Worker_Thread *thread_context){
    char success_msg[MAX_MESSAGE_LEN_FROM_SERVER];
    bool rooms_avail = false;

    for(int i = 0; i < MAX_ROOMS; i++) {
        pthread_mutex_lock(&SERVER_ROOMS[i].room_lock);
        if(SERVER_ROOMS[i].in_use == false) {
            SERVER_ROOMS[i].in_use = true;
            SERVER_ROOMS[i].num_clients = 1;
            strcpy(SERVER_ROOMS[i].room_name, &client->current_msg[2]);
            client->room_index = i;
            rooms_avail = true;
            SERVER_ROOMS[i].clients[0] = client;
            pthread_mutex_unlock(&SERVER_ROOMS[i].room_lock);
            break; 
        }
        pthread_mutex_unlock(&SERVER_ROOMS[i].room_lock);
    }

    if(!rooms_avail) {
        send_message_to_client(client, ERR_SERVER_ROOM_FULL,
            "Room creation failed: Maximum number of rooms reached\n");
    } else {
        client->state = IN_CHAT_ROOM;
            snprintf(success_msg, sizeof(success_msg), 
            "Room created successfully: %s\n", &client->current_msg[2]);
        send_message_to_client(client, CMD_ROOM_CREATE_OK, success_msg);
    }
}




void send_avail_rooms(Client *client, Worker_Thread* thread_context) {
    char room_list_msg[MAX_MESSAGE_LEN_FROM_SERVER] = "=== Available Chat Rooms ===\n\n";
    bool rooms_avail = false;

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

    send_message_to_client(client,CMD_ROOM_LIST_RESPONSE, room_list_msg);

}



void handle_in_chat_room(Client* client, Worker_Thread* thread_context){

    char msg[MAX_MESSAGE_LEN_FROM_SERVER];
    char command = client->current_msg[0];

    if(command != CMD_ROOM_MESSAGE_SEND) {
        //error msg
        return;
    }
    int room_index = client->room_index;
    pthread_mutex_lock(&SERVER_ROOMS[room_index].room_lock);

    sprintf(msg, "%s: %s", client->name,&client->current_msg[2]);
    for(int i = 0; i < MAX_CLIENTS_ROOM; i++) {
        if(SERVER_ROOMS[room_index].clients[i] != client && SERVER_ROOMS[room_index].clients[i] != NULL) {
            send_message_to_client(SERVER_ROOMS[room_index].clients[i],CMD_ROOM_MSG, msg);
        }
    }
    pthread_mutex_unlock(&SERVER_ROOMS[room_index].room_lock);


}









