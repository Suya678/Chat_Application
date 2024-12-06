#include "logger.h"

// System/Library headers
#include <errno.h>   // For errno
#include <pthread.h> // For threading functions and data types
#include <stdarg.h>  // For va_list, va_start, va_end
#include <stdio.h>   // For printf, fprintf, perror, fflush, stdout, stderr
#include <stdlib.h>  // For exit()
#include <time.h>    // For time, local_time_r

// The prgram is multithreaded so printing needs to be thread safe
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Variadic wrapper around printf for thread-safe
 *
 * @param format_str format string to be written to standard output. Can contain
 *                  format specifiers which will be replaced by values from
 *                   additional arguments.
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
        printf("%s║%s %s%s%s%s %s║%s ", ANSI_MAGENTA, ANSI_RESET, ANSI_BOLD, ANSI_CYAN, time_stamp, ANSI_RESET,
               ANSI_MAGENTA, ANSI_RESET);
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
