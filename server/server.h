#ifndef SERVER_H
#define SERVER_H

#include "protocol.h"
#include "stdbool.h"
#include <pthread.h> 



// MAX THREADS NEEDED
#define MAX_THREADS  2
#define MAX_CLIENTS_PER_THREAD 1000                   // How many client does each thread handles

#define MAX_CLIENTS_ROOM 40                        // Max clients per room
#define MAX_ROOMS 50                               // Max rooms
#define MAX_CLIENTS (MAX_CLIENTS_ROOM * MAX_ROOMS)  // Total possible clients
                                



        
typedef enum ClIENT_STATE {
    AWAITING_USERNAME,
    IN_CHAT_LOBBY,       
    IN_CHAT_ROOM,        
} ClIENT_STATE;



typedef struct Client {
    int client_fd;
    char name[MAX_USERNAME_LEN + 1];
    ClIENT_STATE state;
    int room_index;
    bool in_use;
    char current_msg[MAX_MESSAGE_LEN_TO_SERVER * 3];
} Client;



typedef struct  Worker_Thread {
    pthread_t id;
    int num_of_clients;
    int notification_fd;
    int epoll_fd;
    int client_fds[MAX_CLIENTS_PER_THREAD];
    Client clients[MAX_CLIENTS_PER_THREAD];
} Worker_Thread;


typedef struct Room {
    struct Client* clients[MAX_CLIENTS_ROOM];
    char room_name[MAX_ROOM_NAME_LEN + 1];
    int num_clients;
    bool in_use;
    pthread_mutex_t room_lock;
} Room;

extern Room SERVER_ROOMS[MAX_ROOMS];

void log_message(const char *format_str, ...);
void init_server_rooms();
int setup_server(int port_number, int backlog);
void print_erro_n_exit(char *msg);
#endif