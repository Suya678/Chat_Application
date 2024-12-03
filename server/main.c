
#define _GNU_SOURCE // For accept4()

// Local
#include "server.h"
#include "threads.h"

// Library
#include <errno.h> // For errno, EAGAIN, EWOULDBLOCK
#include <stdio.h> // For perror()
#include <string.h>  // For strerror()
#include <sys/socket.h> // For accept4(), SOCK_NONBLOCK
#include <sys/types.h>
#include <unistd.h> // For close()

#include <netinet/ip.h>
#include <netinet/tcp.h>

#define PORT_NUMBER 30000
#define BACKLOG SOMAXCONN

int set_socket_keep_alive(int socket);

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
    LOG_INFO("Initialized %d rooms and %d worker threads for MAX: %d clients\n", MAX_ROOMS, MAX_THREADS, MAX_CLIENTS);

    // set up the server listening socket
    server_listen_fd = setup_server(PORT_NUMBER, BACKLOG);
    printf("Waiting for connection on Port %d \n", PORT_NUMBER);

    while (1) {
        // Accept new connection with non-blocking socket
        client_fd = accept4(server_listen_fd, NULL, NULL, SOCK_NONBLOCK);

        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            LOG_ERROR("Accept failed: %s\n", strerror(errno));
            continue;
        }
        LOG_INFO("New client connection accepted: fd=%d\n", client_fd);

        if (set_socket_keep_alive(client_fd) == -1) {
            continue;
        }

        LOG_INFO("Distributing client fd=%d to worker threads\n", client_fd);
        // Distribute new client connection to one of the worker threads
        distribute_client(client_fd, worker_threads);
    }
    // should never get here, press ctrl c to exit
    close(server_listen_fd);
    return 0;
}

/**
 * @brief Configures TCP keepalive settings for the specified socket.
 *
 * @param socket - The file descriptor the socket to configure
 *
 * @returns - 0 on success, -1 on failure
 * Ref:https://stackoverflow.com/questions/31426420/configuring-tcp-keepalive-after-accept
 */
int set_socket_keep_alive(int socket) {

    int enable = 1;
    int idle_time = 5;
    int interval = 1;
    int probes = 2;
    LOG_INFO("Configuring keepalive for socket %d\n",
             socket, idle_time, interval, probes);
    if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)) ==
        -1) {
        LOG_ERROR("Error in setsockopt(SO_KEEPALIVE): %s\n", strerror(errno));
        return -1;
    }

    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &idle_time,
                   sizeof(idle_time)) == -1) {
        LOG_ERROR("Error in setsockopt(TCP_KEEPIDLE): %s\n", strerror(errno));
        return -1;
    }

    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &interval,
                   sizeof(interval)) == -1) {
        LOG_ERROR("Error in setsockopt(TCP_KEEPINTVL): %s\n", strerror(errno));
        return -1;
    }

    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &probes, sizeof(probes)) ==
        -1) {
        LOG_ERROR("Error in setsockopt(TCP_KEEPCNT): %s\n", strerror(errno));
        return -1;
    }
    LOG_INFO("Successfully configured keepalive for socket %d\n", socket);

    return 0;
}
