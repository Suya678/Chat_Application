# Variables
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -lncurses -lpthread
TARGET = client
SRC = main.c user_interface.c

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS)

# Clean up
clean:
	rm -f $(TARGET)