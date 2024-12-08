#include "protocol.h"
#include "user_interface.h"
#include <arpa/inet.h>
#include <ctype.h> // For isspace()
#include <ncurses.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define INPUT_WINDOW_HEIGHT 3
#define OUTPUT_WINDOW_HEIGHT 20

int connect_to_server(const char *server_ip, int port);
void get_username(int socket_fd);
void send_message(int socket_fd, const char *message);
void *receive_message(void *socket_fd);
void parse_server_msg(const char *buffer);
void *handle_user_input(void *socket_fd);
bool validate_and_format(const char *input, char *output, size_t out_size);
void send_command(int socket_fd, const char *message);
void exit_client(int socket_fd);
void set_global_bool(volatile bool *is_variable, pthread_mutex_t *mutex, bool state);
bool handle_commands(const char *input, char *output, size_t out_size);

typedef struct {
    const char *command;
    unsigned char cmd_code;
    int argc;
    const char *format;
} CommandFormat;

CommandFormat commands[] = {
    {"submit", 0x02, 1, "\x02 %s\r\n"},   // CMD_USERNAME_SUBMIT
    {"/create", 0x03, 1, "\x03 %s\r\n"},  // CMD_ROOM_CREATE_REQUEST
    {"/join", 0x05, 1, "\x05 %s\r\n"},    // CMD_ROOM_JOIN_REQUEST
    {"/exit", 0x01, 0, "\x01 dummy\r\n"}, // CMD_EXIT
    {"/msg", 0x07, 1, "\x07 %s\r\n"},     // CMD_MESSAGE_SEND 
    {"/leave", 0x06, 0, "\x06 %s\r\n"},   // CMD_LEAVE_ROOM  
    {"/list", 0x04, 0, "\x04 dummy\r\n"}  // CMD_ROOM_LIST_REQUEST
};

pthread_mutex_t room_state_mutex = PTHREAD_MUTEX_INITIALIZER; // mutex for is_in_room
pthread_mutex_t run_state_mutex = PTHREAD_MUTEX_INITIALIZER;  // mutex for is_running
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;      // mutex for ncurses windows

volatile bool is_running = true;  // changing to false signals threads to stop
volatile bool is_in_room = false; // track if user is in room

/**
 * @brief Main function for initializing the client application.
 *
 * This function connects to the server.
 * Sets up threads for handling input and receiving messages, and ensures proper cleanup.
 *
 * @return 0 on successful execution.
 */
int main(int argc, char *argv[]) {
    if (argc != 3) // ensure execution includes server ip and port number
    {
        fprintf(stderr, "Usage: %s <Server IP> <Port Number>\n", argv[0]);
        exit(1);
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int socket_fd = connect_to_server(server_ip, server_port);
    pthread_t thread1, thread2;

    ui_init(); // initialize user interface

    ui_msg_display(info_win, &print_mutex, "\tConnected to server at IP: %s PORT: %d\n", server_ip, server_port);

    ui_msg_display(info_win, &print_mutex,
                   " Thank you for choosing the Quantum Chatroom as your chatroom of choice\n"
                   " The descriptive HELP message below can be shown at anytime by typing HELP!\n"
                   "\n List of commands:\n"
                   "\t/exit -this will allow you to close the client *NOT AVAILABLE WHEN ENTERING USERNAME*\n"
                   "\t/create 'enter room name' -this will allow you to create and enter a room\n"
                   "\t/list -this will allow you to view available rooms\n"
                   "\t/join 'enter room NUMBER' -this will allow you to join a room\n"
                   "\t/leave -this will allow you to leave a room\n"
                   "\n For a list of commands available in a room or not in a room type HELP\n");

    char buffer[MAX_MESSAGE_LEN_FROM_SERVER];
    int n = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        ui_msg_display(info_win, &print_mutex, "%s\n", buffer + 1);
    }

    get_username(socket_fd);

    // create threads
    pthread_create(&thread1, NULL, handle_user_input, &socket_fd);
    pthread_create(&thread2, NULL, receive_message, &socket_fd);

    // wait for threads to finish
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // cleanup
    pthread_mutex_destroy(&room_state_mutex);
    pthread_mutex_destroy(&run_state_mutex);
    close(socket_fd);
    ui_cleanup();

    return 0;
}

