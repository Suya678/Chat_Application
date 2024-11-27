#ifndef SERVER_H
#define SERVER_H

#define _GNU_SOURCE
#include "protocol.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <stdarg.h>

#define MAX_CLIENTS_ROOM 100     // Max clients per room
#define MAX_ROOMS 100            // Max rooms
#define MAX_CLIENTS (MAX_CLIENTS_ROOM * MAX_ROOMS)  // Total possible clients
                                
#define MAX_CLIENTS_PER_THREAD 32  // How many client does each thread handles

// MAX THREADS NEEDED
#define MAX_THREADS ((MAX_CLIENTS + MAX_CLIENTS_PER_THREAD -1) / MAX_CLIENTS_PER_THREAD)
        
typedef enum ClIENT_STATE {
    AWAITING_USERNAME,
    IN_CHAT_LOBBY,       
    IN_CHAT_ROOM,        
    DISCONNECTING
} ClIENT_STATE;



typedef struct Client {
    int client_fd;
    char name[MAX_USERNAME_LEN];
    ClIENT_STATE state;
    int room_index;
    bool in_use;
    char current_msg[MAX_MESSAGE_LEN_TO_SERVER];
    Client* next_client;
    Client* prev_client;
} Client;



typedef struct  Worker_Thread {
    pthread_t id;
    int num_of_clients;
    pthread_mutex_t worker_lock;
    uint64_t notification_fd;
    int client_fds[MAX_CLIENTS_PER_THREAD];
    Client clients[MAX_CLIENTS_PER_THREAD];
} Worker_Thread;


typedef struct Room {
    Client* first_client;
    Client* last_client;
    char room_name[MAX_ROOM_NAME_LEN];
    int num_clients;
    bool in_use;
    pthread_mutex_t room_lock;
} Room;




#endif