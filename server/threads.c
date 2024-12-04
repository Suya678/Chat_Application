
// Local
#include "threads.h" // For our own declarations and constants

#include "chat_message_handler.h" // For send_message_to_client()
#include "client_handler.h" // Contains the function that the threads will run after being set up
// Library
#include <errno.h> // For errno
#include <fcntl.h>
#include <pthread.h>	 // For pthread_create
#include <stdint.h>		 // For uint64_t
#include <string.h>		 // For strlen, strerror, memset
#include <sys/eventfd.h> // For eventfd, EFD_NONBLOCK
#include <sys/socket.h>	 // For send
#include <unistd.h>		 // For write, close


/**
 * @brief Initializes and creates worker threads for processing client
 * connections.
 *
 * This function for each worker thread:
 * - Initializes a non-blocking 'eventfd', which the main thread can use to pass
 * new incoming client fds
 * - Zeroes out the num_of_clients and epoll_fd fields
 *
 * @param worker_threads Array of Worker_Thread structures to initialize
 * @note If eventfd or pthread_create system calls fail, the function will exit
 * the process
 * @see process_client_connections() in "connection_handler.c" The function each
 * worker thread will run
 */
void setup_threads(Worker_Thread worker_threads[]) {
	LOG_INFO("Initializing %d worker threads\n", MAX_THREADS);
	memset(worker_threads, 0, sizeof(Worker_Thread) * MAX_THREADS);
	for (int i = 0; i < MAX_THREADS; i++) {
		worker_threads[i].notification_fd = eventfd(0, EFD_NONBLOCK);
		if (worker_threads[i].notification_fd == -1) {
			LOG_ERROR("Failed to create event_fd for thread %d: %s\n", i, strerror(errno));
			print_erro_n_exit("Could not create event_fd");
		}

		sem_init(&worker_threads[i].sem, 0, 1);

		if (pthread_mutex_init(&worker_threads[i].num_of_clients_lock, NULL) != 0) {
			LOG_ERROR("Failed to initialize mutex for thread %d: %s\n", i, strerror(errno));
			print_erro_n_exit("Could not worker thread num of clients mutex");
		}
		if (pthread_create(&worker_threads[i].id, NULL, process_client_connections,
						   &worker_threads[i]) != 0) {
			LOG_ERROR("pthread_create for for thread %d: %s\n", i, strerror(errno));
			print_erro_n_exit("pthread_create failed");
		}
		LOG_INFO("Successfully initialized worker thread %d\n", i);
	}
	LOG_INFO("Successfully initialized all worker threads\n");
}

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
	const char *capacity_err_msg = "\x2B Sorry, the server is currently at full "
								   "capacity. Please try again later!\r\n";
	const char *connection_error_msg =
		"\x2C Sorry, there was an error connecting to the server. Please try "
		"again!\r\n";
	LOG_INFO("Attempting to distribute new client with (fd=%d)\n", client_fd);

	// Maintains the next worker to assign client to in the round-robin
	static int worker_index = 0;

	// Even though client_fd is an int, it needs to be converted for compatability
	// with event_fd
	uint64_t client_fd_as_uint64 = (uint64_t)client_fd;

	int num_attempts = 0;

	// Finds a worker thread not at capacity,otherwise reject
	pthread_mutex_lock(&workers[worker_index].num_of_clients_lock);
	while (workers[worker_index].num_of_clients >= MAX_CLIENTS_PER_THREAD) {
		pthread_mutex_unlock(&workers[worker_index].num_of_clients_lock);

		worker_index = (worker_index + 1) % MAX_THREADS;
		num_attempts++;

		if (num_attempts == MAX_THREADS) {
			send_message_to_client(client_fd, ERR_SERVER_FULL, capacity_err_msg);
			LOG_INFO("Server at capacity - rejecting client (fd=%d) with msg  %s\n", client_fd, capacity_err_msg);

			if (close(client_fd) == -1) {
				LOG_ERROR("Failed to close client fd %d: %s\n", client_fd, strerror(errno));
			}
			return;
		}
		pthread_mutex_lock(&workers[worker_index].num_of_clients_lock);
	}
	workers[worker_index].num_of_clients++;
	pthread_mutex_unlock(&workers[worker_index].num_of_clients_lock);

	LOG_INFO("Assigned client (fd=%d) to worker thread %d (current number of clients=%d)\n",
			 client_fd, worker_index, workers[worker_index].num_of_clients);
	sem_wait(&workers[worker_index].sem);

	// Notify chosen worker thread of new client using eventfd
	if (write(workers[worker_index].notification_fd, &client_fd_as_uint64, sizeof(uint64_t)) ==
		-1) {
		LOG_ERROR("Failed to notify worker thread %d of new client (fd=%d): %s\n", worker_index,
				  client_fd, strerror(errno));
		sem_post(&workers[worker_index].sem);
		send_message_to_client(client_fd, ERR_CONNECTING, connection_error_msg);
		if (close(client_fd) == -1) {
			LOG_ERROR("Failed to close client fd %d: %s\n", client_fd, strerror(errno));
		}
		pthread_mutex_lock(&workers[worker_index].num_of_clients_lock);
		workers[worker_index].num_of_clients--;
		pthread_mutex_unlock(&workers[worker_index].num_of_clients_lock);
		return;
	}

	worker_index = (worker_index + 1) % MAX_THREADS;
}
