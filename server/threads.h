#ifndef THREADS_H
#define THREADS_H
#include "server.h"
void setup_threads(Worker_Thread worker_threads[]);
void distribute_client(int client_fd, Worker_Thread workers[]);
#endif