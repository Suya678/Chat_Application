#ifndef CHAT_MSG_HANDLER
#define CHAT_MSG_HANDLER

#include "server.h"
void process_client_message(Client* client, Worker_Thread* thread_context);

#endif