/**
 * @brief Establishes a TCP connection to the specified server and port.
 *
 * @param server_ip The IP address of the server (IPv4).
 * @param port The port number to connect to.
 * @return The socket file descriptor on success, or -1 on failure.
 */
int connect_to_server(const char *server_ip, int port) {
    int socket_fd;
    struct sockaddr_in serv_addr;

    // creating socket
    socket_fd = socket(AF_INET, SOCK_STREAM,
                       0); // af_inet specifies ipv4 and sock_stream specifies socket type and 0 specifies tcp
    if (socket_fd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &serv_addr.sin_addr);
    serv_addr.sin_port = htons(port);

    // connect to the server
    if (connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        close(socket_fd);
        exit(1);
    }

    return socket_fd;
}

/**
 * @brief Prompts the user to enter a valid username and sends it to the server.
 *
 * This function continuously prompts the user to enter a username until a valid one is provided.
 * It ensures the username meets the following criteria:
 * - It is not empty.
 * - It does not exceed max username length.
 * - It does not contain spaces or whitespace characters.
 * - It is not reserved
 *
 * Once a valid username is entered, it is formatted and sent to the server.
 *
 * @param socket_fd The socket file descriptor used for communication with the server.
 * @return None
 */
void get_username(int socket_fd) {
    char username[MAX_USERNAME_LEN];
    while (1) {
        // Display prompt in the input window
        ui_input_prompt(input_win, &print_mutex,
                        "Enter username (Must not contain spaces and be no more than 31 characters in length): ");

        int result = wgetnstr(input_win, username, MAX_USERNAME_LEN);

        if (result == ERR) {
            ui_msg_display(output_win, &print_mutex, "Error reading username. Try again.\n");
            continue;
        }

        // Check if the username length exceeds the limit
        if (strlen(username) > MAX_USERNAME_LEN - 1) {

            ui_msg_display(output_win, &print_mutex,
                           "Username too long. Maximum allowed is %d characters. Try again.\n", MAX_USERNAME_LEN - 1);
            continue;
        }

        username[strcspn(username, "\n")] = '\0'; // Remove trailing newline if any

        // Check if input is empty
        if (strlen(username) == 0) {
            ui_msg_display(output_win, &print_mutex, "Username cannot be empty. Try again.\n");
            continue;
        }

        // Check for whitespace in the username
        bool has_whitespace = false;
        for (size_t i = 0; i < strlen(username); i++) {
            if (isspace(username[i])) {
                has_whitespace = true;
                break;
            }
        }

        if (has_whitespace) {

            ui_msg_display(output_win, &print_mutex, "Username cannot contain spaces or whitespace. Try again.\n");
            continue;
        }

        // Check for reserved usernames
        if (strcmp(username, "/exit") == 0 || strcmp(username, "HELP") == 0) {
            ui_msg_display(output_win, &print_mutex, "Username cannot be \"/exit\" or \"HELP\". Try again.\n");
            continue;
        }

        // If all checks passed, exit the loop
        ui_msg_display(output_win, &print_mutex, "\t\n\nLive Long And Prosper %s\n\n", username);
        break;
    }

    char formatted_command[256];
    snprintf(formatted_command, sizeof(formatted_command), "\x02 %s\r\n", username);
    send_message(socket_fd, formatted_command);
}

/**
 * @brief Sends a message to the server.
 *
 * This function sends a formatted message string through the specified socket.
 * It ensures the message is sent in its entirety and checks for errors during the process.
 * If an error occurs, the function terminates the client with an appropriate error message.
 *
 * @param socket_fd The socket file descriptor used for communication with the server.
 * @param message A pointer to the null-terminated string containing the message to be sent.
 *
 * @return None
 */
