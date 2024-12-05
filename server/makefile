CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = server
OBJS = main.o room_manager.o client_state_manager.o client_distributor.o connection_handler.o logger.o
LOG = 0
ifeq ($(LOG),1)
	CFLAGS += -DLOG
endif

# Default target
all: $(TARGET)



$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c server_config.h
	$(CC) $(CFLAGS) -c main.c -o main.o

room_manager.o: room_manager.c room_manager.h server_config.h
	$(CC) $(CFLAGS) -c room_manager.c -o room_manager.o

client_state_manager.o: client_state_manager.c client_state_manager.h server_config.h
	$(CC) $(CFLAGS) -c client_state_manager.c -o client_state_manager.o


connection_handler.o: connection_handler.c connection_handler.h server_config.h
	$(CC) $(CFLAGS) -c connection_handler.c -o connection_handler.o


client_distributor.o: client_distributor.c client_distributor.h server_config.h
	$(CC) $(CFLAGS) -c client_distributor.c -o client_distributor.o

logger.o: logger.o logger.h
	$(CC) $(CFLAGS) -c logger.c -o logger.o


clean:
	rm -rf $(OBJS) $(TARGET)

