
//Local
#include "threads.h"            // For our own declarations and constants
#include "client_handler.h" // Contains the function that the threads will run after being set up


//Library
#include <string.h>     // For strlen, strerror, memset
#include <errno.h>      // For errno
#include <unistd.h>     // For write, close
#include <pthread.h>    // For pthread_create
#include <sys/eventfd.h>// For eventfd, EFD_NONBLOCK
#include <sys/socket.h> // For send
#include <stdint.h>     // For uint64_t



/**
 * @brief Initializes and creates worker threads for processing client connections.
 *
 * This function for each worker thread:
 * - Initializes a non-blocking 'eventfd', which the main thread can use to pass new incoming client fds
 * - Zeroes out the num_of_clients and epoll_fd fields
 *
 * @param worker_threads Array of Worker_Thread structures to initialize
 * @note If eventfd or pthread_create system calls fail, the function will exit the process
 * @see process_client_connections() in "connection_handler.c" The function each worker thread will run
 */
void setup_threads(Worker_Thread worker_threads[]){
    memset(worker_threads, 0,sizeof(Worker_Thread) * MAX_THREADS);
    for (int i = 0; i < MAX_THREADS; i++) {

        worker_threads[i].notification_fd = eventfd(0, EFD_NONBLOCK);
        if(worker_threads[i].notification_fd == -1) {
            print_erro_n_exit("Could not create eventfd");
        }

        if(pthread_create(&worker_threads[i].id, NULL, process_client_connections,&worker_threads[i]) != 0) {
             print_erro_n_exit("pthread_create failed");
        }
    }

}


/**
 * @brief Distributes new client connections across worker threads using round-robin dispatching
 * 
 * Attempts to assign the client to the next available worker thread that isn't at capacity.
 * If all threads are at capacity, rejects the connection. Uses eventfd to notify
 * worker threads of new clients.
 *
 * @param client_fd file descriptor of the newly accepted client connection
 * @param workers array of worker threads to distribute clients across
 */
void distribute_client(int client_fd, Worker_Thread workers[]) {
    const char* capacity_err_msg =      "\x2B Sorry, the server is currently at full capacity. Please try again later!\r\n";
    const char* connection_error_msg =  "\x2C Sorry, there was an error connecting to the server. Please try again!\r\n";

    // Maintains the next worker to assing client to in the round robin
    static int worker_index = 0;


    // Even though client_fd is an int, it needs to be converted for compatability wiht event_fd
    uint64_t client_fd_as_uint64 = (uint64_t) client_fd;

    int num_attempts = 0;

    // Finds a worker thread not at capacity,otherwise reject
    while(workers[worker_index].num_of_clients >= MAX_CLIENTS_PER_THREAD) {
        worker_index = (worker_index + 1) % MAX_THREADS;
        num_attempts++;
        if(num_attempts == MAX_THREADS) {
            send(client_fd, capacity_err_msg, strlen(capacity_err_msg), 0);
            close(client_fd);
            log_message("Connection rejected: Server at capacity (all threads full)\n");
            return;
        }
    }

    // Notify chosen worker thread of new client using eventfd
    if(write(workers[worker_index].notification_fd, &client_fd_as_uint64, sizeof(uint64_t)) == -1) {
        send(client_fd, connection_error_msg, strlen(connection_error_msg), 0);
        close(client_fd);
        log_message("Failed to notify worker thread of new connection: %s\n", strerror(errno));
        return;
    }
    worker_index = (worker_index + 1) % MAX_THREADS;

}
