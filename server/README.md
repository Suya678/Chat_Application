# Chat Server

This is the server component of the Multi Thread Chat Application, handling multiple client connections and message routing.

## Related Documentation

- [Main Project Documentation](../README.md)
- [Client Documentation](../client/README.md)
- [Protocol documentation](../protocol.md)

## Features

- Handles messaging between multiple users
- **Comprehensive logging**:
  - Provides three different log levels with colors using ANSI Escape codes: `LOG_INFO`, `LOG_USER_ERRROR`, `LOG_SERVER_ERROR` and `LOG_CLIENT_DISCONNECT`.

## Running the Server

```bash
cd server
make clean
make LOG=1 #Optional skip the log and just enter: make, if you would not like logging information
./server   #Run this if you already compiled it are in the directory
```

## Architecture

- **Thread Management**:

  - Uses `4` worker threads `MAX_THREADS`, each handling up to `1500` clients concurrently`MAX_CLIENTS_PER_THREAD`.
  - Maximum total clients: `6000` (MAX_CLIENTS)

- **New Client Distribution Process**:
  - Main thread uses round-robin to select next available worker thread
  - Distribution mechanism:
    1. Main thread waits on a semaphore(ensures worker handled previous client)
    2. Main thread writes new client's fd to worker's notification_fd
    3. Worker thread wakes up from epoll wait
    4. Worker processes the epoll events
    5. It finds the notficaiton_fd in one of the events and then- Reads the client fd and does a sem_post to signal to the main thread that it has picked up the client.
    6. The worker then adds the new client fd to its epoll set and sets up the new client connection.

**Worker threads do a sem_post during initialization which allows the main thread to start distributing clients.**

- **Client state Management**:
  - Each client goes through the following states:
    - `AWAITING_USERNAME`: Initial connection, awaiting a username.
    - `In_CHAT_LOBBY`: After AWAITING_USERNAME, the client can enter or create a chat room
    - `IN_CHAT_ROOM`: The client is currently in a chat room.
    - Please see for [Available Commands For Each Client State](../protocol.md#available-commands-for-each-client-state).
- **Chat Rooms**:
  - The server creates a `50` `MAX_ROOMS` array at the start of the process which are re-used. Each room supports up to `120` `MAX_CLIENTS_ROOM` clients.
  - Rooms are managed using the `Room` struct, which includes:
    - A list of connected clients, the room name, and a mutex to avoid race conditions.

### Configurable Scalability

The server's scalability is capped as the server creates all its resource - MAX_ROOM, MAX_CLIENTS_PER_ROOM, MAX_THREADS, MAX_CLIENTS_PER_THREAD at compile time through MACROS
defined in server_config.h. These MACROS can be easily changed to accommodate a different load.

## [Tests](./test/README.md)

## [Performance Test Results](./performance_results.md)
