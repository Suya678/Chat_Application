#ifndef CHAT_MSG_HANDLER
#define CHAT_MSG_HANDLER
#include "server.h"

void process_client_message(Client* client, Worker_Thread* thread_context);
void send_message_to_client(Client* client, char cmd_type, const char* message, Worker_Thread* thread_context);
void handle_client_disconnection(Client* client, Worker_Thread *thread_context);



#endif