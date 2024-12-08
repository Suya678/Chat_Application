
#ifndef LOGGER_H
#define LOGGER_H

void log_message(const char *format_str, ...);
void print_erro_n_exit(char *msg);

#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_YELLOW "\033[33m"

// GOT HELP FORM CHATGP2 FOR THIS MACRO and GOT THE the ANSI ESCAPE COLOR MACROS FROM
// IT AS WELL
#ifdef LOG
#define LOG_SERVER_ERROR(format, ...) log_message(ANSI_BOLD ANSI_RED "SERVER ERROR: " format ANSI_RESET, ##__VA_ARGS__)
#define LOG_INFO(format, ...) log_message(ANSI_BOLD ANSI_GREEN "INFO: " format ANSI_RESET, ##__VA_ARGS__)
#define LOG_USER_ERROR(format, ...) log_message(ANSI_BOLD ANSI_YELLOW "USER ERROR: " format ANSI_RESET, ##__VA_ARGS__)
#define LOG_CLIENT_DISCONNECT(format, ...) log_message(ANSI_BOLD ANSI_CYAN "CLIENT DISCONNECTED: " format ANSI_RESET, ##__VA_ARGS__)
#else
#define LOG_SERVER_ERROR(format, ...) ((void)0)
#define LOG_INFO(format, ...) ((void)0)
#define LOG_USER_ERROR(format, ...) ((void)0)
#define LOG_CLIENT_DISCONNECT(format, ...) ((void)0)
#endif

#endif
