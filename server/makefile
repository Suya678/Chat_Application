CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = server
OBJS = main.o server.o chat_message_handler.o threads.o client_handler.o

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

server.o: server.c server.h
	$(CC) $(CFLAGS) -c server.c -o server.o

chat_message_handler.o: chat_message_handler.c chat_message_handler.h
	$(CC) $(CFLAGS) -c chat_message_handler.c -o chat_message_handler.o


client_handler.o: client_handler.c client_handler.h
	$(CC) $(CFLAGS) -c client_handler.c -o client_handler.o


threads.o: threads.c threads.h
	$(CC) $(CFLAGS) -c threads.c -o threads.o

clean:
	rm -rf $(OBJS) $(TARGET)

