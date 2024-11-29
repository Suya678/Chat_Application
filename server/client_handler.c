// Local
#include "client_handler.h"         // For our own declarations and constants
#include "server.h"                 // For Worker_Thread, Client structs
#include "protocol.h"               // FOR Commands in the messaging protocol
#include "chat_message_handler.h"   // For process_client_message()

// Library
#include <stdio.h>      // For perror, sprintf
#include <string.h>     // For strlen, memset
#include <errno.h>      // For errno, EAGAIN
#include <stdint.h>     // For uint64_t
#include <stdbool.h>    // For bool type
#include <unistd.h>     // For read, EAGAIN
#include <sys/epoll.h>  // For epoll functions, epoll_event struct
#include <sys/socket.h> // For send

static void process_new_client(Worker_Thread *thread_data, int epoll_fd);
static bool register_with_epoll(int epoll_fd, int target_fd);
static Client* find_client_by_fd(Worker_Thread *thread_data, int fd);
static void setup_new_user(Worker_Thread *thread_data, int client_fd);

static bool register_with_epoll(int epoll_fd, int target_fd) {
    struct epoll_event event_config;
    event_config.events = EPOLLIN;
    event_config.data.fd = target_fd;
    
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, target_fd, &event_config) == -1) {
        perror("Failed to register fd with epoll");
        return false;
    }

    return true;
}



static void setup_new_user(Worker_Thread *thread_data, int client_fd) {
    for(int i = 0; i< MAX_CLIENTS_PER_THREAD;i++) {
        if(thread_data->clients[i].in_use == false) {
            memset(&thread_data->clients[i],0, sizeof(Client));
            thread_data->clients[i].in_use = true;
            thread_data->clients[i].state = AWAITING_USERNAME;
            thread_data->clients[i].client_fd = client_fd;

          return;
        }
    }
    log_message("Could not find an empty client struct to add the new connection to");
}




void *process_client_connections(void *worker){
    int event_count; 
    Worker_Thread *thread_data  = (Worker_Thread *)worker;

    //+1 for the notification fd, the main thread will put the fd of the new client in that fd
    struct epoll_event event_queue[MAX_CLIENTS_PER_THREAD + 1];

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);    

    if(epoll_fd == -1) {
        print_erro_n_exit("Could not create epoll fd");
    }
    thread_data->epoll_fd =  epoll_fd;
    register_with_epoll(epoll_fd, thread_data->notification_fd);

    while(1) {
        event_count = epoll_wait(epoll_fd, event_queue, MAX_CLIENTS_PER_THREAD + 1, -1);
        if(event_count == -1) {
            perror("epoll wait failed");
            continue;
        }
        for(int i = 0; i < event_count; i++) {
            if(event_queue[i].data.fd == (int)thread_data->notification_fd) {
                    process_new_client(thread_data,epoll_fd);
                    continue;
            }
            Client *user = find_client_by_fd(thread_data,event_queue[i].data.fd);
            if (user == NULL) {
                log_message("Error: Could not find client for fd %d\n", event_queue[i].data.fd);
                continue;
            }
            process_client_message(user,thread_data);
        }       

    }
}


static void process_new_client(Worker_Thread *thread_data, int epoll_fd) {
    char welcome_msg[MAX_MESSAGE_LEN_FROM_SERVER];
    sprintf(welcome_msg, "%c%s", CMD_WELCOME_REQUEST, 
    "WELCOME TO THE SERVER: "
    "THIS IS A FAMILY FRIENDLY SPACE\n"
    "NO CURSING "
    "OR YOU WILL BE SPANKED\n"
    "Please enter Your USER NAME\n\r\n");

    uint64_t value;
    int client_fd;
    if(read(thread_data->notification_fd, &value, sizeof(uint64_t)) == -1) {
        if(errno != EAGAIN) {
            perror("Failed to read from eventfd");
        }
        return;
    }

    client_fd = (int) value;
    if(register_with_epoll(epoll_fd,(int) client_fd) == false) {
        close((int)client_fd);
        return;
    }
    thread_data->num_of_clients++;

    setup_new_user(thread_data,(int) client_fd);
    log_message("Thread %lu got new client fd: %d\n", (unsigned long)thread_data->id, client_fd);
    send(client_fd,welcome_msg, strlen(welcome_msg),0); 
}


static Client* find_client_by_fd(Worker_Thread *thread_data, int fd) {
    for(int i = 0; i < MAX_CLIENTS_PER_THREAD; i++) {
        if(thread_data->clients[i].client_fd == fd) {
            return &thread_data->clients[i];
        }
    }
    return NULL; 
}