void send_message(int socket_fd, const char *message) {
    int n = send(socket_fd, message, strlen(message), 0);
    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
}

/**
 * @brief Receives messages from the server and processes them.
 *
 * This function runs in a separate thread and continuously listens for
 * messages from the server. It synchronizes access using a semaphore
 * (`msg_semaphore`) and checks the client state (`is_running`) to determine
 * whether to continue processing. Messages received are passed to
 * `parse_server_msg` for further interpretation and handling.
 *
 * @param socket_fd A pointer to the socket file descriptor used for communication with the server.
 *
 * @return NULL Always returns NULL when the thread exits.
 */
void *receive_message(void *socket_fd) {
    int fd = *(int *)socket_fd;
    char buffer[MAX_MESSAGE_LEN_FROM_SERVER];

    while (1) {
        pthread_mutex_lock(&run_state_mutex);
        if (!is_running) {
            pthread_mutex_unlock(&run_state_mutex);
            break;
        }
        pthread_mutex_unlock(&run_state_mutex);

        int n = recv(fd, buffer, MAX_MESSAGE_LEN_FROM_SERVER - 1, 0);
        if (n < 0) {
            perror("ERROR reading from socket");
            exit(1);
        }

        else if (n == 0) {
            ui_msg_display(output_win, &print_mutex, "Server has terminated connection\n");
            exit_client(fd);
            break;
        }
        buffer[n] = '\0';
        parse_server_msg(buffer);
    }
    return NULL;
}

/**
 * @brief Parses and handles messages received from the server
 *
 * This function interprets server messages based on their command code
 * (the first byte of the message) and performs appropriate actions
 * such as updating client state, displaying server responses, or handling
 * room-related commands.
 *
 * @param buffer The message buffer received from the server.
 * @param socket_fd The socket file descriptor used for communication with the server
 *
 * @return None
 */
void parse_server_msg(const char *buffer) {
    // Extract the command code (first byte of the message)
    unsigned char cmd_code = (unsigned char)buffer[0];

    switch (cmd_code) {
    case CMD_WELCOME_REQUEST:
        ui_msg_display(output_win, &print_mutex, " Server: Welcome request received. Please submit your username.\n");
        break;

    case CMD_ROOM_CREATE_OK:
        ui_msg_display(output_win, &print_mutex, " Server: Room created successfully\nPlease type a message:\n");
        set_global_bool(&is_in_room, &room_state_mutex, true);
        break;

    case CMD_ROOM_LIST_RESPONSE:
        ui_msg_display(output_win, &print_mutex, " Server: Room list received:\n%s\n", buffer + 1);
        break;

    case CMD_ROOM_JOIN_OK:

        ui_msg_display(output_win, &print_mutex, " Server: Room joined successfully\nPlease type a message:\n");
        set_global_bool(&is_in_room, &room_state_mutex, true);
        break;

    case CMD_ROOM_MSG: { //*note to Marc, this is where my bug was, reading \r from server caused ncurses issue

        char sanitized[MAX_MESSAGE_LEN_FROM_SERVER];
        int sanitized_index = 0;

        // Copy buffer + 1 while skipping \r
        for (int i = 1; buffer[i] != '\0' && i < MAX_MESSAGE_LEN_FROM_SERVER; i++) {
            if (buffer[i] != '\r') { // Skip carriage return
                sanitized[sanitized_index++] = buffer[i];
            }
        }
        sanitized[sanitized_index] = '\0'; // Null terminate sanitized string
        ui_msg_display(msg_win, &print_mutex, " %s", sanitized);
        break;
    }

    case CMD_ROOM_LEAVE_OK:
        ui_msg_display(output_win, &print_mutex, " Server: You have left the room.\n");
        set_global_bool(&is_in_room, &room_state_mutex, false);
        break;

    default:
        ui_msg_display(output_win, &print_mutex, "\n Server: %s\n", buffer + 1);
        break;
    }
}

