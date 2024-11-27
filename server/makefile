CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = server
OBJS = server.o chat_message_handler.o

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

server.o: server.c server.h
	$(CC) $(CFLAGS) -c server.c -o server.o

chat_message_handler.o: chat_message_handler.c chat_message_handler.h
    $(CC) $(CFLAGS) -c chat_message_handler.c -o chat_message_handler.o



clean:
	rm -rf $(OBJS) $(TARGET)

