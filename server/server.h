#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>

#include "protocol.h"
#include "semaphore.h"
#include "stdbool.h"

void log_message(const char *format_str, ...);

#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"

// GOT HELP FORM CHATGP2 FOR THIS MACRO and GO THE the ANSI ESCAPE MACROS FROM IT AS WELL
#ifdef LOG
#define LOG_ERROR(format, ...)                                                                     \
	log_message(ANSI_BOLD ANSI_RED "ERROR: " format ANSI_RESET, ##__VA_ARGS__)
#define LOG_INFO(format, ...)                                                                      \
	log_message(ANSI_BOLD ANSI_GREEN "INFO: " format ANSI_RESET, ##__VA_ARGS__)
#else
#define LOG_ERROR(format, ...) ((void)0)
#define LOG_INFO(format, ...) ((void)0)
#endif


// MAX THREADS NEEDED
#define MAX_THREADS 2
#define MAX_CLIENTS_PER_THREAD 1000 // How many client does each thread handles

#define MAX_CLIENTS_ROOM 40						   // Max clients per room
#define MAX_ROOMS 50							   // Max rooms
#define MAX_CLIENTS (MAX_CLIENTS_ROOM * MAX_ROOMS) // Total possible clients

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

typedef struct Worker_Thread {
	pthread_t id;
	int num_of_clients;
	int notification_fd;
	int epoll_fd;
	Client clients[MAX_CLIENTS_PER_THREAD];
	pthread_mutex_t num_of_clients_lock;
	sem_t sem;

} Worker_Thread;

typedef struct Room {
	struct Client *clients[MAX_CLIENTS_ROOM];
	char room_name[MAX_ROOM_NAME_LEN + 1];
	int num_clients;
	bool in_use;
	pthread_mutex_t room_lock;
} Room;

extern Room SERVER_ROOMS[MAX_ROOMS];


void init_server_rooms();

int setup_server(int port_number, int backlog);

void print_erro_n_exit(char *msg);


#endif
