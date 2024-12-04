// Local
#include "server.h" // For our own declarations and constants

// Library
#include <errno.h>		// For errno
#include <netinet/in.h> // For sockaddr_in, AF_INET, INADDR_ANY, htons
#include <stdarg.h>		// For va_list, va_start, va_end
#include <stdio.h>		// For printf, fprintf, perror, fflush, stdout, stderr
#include <stdlib.h>		// For exit, EXIT_FAILURE
#include <string.h>		// For memset
#include <time.h>		// For time, local_time_r
#include <unistd.h>		// For close

// Will be manipulated by other files
Room SERVER_ROOMS[MAX_ROOMS] = {};
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;


/**
 * @brief Variadic wrapper around printf for thread-safe
 *
 * @param format_str format string to be written to standard output. Can contain
 *                  format specifiers which will be replaced by values from
 * additional arguments.
 * @param ... additional values to replace format specifiers in format_str
 */
void log_message(const char *format_str, ...) {
	va_list args;
	char time_stamp[200];
	struct tm t;
	va_start(args, format_str);
	time_t now = time(NULL);

	pthread_mutex_lock(&log_mutex);
	if (localtime_r(&now, &t) != NULL) {
		strftime(time_stamp, sizeof(time_stamp), "%Y:%m:%d:%T", &t);
		printf("%s║%s %s%s%s%s %s║%s ", ANSI_MAGENTA, ANSI_RESET, ANSI_BOLD, ANSI_CYAN, time_stamp,
			   ANSI_RESET, ANSI_MAGENTA, ANSI_RESET);
	} else {
		printf("localtime_r error\n");
	}
	printf("%s[TID- %lu]%s ", "\033[95;2m", pthread_self(), ANSI_RESET);

	vprintf(format_str, args);
	fflush(stdout);
	pthread_mutex_unlock(&log_mutex);
	va_end(args);
}


/**
 * @brief Initializes the server rooms by clearing the room data and setting up
 * mutexes.
 *
 * @note This function will exit the program if any room mutex initialization
 * fails
 */
void init_server_rooms() {
	memset(&SERVER_ROOMS, 0, sizeof(Room) * MAX_ROOMS);
	for (int i = 0; i < MAX_ROOMS; i++) {
		if (pthread_mutex_init(&SERVER_ROOMS[i].room_lock, NULL) != 0) {
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
int setup_server(int port_number, int backlog) {
	int server_fd;
	int value = 1;
	struct sockaddr_in server_address = {
		.sin_family = AF_INET, .sin_port = htons(port_number), .sin_addr.s_addr = INADDR_ANY};

	server_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (server_fd == -1) {
		print_erro_n_exit("Socket creation failed");
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) == -1) {
		print_erro_n_exit("setsockopt failed");
	}
	if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
		print_erro_n_exit("Bind failed");
	}

	if (listen(server_fd, backlog) == -1) {
		print_erro_n_exit("Listen call failed");
	}

	return server_fd;
}

/**
 * @brief Prints an error message and exits the program
 * @param char *msg msg to be printed before exiting
 */
void print_erro_n_exit(char *msg) {
	if (errno != 0) {
		perror(msg);
	} else {
		fprintf(stderr, "%s\n", msg);
	}
	exit(EXIT_FAILURE);
}
