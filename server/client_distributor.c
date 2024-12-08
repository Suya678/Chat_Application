
// Local
#include "client_state_manager.h" // For send_message_to_client()
#include "logger.h"               // Has the logging function for LOG_INFO, LOG_SERVER_ERROR, LOG_WARNING
// Library
#include "errno.h"   // For errno
#include "string.h"  // For strerror
#include <pthread.h> // For pthread_create
#include <stdint.h>  // For uint64_t
#include <unistd.h>  // For write, close

static int find_worker_not_at_capacity(Worker_Thread workers[]);

static void handle_write_error(Worker_Thread workers[], int worker_index, int client_fd);

/**
 * @brief Distributes new client connections across worker threads using
 * round-robin dispatching
 *
 * Attempts to assign the client to the next available worker thread that isn't
 * at capacity. If all threads are at capacity, rejects the connection. Uses
 * eventfd to notify worker threads of new clients.
 *
 * @param client_fd file descriptor of the newly accepted client connection
 * @param workers array of worker threads to distribute clients across
 */
void distribute_client(int client_fd, Worker_Thread workers[]) {
    const char *capacity_err_msg = "Sorry, the server is currently at full "
                                   "capacity. Please try again later!\r\n";

    // Even though client_fd is an int, it needs to be converted for compatability
    // with event_fd
    uint64_t client_fd_as_uint64 = (uint64_t)client_fd;

    LOG_INFO("Attempting to distribute new client with (fd=%d)\n", client_fd);

    int worker_assigned_index = find_worker_not_at_capacity(workers);

    if (worker_assigned_index == -1) {
        send_message_to_client(client_fd, ERR_SERVER_FULL, capacity_err_msg);
        if (close(client_fd) == -1) {
            LOG_SERVER_ERROR("Failed to close client fd %d: %s\n", client_fd, strerror(errno));
        }
        return;
    }

    LOG_INFO("Assigned client (fd=%d) to worker thread %lu (current number of "
             "clients=%d)\n",
             client_fd, worker_assigned_index, workers[worker_assigned_index].id);

    if (sem_wait(&workers[worker_assigned_index].new_client) == -1) {
        LOG_SERVER_ERROR("sem_Wait failed in distribute_client: %s\n", strerror(errno));
    }

    // Notify chosen worker thread of new client using eventfd
    if (write(workers[worker_assigned_index].notification_fd, &client_fd_as_uint64, sizeof(uint64_t)) == -1) {
        handle_write_error(workers, worker_assigned_index, client_fd);
        return;
    }
}
/**
 * @brief Handles write error that occurs when trying to write a new client fd
 * to the notification fd in the worker thread
 *
 * Performs cleanup when writing to worker's notification fd fails:
 * 1. Posts back to semaphore
 * 2. Sends error message to client
 * 3. Closes client connection
 * 4. Decrements worker's client count
 *
 * @param workers Array of worker threads
 * @param worker_index Index of worker that failed
 * @param client_fd Client connection to clean up
 */
static void handle_write_error(Worker_Thread workers[], int worker_index, int client_fd) {
    const char *connection_error_msg = "Sorry, there was an error connecting to the server. Please try "
                                       "again!\r\n";

    LOG_SERVER_ERROR("Failed to notify worker thread %d of new client (fd=%d): %s\n", worker_index, client_fd,
                     strerror(errno));

    if (sem_post(&workers[worker_index].new_client) == -1) {
        LOG_SERVER_ERROR("sem_wait in distribute_client fialed: \n", strerror(errno));
    }
    send_message_to_client(client_fd, ERR_CONNECTING, connection_error_msg);
    if (close(client_fd) == -1) {
        LOG_SERVER_ERROR("Failed to close client fd %d: %s\n", client_fd, strerror(errno));
    }
    pthread_mutex_lock(&workers[worker_index].num_of_clients_lock);
    workers[worker_index].num_of_clients--;
    pthread_mutex_unlock(&workers[worker_index].num_of_clients_lock);
}

/**
 * @brief Finds next available worker thread that can accept new clients
 *
 * Uses round-robin selection with. Maintains a static index
 * for distribution. Checks each worker's current client count. Returns first
 * worker found under capacity. Returns -1 if all workers are at capacity.
 *
 * @param workers Array of worker threads to search
 * @return index of selected worker, or -1 if all workers at capacity
 */
static int find_worker_not_at_capacity(Worker_Thread workers[]) {
    static int worker_index = 0;
    int num_attempts = 0;

    // Finds a worker thread not at capacity,otherwise reject
    pthread_mutex_lock(&workers[worker_index].num_of_clients_lock);
    while (workers[worker_index].num_of_clients >= MAX_CLIENTS_PER_THREAD) {
        pthread_mutex_unlock(&workers[worker_index].num_of_clients_lock);
        worker_index = (worker_index + 1) % MAX_THREADS;
        if (num_attempts++ == MAX_THREADS) {
            return -1;
        }
        pthread_mutex_lock(&workers[worker_index].num_of_clients_lock);
    }

    workers[worker_index].num_of_clients++;
    pthread_mutex_unlock(&workers[worker_index].num_of_clients_lock);
    int selected_worker = worker_index;
    worker_index = (worker_index + 1) % MAX_THREADS;

    return selected_worker;
}