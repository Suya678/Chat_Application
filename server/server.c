#include "server.h"
#include "protocol.h"
 #include <sys/epoll.h>
#define PORT_NUMBER 30000
#define BACKLOG 20



void print_erro_n_exit(char *msg);
void process_new_client(Worker_Thread *thread_data, int epoll_fd);
int setupServer(int port_number, int backlog);
void format_msg (char* buffer, uint8_t error_code, const char* msg);
bool validate_msg(char *msg, char *return_msg);
void *process_client_connections(void *worker);
void setup_threads(Worker_Thread worker_threads[]);
void distribute_client(int client_fd, Worker_Thread workers[]);
void register_with_epoll(int epoll_fd, int target_fd);
void process_client_message(Client *client, Worker_Thread* thread_data);
void handle_disconnection(Client *client);
void handle_client_message(Client *client);
void setup_new_cleint(Worker_Thread *thread_data, int epoll_fd);
void setup_new_user(Worker_Thread *thread_data, int client_fd);
void init_server_rooms();
void handle_connected_clients(Client *client);
void handle_in_room_clients(Client *client);
void handle_in_lobby_clients(Client *client);


Room SERVER_ROOMS[MAX_ROOMS] = {};

Client *find_client_by_fd(Worker_Thread *thread_data, int fd);
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int main() {
    int server_listen_fd, client_fd;
    Worker_Thread worker_threads[MAX_THREADS];
    init_server_rooms();
    setup_threads(worker_threads);
    
    server_listen_fd = setupServer(PORT_NUMBER, BACKLOG);
    log_message("Wating for connection on Port %d \n", PORT_NUMBER);   

    while(1) {

       client_fd = accept4(server_listen_fd, NULL, NULL, SOCK_NONBLOCK);
        if(client_fd == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;  
            }
            perror("accept failed");
            continue;
        }
        distribute_client(client_fd, worker_threads); 
    }

    close(server_listen_fd);
    return 0;
}


void init_server_rooms() {        
    for(int i = 0; i < MAX_ROOMS; i++) {
        pthread_mutex_init(&SERVER_ROOMS[i].room_lock, NULL);
    }
}




void setup_threads(Worker_Thread worker_threads[]){
    for(int i = 0; i < MAX_THREADS; i++) {
        worker_threads[i].notification_fd = eventfd(0, EFD_NONBLOCK);
        worker_threads[i].num_of_clients = 0;
        pthread_mutex_init(&worker_threads[i].worker_lock, NULL);

        if(worker_threads[i].notification_fd == -1) {
            print_erro_n_exit("Could not create eventfd");
        }
        if(pthread_create(&worker_threads[i].id, NULL, process_client_connections,&worker_threads[i]) != 0) {
             print_erro_n_exit("pthread_create failed");
        }
    }

}



void register_with_epoll(int epoll_fd, int target_fd) {
    struct epoll_event event_config;
    event_config.events = EPOLLIN | EPOLLET;
    event_config.data.fd = target_fd;
    
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, target_fd, &event_config) == -1) {
        perror("Failed to register fd with epoll");
    }
}






void setup_new_user(Worker_Thread *thread_data, int client_fd) {
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



/**
 * Variadic wrapper around printf  for thread safety
 */
void log_message(const char *format_str, ...) {
    va_list args;
    va_start(args, format_str);
    
    pthread_mutex_lock(&log_mutex);
    vprintf(format_str, args);
    fflush(stdout);  
    pthread_mutex_unlock(&log_mutex);
    
    //For compatibility but can be ignored in Gcc and clang compilers
    va_end(args);
}





void *process_client_connections(void *worker){
    int event_count; 
    Worker_Thread *thread_data  = (Worker_Thread *)worker;

    struct epoll_event event_queue[MAX_CLIENTS_PER_THREAD + 1];

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);    

    if(epoll_fd == -1) {
        print_erro_n_exit("Could not create epoll fd");
    }

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
            process_client_message(user,thread_data);

        }       

        }

}

void process_new_client(Worker_Thread *thread_data, int epoll_fd) {
    char welcome_msg[MAX_MESSAGE_LEN_FROM_SERVER];
    sprintf(welcome_msg, "%c%s", CMD_WELCOME_REQUEST, 
    "WELCOME TO THE SERVER: "
    "THIS IS A FAMILY FRIENDLY SPACE\n"
    "NO CURSING "
    "OR YOU WILL BE SPANKED\n"
    "Please enter Your USER NAME\n\r\n");

    uint64_t client_fd;
    if(read(thread_data->notification_fd, &client_fd, sizeof(uint64_t)) == -1) {
        if(errno != EAGAIN) {
            perror("Failed to read from eventfd");
        }
        return;
    }
    thread_data->num_of_clients++;

    register_with_epoll(epoll_fd,(int) client_fd);
    setup_new_user(thread_data,(int) client_fd);
    log_message("Thread %lu got new client fd: %d\n", (unsigned long)thread_data->id, client_fd);

    send(client_fd,welcome_msg, strlen(welcome_msg),0);
}



Client *find_client_by_fd(Worker_Thread *thread_data, int fd) {
    for(int i = 0; i < MAX_CLIENTS_PER_THREAD; i++) {
        if(thread_data->clients[i].client_fd == fd) {
            log_message("FOUND CLIENT\n");
            return &thread_data->clients[i];
        }
    }
    return NULL; 
}







/**
 * Distrubtes incoming clients to worker threads via event_fd
 */
void distribute_client(int client_fd, Worker_Thread workers[]) {
    static int worker_index = 0;
    uint64_t value = (uint64_t) client_fd;

    int num_attempts = 0;

    while(workers[worker_index].num_of_clients >= MAX_CLIENTS_PER_THREAD) {
        worker_index = (worker_index + 1) % MAX_THREADS;
        num_attempts++;
        if(num_attempts == MAX_THREADS) {
            close(client_fd);
            perror("Connection with a incoming client closed as the threads are full");
            return;
        }
    }

    if(write(workers[worker_index].notification_fd, &value, sizeof(uint64_t)) == -1) {
        perror("Failed to notify a worker thread of a new connection:");
        return;
    }

    worker_index = (worker_index + 1) % MAX_THREADS;

}











/**
 * Sets up the server with the provided port number and backlog.
 * @param port_number The port number to be used for the server
 * @param backlog The maximum number of clients that can be held up in the queue
 * @return The server socket file descriptor on success, on failure, this function exits the process
 */
int setupServer(int port_number, int backlog){
    int server_fd;

    struct sockaddr_in server_address = {
        .sin_family = AF_INET,
        .sin_port = htons(port_number),
        .sin_addr.s_addr = INADDR_ANY
    };
   
    server_fd = socket(AF_INET,SOCK_STREAM | SOCK_CLOEXEC, 0);
    if(server_fd == -1) {
        print_erro_n_exit("Socket creation failed");
    }
   
    if(bind(server_fd, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
        print_erro_n_exit("Bind failed");
    }

    if(listen(server_fd, backlog) == -1) {
        print_erro_n_exit("Listen call failed");
    }

    return server_fd;  
}



/** 
* Prints an error message and exits the program
* @param char *msg msg to be printed before exiting
*/
void print_erro_n_exit(char *msg) {
    if(errno != 0) {
        perror(msg);
    } else {
        fprintf(stderr, "%s\n", msg);
    }
    exit(EXIT_FAILURE);
}
