# Chat Server 

This is the server component of the Multi Thread Chat Application, handling multiple client connections and message routing.

## Related Documentation
- [Main Project Documentation](../README.md)
- [Client Documentation](../client/README.md)
- [Protocol documentation](./protocol.h)


## Features

- Handles messaging between multiple users(max users is easily configurable through macros in server_config.h)
- **Comprehensive logging**:
    - Provides three different log levels with colors using ANSI Escape codes: `LOG_INFO`, `LOG_USER_ERRROR`, `LOG_SERVER_ERROR` and `LOG_CLIENT_DISCONNECT`.
  

## Running the Server

```bash
cd server
make clean
make LOG=1 #Optional make  , if you would not like logging information
./server   #Run this if you already compiled it are in the directory
```
## Architecture

- **Thread Management**:
    - Supports up to `MAX_THREADS` worker threads, each handling up to `MAX_CLIENTS_PER_THREAD` clients concurrently.
    - Threads use:
        - `epoll` for  client connection management.
        - `notification_fd` for the main thread to notify a worker thread of a new client connection.
        - `pthread_mutex` for thread-safe operations.

- **Client state Management**:
  - Each client goes through the following states:
     - `AWAITING_USERNAME`: Initial connection, awaiting a username.
     - `In_CHAT_LOBBY`: After AWAITING_USERNAME, the client can enter or create a chat room
     - `IN_CHAT_ROOM`: The client is currently in a chat room.
     - Please see for [Available Commands For Each Client State](../protocol.md#available-commands-for-each-client-state).
    
- **Chat Rooms**:
    - The server creates a predefined `MAX_ROOMS` at the start of the process which are re-used. Each room supports up to `MAX_CLIENTS_ROOM` clients.
    - Rooms are managed using the `Room` struct, which includes:
        - A list of connected clients, the room name, and a mutex for safe concurrent access.

### Configurable Scalability

The server's scalability is capped as the server creates all its resource - MAX_ROOM, MAX_CLIENT etc at compile time through MACROS
defined in server_config.h. These MACROS can be easily changed to accommodate a different load.

## [Tests](./test/README.md)
## [Performance Test Results](./performance_results.md)
