#ifndef user_interface_h
#define user_interface_h

#include <ncurses.h>
#include <pthread.h>

#define INPUT_WINDOW_HEIGHT 3
#define OUTPUT_WINDOW_HEIGHT 20

extern WINDOW *output_win;
extern WINDOW *input_win;
extern WINDOW *boarder_win;
extern WINDOW *msg_win;
extern WINDOW *info_win;

void ui_init();
void ui_cleanup();
void ui_msg_display(WINDOW *window, pthread_mutex_t *mutex, const char *string, ...);
void ui_input_prompt(WINDOW *window, pthread_mutex_t *mutex, const char *string);

#endif
