#include "user_interface.h"
#include <ncurses.h>
#include <pthread.h>
#include <stdarg.h> //for the variadic function
#include <stdio.h>
#include <stdlib.h>

WINDOW *boarder_win = NULL; // just for formatting other windows
WINDOW *output_win = NULL;  // server messages realted to operation
WINDOW *msg_win = NULL;     // chatroom messages
WINDOW *info_win = NULL;    // greeting messages and instructions
WINDOW *input_win = NULL;   // user input

/**
 * @brief Initializes the user interface
 *
 * This function uses the ncurses library to get the screen dimensions and
 * ensure the terminal is an appropriate size. There are two windows created
 * (boarder and input) and three sub windows (output,msg, and info).
 *
 * @param None
 *
 * @return None
 */
void ui_init() {
    initscr();
    cbreak();
    curs_set(true);
    keypad(stdscr, TRUE);

    // Get screen dimensions
    int screen_height, screen_width;
    getmaxyx(stdscr, screen_height, screen_width);

    // Ensure terminal size is sufficient
    if (screen_height < OUTPUT_WINDOW_HEIGHT + INPUT_WINDOW_HEIGHT + 1 || screen_width < 6) {
        endwin();
        fprintf(stderr, "Terminal too small. Please resize and try again.\n");
        exit(1);
    }

    // Create the border window
    boarder_win = newwin(OUTPUT_WINDOW_HEIGHT, screen_width, 0, 0);
    box(boarder_win, 0, 0); // Draw box around the window
    wrefresh(boarder_win);

    // Allocate widths for the three windows
    int msg_width = screen_width * 0.2;
    int info_width = screen_width * 0.5;
    int output_width = screen_width - info_width - msg_width - 4; // Remaining space for info, adjusted for borders

    int inner_height = OUTPUT_WINDOW_HEIGHT - 2; // Subtract 2 for the border

    // Draw vertical lines
    mvwvline(boarder_win, 1, output_width + 1, 0, inner_height);             // Line between output and msg
    mvwvline(boarder_win, 1, output_width + msg_width + 2, 0, inner_height); // Line between msg and info
    wrefresh(boarder_win);

    // Create the left (output) window
    output_win = derwin(boarder_win, inner_height, output_width, 1, 1);
    scrollok(output_win, TRUE);
    wrefresh(output_win);

    // Create the middle (message) window
    msg_win = derwin(boarder_win, inner_height, msg_width, 1, output_width + 2);
    scrollok(msg_win, TRUE);
    wrefresh(msg_win);

    // Create the right (info) window
    info_win = derwin(boarder_win, inner_height, info_width, 1, output_width + msg_width + 3);
    scrollok(info_win, TRUE);
    wrefresh(info_win);

    // Create the input window at the bottom
    input_win = newwin(INPUT_WINDOW_HEIGHT, screen_width, OUTPUT_WINDOW_HEIGHT, 0);
    box(input_win, 0, 0);
    mvwprintw(input_win, 1, 1, "> ");
    wrefresh(input_win);
}

/**
 * @brief Cleans up the user interface
 *
 * This function deletes all the windows that were created and it
 * finishes off by ensuring the terminal is restored to its normal
 * working condition.
 *
 * @param None
 *
 * @return None
 */
void ui_cleanup() {
    delwin(output_win);
    delwin(msg_win);
    delwin(info_win);
    delwin(input_win);
    delwin(boarder_win);
    endwin();
}

/**
 * @brief Displays a formatted message to a specified ncurses window
 *
 * This function prints a string the an ncurses window using a
 * variable number of arguments. It first locks the provided mutex
 * to ensure thread safe output. After it has printed it will unlock
 * the mutex. It utilizes `va_start` and `va_end` to access the
 * additional arguments.
 *
 * @param window Pointer to an ncurses window where the message will be displayed
 * @param mutex Pointer to a mutex that will be locked and unlocked during the
 * critical section
 * @param string a format string
 * @param ... Optional arguments corresponding to the format specifiers in the string
 *
 * @return None
 */
void ui_msg_display(WINDOW *window, pthread_mutex_t *mutex, const char *string, ...) {
    va_list args;
    va_start(args, string);

    pthread_mutex_lock(mutex);
    vw_printw(window, string, args);
    wrefresh(window);
    pthread_mutex_unlock(mutex);

    va_end(args);
}

/**
 * @brief Displays a prompt for entering input in the input window.
 *
 * This function prints a line prompting the user to enter data in the given
 * ncurses window. Unlike `ui_msg_display()`, this function directly uses `mvwprintw()` to position
 * the text at a specific location and `wclrtoeol()` to ensure that any remaining
 * characters on the line are cleared.
 *
 * @param window Pointer to an ncurses window where the message will be displayed
 * @param mutex Pointer to a mutex that will be locked and unlocked during the
 * critical section
 *
 * @return None
 */
void ui_input_prompt(WINDOW *window, pthread_mutex_t *mutex, const char *string) {
    pthread_mutex_lock(mutex);
    mvwprintw(window, 1, 1, "%s", string);
    wclrtoeol(window); // Clear the rest of the line
    wrefresh(window);  // Refresh the input window
    pthread_mutex_unlock(mutex);
}