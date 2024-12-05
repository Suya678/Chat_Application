#ifndef CLIENT_STATE_MANAGER
#define CLIENT_STATE_MANAGER

#include "server_config.h" // Custom header containing server configuration
void read_and_process_client_message(Client *client, Worker_Thread *thread_context);
void handle_client_disconnection(Client *client, Worker_Thread *thread_context);
void send_message_to_client(int client_fd, char cmd_type, const char *message);

#endif