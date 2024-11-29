// Local
#include "client_handler.h" // For our own declarations and constants

#include "chat_message_handler.h" // For process_client_message()
#include "protocol.h"             // FOR Commands in the messaging protocol
#include "server.h"               // For Worker_Thread, Client structs

// Library
#include <errno.h>      // For errno, EAGAIN
#include <stdbool.h>    // For bool type
#include <stdint.h>     // For uint64_t
#include <stdio.h>      // For perror, sprintf
#include <string.h>     // For strlen, memset
#include <sys/epoll.h>  // For epoll functions, epoll_event struct
#include <sys/socket.h> // For send
#include <unistd.h>     // For read, EAGAIN

static void process_new_client(Worker_Thread *thread_context);
static void process_epoll_events(struct epoll_event event_queue[],
                                 int event_count,
                                 Worker_Thread *thread_context);
static bool register_with_epoll(int epoll_fd, int target_fd);
static Client *find_client_by_fd(Worker_Thread *thread_data, int fd);
static void setup_new_user(Worker_Thread *thread_data, int client_fd);

/**
 * @brief Registers the target_fd with epoll_fd
 *
 * @param epoll_fd The epoll  file descriptor
 * @param target_fd The file descriptor to register with epoll
 *
 * @return bool True if registration successful, false if it fails
 *
 */
static bool register_with_epoll(int epoll_fd, int target_fd) {
  struct epoll_event event_config;
  event_config.events = EPOLLIN;
  event_config.data.fd = target_fd;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, target_fd, &event_config) == -1) {
    perror("Failed to register fd with epoll");
    return false;
  }

  return true;
}

/**
 * @brief Initializes a client structure in a free spot in thread_data with the
 * client_fd
 *
 * @param thread_context Worker thread context containing data about the thread
 * including the client array
 * @param client_fd File descriptor associated with the new client
 *
 */
static void setup_new_user(Worker_Thread *thread_data, int client_fd) {
  for (int i = 0; i < MAX_CLIENTS_PER_THREAD; i++) {
    if (thread_data->clients[i].in_use == false) {
      memset(&thread_data->clients[i], 0, sizeof(Client));
      thread_data->clients[i].in_use = true;
      thread_data->clients[i].state = AWAITING_USERNAME;
      thread_data->clients[i].client_fd = client_fd;
      return;
    }
  }
  log_message(
      "Could not find an empty client struct to add the new connection to\n");
}

/**
 * @brief Processes epoll events for both new and existing client connections
 *
 * Iterates through the epoll event queue, handling two differnet types of
 * events:
 * 1. New client notifications from the main thread via the notification_fd.
 * 2. Messages from existing clients
 *
 * @param event_queue Array of epoll events to process
 * @param event_count Number of events in the queue
 * @param thread_context Worker thread context containing data about the thread
 *
 * @see process_client_message() Processes messages from existing clients in
 * chat_message_handler.c
 *
 */
static void process_epoll_events(struct epoll_event event_queue[],
                                 int event_count,
                                 Worker_Thread *thread_context) {
  for (int i = 0; i < event_count; i++) {
    // Recieved new client notification from the main thread
    if (event_queue[i].data.fd == (int)thread_context->notification_fd) {
      process_new_client(thread_context);
      continue;
    }

    // Handle exsisting client
    Client *user = find_client_by_fd(thread_context, event_queue[i].data.fd);
    if (user == NULL) {
      log_message("Error: Could not find client for fd %d\n",
                  event_queue[i].data.fd);
      continue;
    }
    process_client_message(user, thread_context);
  }
}

/**
 * @brief This function is indefinitely exectuted by a worker thread to handle
 * clients handed to it by the main thread via
 *
 * This funciton monitors and process events from clients connections. It
 * recives new clients via the the notification fd in the worker struct by the
 * main thread. Upon reciving the new client fd, it registers that in its epoll
 * instance and manaages the client.
 *
 * @param worker Pointer to Worker_Thread structure (cast from void*) containg
 * thread data, including epoll_fd and notification fd
 *
 * @note Thread runs indefinitely until process termination via ctrk c
 */
void *process_client_connections(void *worker) {
  Worker_Thread *thread_context = (Worker_Thread *)worker;

  // Size is to account for the notification fd
  // used by the main thread to signal new client connections
  struct epoll_event event_queue[MAX_CLIENTS_PER_THREAD + 1];

  thread_context->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (thread_context->epoll_fd == -1) {
    print_erro_n_exit("Could not create epoll fd");
  }

  if (!register_with_epoll(thread_context->epoll_fd,
                           thread_context->notification_fd)) {
    print_erro_n_exit("Could not register notification fd with epoll");
  }

  while (1) {
    int event_count = epoll_wait(thread_context->epoll_fd, event_queue,
                                 MAX_CLIENTS_PER_THREAD + 1, -1);
    if (event_count == -1) {
      perror("epoll wait failed");
      continue;
    }
    process_epoll_events(event_queue, event_count, thread_context);
  }
}

/**
 * @brief Process and set up a new client connection received from the main
 * thread
 *
 * Reads the new client file descriptor from the notification eventfd,
 * registers it with epoll, initializes the client data structure,
 * and sends a welcome message back to the client. The welcome message prompts
 * the user to enter their username.
 *
 * @param thread_context Worker thread context containing data about the thread
 *
 */
static void process_new_client(Worker_Thread *thread_context) {
  char welcome_msg[MAX_MESSAGE_LEN_FROM_SERVER];
  sprintf(welcome_msg, "%c%s", CMD_WELCOME_REQUEST,
          "WELCOME TO THE SERVER: "
          "THIS IS A FAMILY FRIENDLY SPACE\n"
          "NO CURSING "
          "OR YOU WILL BE SPANKED\n"
          "Please enter Your USER NAME\n\r\n");

  uint64_t value;

  if (read(thread_context->notification_fd, &value, sizeof(uint64_t)) == -1) {
    if (errno != EAGAIN) {
      perror("Failed to read from eventfd");
    }
    return;
  }

  int client_fd = (int)value;
  if (register_with_epoll(thread_context->epoll_fd, (int)client_fd) == false) {
    close((int)client_fd);
    return;
  }
  thread_context->num_of_clients++;

  setup_new_user(thread_context, (int)client_fd);
  log_message("Thread %lu got new client fd: %lu\n", thread_context->id,
              client_fd);
  send(client_fd, welcome_msg, strlen(welcome_msg), 0);
}

/**
 * @brief Finds the client structure in the thread_data associated with the fd
 * in the parameter
 *
 * @param thread_context Worker thread context containing data about the thread
 * @param fd File descriptor to search for
 *
 * @return Client* Pointer to found client structure, or NULL if not found
 *
 */
static Client *find_client_by_fd(Worker_Thread *thread_data, int fd) {
  for (int i = 0; i < MAX_CLIENTS_PER_THREAD; i++) {
    if (thread_data->clients[i].client_fd == fd) {
      return &thread_data->clients[i];
    }
  }
  return NULL;
}