/**
 * @brief Handles user input and sends commands to the server.
 *
 * This function continuously listens for user input from the standard input.
 * Depending on whether the user is in a room or not, it prompts the user to
 * enter a command or a message. The input is validated, formatted, and then
 * sent to the server. If the user inputs `/exit`, the function terminates the client.
 *
 * @param socket_fd  A pointer to the socket file descriptor used for communication with the server.
 *
 * @return Always returns NULL after exiting the input loop.
 */
void *handle_user_input(void *socket_fd) {
    int fd = *(int *)socket_fd;
    char command[MAX_CONTENT_LEN];
    while (1) {
        ui_input_prompt(input_win, &print_mutex, "> ");

        pthread_mutex_lock(&run_state_mutex);
        if (!is_running) { // Exit if client is stopping
            pthread_mutex_unlock(&run_state_mutex);
            break;
        }
        pthread_mutex_unlock(&run_state_mutex);

        // Get user input
        if (wgetnstr(input_win, command, MAX_CONTENT_LEN)) {
            ui_msg_display(output_win, &print_mutex, " Input error\n");
            break;
        }

        command[strcspn(command, "\n")] = '\0'; // Null terminate command

        if (strlen(command) == 0) {
            ui_msg_display(output_win, &print_mutex, " Invalid input, cannot be empty.\n");
            continue;
        }

        if (strcmp(command, "/exit") == 0) {
            send_command(fd, command);
            exit_client(fd);
            break;
        }

        if (strcmp(command, "HELP!") == 0) {
            ui_msg_display(output_win, &print_mutex,
                           "\n List of commands:\n"
                           "\t/exit -this will allow you to close the client *NOT AVAILABLE WHEN ENTERING USERNAME*\n"
                           "\t/create 'enter room name' -this will allow you to create and enter a room\n"
                           "\t/list -this will allow you to view available rooms\n"
                           "\t/join 'enter room NUMBER' -this will allow you to join a room\n"
                           "\t/leave -this will allow you to leave a room\n"
                           "\n For a list of commands available in a room or not in a room type HELP\n");
            continue;
        }

        if (strcmp(command, "HELP") == 0) {

            ui_msg_display(output_win, &print_mutex,
                           "\n List of commands available when NOT IN a room:\n"
                           "\t/exit , /create , /join 'room #' , /list\n"
                           "\n List of commands available when IN a room:\n"
                           "\t/exit , /leave\n\n");
            continue;
        }
        send_command(fd, command);

        // Update message window with sent message
        if (is_in_room) {
            ui_msg_display(msg_win, &print_mutex, " You: %s\n", command);
        }
    }
    return NULL;
}

/**
 * @brief Validates user input and formats it for server transmission.
 *
 * This function ensures the user's input adheres to the expected command format or
 * message structure. Depending on the user's state (in a room or not) it validates
 * the command against a predefined set of commands or formats the input as a message.
 * If the input is valid the function will format the command or message into the output buffer
 * for server transmission.
 *
 * @param input A string contain the user's input
 * @param output A buffer to store the formatted output for server transmission
 * @param out_size The size of the output buffer
 *
 * @return boolean value indicating if the input is valid or not
 */
bool validate_and_format(const char *input, char *output, size_t out_size) {

    if (!is_running) {
        ui_msg_display(output_win, &print_mutex,
                       " Server has terminated the connection.\nNo further commands can be processed.\n");
        return false;
    }

    bool validate;

    if (input[0] == '/') {
        validate = handle_commands(input, output, out_size);
        return validate;
    } else if (is_in_room) {
        if (strlen(input) > MAX_CONTENT_LEN) {
            ui_msg_display(output_win, &print_mutex, "\n Message too long please keep it under 128 characters\n");
            return false;
        }
        // handle_message_command;
        snprintf(output, out_size, "\x07 %s\r\n", input);
        return true;
    } else {
        ui_msg_display(output_win, &print_mutex, "Invalid input. type HELP for list of commands\n");
        return false;
    }
    ui_msg_display(output_win, &print_mutex, "Unknown Command. Type HELP for a list of commands.\n");
    return false;
}

