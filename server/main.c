
#define _GNU_SOURCE // Enables GNU extensions required for the ccept4()
                    // function

// Local headers
#include "client_distributor.h" // Custom header containing thread-related definitions and functions
#include "connection_handler.h" // Contains the function that the threads will run after being set up, handles all functionality related to when the the client is succesfully connected
#include "logger.h"             // Has the logging functin for LOG_INFO, LOG_SERVER_ERROR, LOG_WARNING
#include "server_config.h"      // Custom header containing server configuration

// System/Library headers
#include <errno.h>       // Provides error codes like EAGAIN, EWOULDBLOCK and errno variable
#include <netinet/ip.h>  // IP protocol definitions and constants
#include <netinet/tcp.h> // TCP protocol specific options and constants like TCP_KEEPINTVL
#include <stdio.h>       // For printf(), this funciton is used only once in the program
#include <string.h>      // For strerror() to convert error numbers to messages
#include <sys/eventfd.h> // For eventfd, EFD_NONBLOCK
#include <sys/socket.h>  // Socket-related functions and constants (accept4(), SOCK_NONBLOCK, SOMAXCONN)
#include <unistd.h>      // close() function

#define PORT_NUMBER 30000 // PORT NUMBER FOR THE SERVER TO LISTEN ON
#define BACKLOG SOMAXCONN // DEFINED IN socket.h

// This is a global that will also be used by other files
Room SERVER_ROOMS[MAX_ROOMS] = {};

static void init_server_rooms();
static int setup_server(int port_number, int backlog);
static int set_socket_keep_alive(int socket);
static void setup_threads(Worker_Thread worker_threads[]);
/**
 * @brief Main server loop that initializes the chat server and handles incoming
 * connections
 *
 * @note press ctrl c to exit the server
 */
int main()
{
    int server_listen_fd, client_fd;
    // these threads will manage the clients
    Worker_Thread worker_threads[MAX_THREADS];

    // Initialize all the rooms in the servers and worker threads
    init_server_rooms();
    setup_threads(worker_threads);
    LOG_INFO("Initialized %d rooms and %d worker threads for MAX: %d clients\n", MAX_ROOMS, MAX_THREADS, MAX_CLIENTS);

    // set up the server listening socket
    server_listen_fd = setup_server(PORT_NUMBER, BACKLOG);

    // printf is only ever used here
    printf("Waiting for connection on Port %d \n", PORT_NUMBER);

    while (1)
    {
        // Accept new connection with non-blocking socket
        client_fd = accept4(server_listen_fd, NULL, NULL, SOCK_NONBLOCK);

        if (client_fd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            LOG_SERVER_ERROR("Accept failed: %s\n", strerror(errno));
            continue;
        }
        LOG_INFO("New client connection accepted: fd=%d\n", client_fd);

        if (set_socket_keep_alive(client_fd) == -1)
        {
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
static int set_socket_keep_alive(int socket)
{
    int enable = 1;
    int idle_time = 5;
    int interval = 1;
    int probes = 2;

    LOG_INFO("Configuring keepalive for socket %d\n", socket, idle_time, interval, probes);

    if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)) == -1)
    {
        LOG_SERVER_ERROR("Error in setsockopt(SO_KEEPALIVE): %s\n", strerror(errno));
        return -1;
    }

    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &idle_time, sizeof(idle_time)) == -1)
    {
        LOG_SERVER_ERROR("Error in setsockopt(TCP_KEEPIDLE): %s\n", strerror(errno));
        return -1;
    }

    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval)) == -1)
    {
        LOG_SERVER_ERROR("Error in setsockopt(TCP_KEEPINTVL): %s\n", strerror(errno));
        return -1;
    }

    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &probes, sizeof(probes)) == -1)
    {
        LOG_SERVER_ERROR("Error in setsockopt(TCP_KEEPCNT): %s\n", strerror(errno));
        return -1;
    }
    LOG_INFO("Successfully configured keepalive for socket %d\n", socket);

    return 0;
}

/**
 * @brief Initializes and creates worker threads for processing client
 * connections.
 *
 * This function for each worker thread:
 * - Initializes a non-blocking 'eventfd', which the main thread can uses to
 * pass new incoming client fds
 * - Zeroes out the num_of_clients and epoll_fd fields
 *
 * @param worker_threads Array of Worker_Thread structures to initialize
 * @note If eventfd or pthread_create system calls fail, the function will exit
 * the process
 * @see process_client_connections() in "connection_handler.c" The function each
 * worker thread will run
 */
static void setup_threads(Worker_Thread worker_threads[])
{
    LOG_INFO("Initializing %d worker threads\n", MAX_THREADS);
    memset(worker_threads, 0, sizeof(Worker_Thread) * MAX_THREADS);
    for (int i = 0; i < MAX_THREADS; i++)
    {
        worker_threads[i].notification_fd = eventfd(0, EFD_NONBLOCK);
        if (worker_threads[i].notification_fd == -1)
        {
            print_erro_n_exit("Could not create event_fd in setup_threads");
        }

        if (sem_init(&worker_threads[i].new_client, 0, 1) == -1)
        {
            print_erro_n_exit("Could not initialize semaphore in setup_threads\n");
        }

        if (pthread_mutex_init(&worker_threads[i].num_of_clients_lock, NULL) != 0)
        {
            print_erro_n_exit("Could not worker thread num of clients mutex");
        }
        if (pthread_create(&worker_threads[i].id, NULL, process_client_connections, &worker_threads[i]) != 0)
        {
            print_erro_n_exit("Failed to create worker thread");
        }
        LOG_INFO("Successfully initialized worker thread %d\n", i);
    }
    LOG_INFO("Successfully initialized all worker threads\n");
}

/**
 * @brief Initializes the server rooms by clearing the room data and setting up
 * mutexes.
 *
 * @note This function will exit the program if any room mutex initialization
 * fails
 */
static void init_server_rooms()
{
    memset(&SERVER_ROOMS, 0, sizeof(Room) * MAX_ROOMS);
    for (int i = 0; i < MAX_ROOMS; i++)
    {
        if (pthread_mutex_init(&SERVER_ROOMS[i].room_lock, NULL) != 0)
        {
            print_erro_n_exit("Failed to initialize mutex for a server room in "
                              "init_server_room");
        }
    }
}

/**
 * @brief Initializes a server socket, binds it to the specified port, and sets
 * it up to listen for incoming connections
 *
 * @param port_number The port number to be used for the server
 * @param backlog The maximum number of clients that can be held up in the queue
 *
 * @return int A file descriptor for the server socket, or -1 on failure.
 *
 * @note ON failure during the socket creation, binding or listening system
 * calls, this function will print an error and exit.
 */
static int setup_server(int port_number, int backlog)
{
    int server_fd;
    int value = 1;
    struct sockaddr_in server_address = {
        .sin_family = AF_INET, .sin_port = htons(port_number), .sin_addr.s_addr = INADDR_ANY};

    server_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (server_fd == -1)
    {
        print_erro_n_exit("Socket creation failed");
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) == -1)
    {
        print_erro_n_exit("setsockopt failed");
    }
    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        print_erro_n_exit("Bind failed");
    }

    if (listen(server_fd, backlog) == -1)
    {
        print_erro_n_exit("Listen call failed");
    }

    return server_fd;
}
