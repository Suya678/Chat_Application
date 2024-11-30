
#define _GNU_SOURCE // For accept4()

// Local
#include "server.h"
#include "threads.h"

// Library
#include <errno.h>      // For errno, EAGAIN, EWOULDBLOCK
#include <stdio.h>      // For perror()
#include <sys/socket.h> // For accept4(), SOCK_NONBLOCK
#include <sys/types.h>
#include <unistd.h> // For close()

#define PORT_NUMBER 30000
#define BACKLOG 20

/**
 * @brief Main server loop that initializes the chat server and handles incoming
 * connections
 *
 * @note press ctrl c to exit the server
 */
int main() {
  int server_listen_fd, client_fd;
  // these threads will manage the clients
  Worker_Thread worker_threads[MAX_THREADS];

  // Initialize all the rooms in the servers and worker threads
  init_server_rooms();
  setup_threads(worker_threads);

  // set up the server listening socket
  server_listen_fd = setup_server(PORT_NUMBER, BACKLOG);
  log_message("Wating for connection on Port %d \n", PORT_NUMBER);

  while (1) {
    // Accept new connection with non-blocking socket
    client_fd = accept4(server_listen_fd, NULL, NULL, SOCK_NONBLOCK);
    // log_message("Num of clients: %d", num);

    if (client_fd == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      perror("Accept failed: ");
      continue;
    }
    log_message("received FD: %d\n", client_fd);

    // Distribute new client connection to one of the worker threads
    distribute_client(client_fd, worker_threads);
  }
  // should never get here, press ctrl c to exit
  close(server_listen_fd);
  return 0;
}