/**
 * @brief Validates and sends a user command to the server
 *
 * This function ensures that the entered user input is not empty.
 * If valid, it prepares the command for transmission by formatting it
 * through a helper function, then sends it to the server.
 * If validation fails, an appropriate error message is displayed.
 *
 * @param socket_fd The socket file descriptor used for communication with the server
 * @param message A string containing the user's input command
 * @return None
 */
void send_command(int socket_fd, const char *message) {
    if (strlen(message) == 0) {
        ui_msg_display(output_win, &print_mutex, " Invalid input. Command cannot be empty.\n");
        return;
    }

    char formatted_command[MAX_MESSAGE_LEN_TO_SERVER];

    if (validate_and_format(message, formatted_command, sizeof(formatted_command))) {
        send_message(socket_fd, formatted_command);
    } else {
        ui_msg_display(output_win, &print_mutex, " Failed to send command. Please retry\n\n");
    }
}

/**
 * @brief Exits the client
 *
 * This function changes the global variable 'is_running' to false
 * to signal all threads to stop running, it then closes the socket connection.
 *
 * @param socket_fd The socket file descriptor to close
 *
 * @return None
 */
void exit_client(int socket_fd) {
    ui_msg_display(output_win, &print_mutex, " Exiting client\n");
    set_global_bool(&is_running, &run_state_mutex, false);
    close(socket_fd);
}

/**
 * @brief Sets a global boolean variable in a thread-safe manner.
 *
 * This function locks the provided mutex, updates the boolean variable
 * to the specified state, and then unlocks the mutex.
 *
 * @param is_variable Pointer to the boolean variable to be updated.
 * @param mutex Pointer to the mutex protecting the boolean variable.
 * @param state The new state to assign to the boolean variable.
 *
 * @return None
 */
void set_global_bool(volatile bool *is_variable, pthread_mutex_t *mutex, bool state) {
    pthread_mutex_lock(mutex);
    *is_variable = state;
    pthread_mutex_unlock(mutex);
}

/**
 * @brief Helper to validate_and_format() incase command starts with '/'
 *
 * This function handles the logic when a user enters a command that starts with '/'
 *
 * @param input A string contain the user's input
 * @param output A buffer to store the formatted output for server transmission
 * @param out_size The size of the output buffer
 *
 * @return boolean value indicating if the input is valid or not
 */
bool handle_commands(const char *input, char *output, size_t out_size) {
    char cmd[512], arg1[MAX_CONTENT_LEN];
    int num_args = sscanf(input, "%s %s", cmd, arg1);

    if (is_in_room) {
        if (strcmp(cmd, "/leave") == 0 || strcmp(cmd, "/exit") == 0) {
            for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
                if (strcmp(cmd, commands[i].command) == 0) {
                    snprintf(output, out_size, commands[i].format, arg1);
                    return true;
                }
            }
        } else {
            ui_msg_display(output_win, &print_mutex,
                           "\n Invalid command. Available commands while in a room:\n\t/leave, /exit.\n");
            return false;
        }
    } else {
        if (strcmp(cmd, "/create") == 0 || strcmp(cmd, "/join") == 0 || strcmp(cmd, "/list") == 0 ||
            strcmp(cmd, "/exit") == 0) {
            // Format the command if valid
            for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
                if (strcmp(cmd, commands[i].command) == 0) {
                    if (num_args - 1 == commands[i].argc) {
                        snprintf(output, out_size, commands[i].format, arg1);
                        return true;
                    } else {
                        ui_msg_display(output_win, &print_mutex,
                                       "\n Improper Usage. Type HELP for a list of commands.\n");
                        return false;
                    }
                }
            }
        } else {
            ui_msg_display(
                output_win, &print_mutex,
                "\n Invalid command. Available commands while not in a room are:\n\t/create, /join, /list, /exit.\n");
            return false;
        }
    }
    return false; // saftey return because compiler complained
}
