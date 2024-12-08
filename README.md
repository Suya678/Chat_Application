

# Multi Thread Chat Application

A real-time chat application supporting multiple concurrent users through multi-threading. The application consists of a 
server component handling multiple client connections and a client interface for users to communicate.

**Note**: Both client and server are configured to work over localhost in the repo. If you wish, this can be easily changed through the macros in the implementation files.

## Technologies Used
- **C Programming Language**: Server and client 
- **Linux**: Platform for development and execution. The server uses some Linux specific system calls - accept4()
- **ncurses**: For the client UI.
- **Java JUnit Framework**: For testing the server.

## Features

### Server Architecture
- Multithreaded design:
  - A main thread accepts new connections and delegates them to worker threads
  - Worker threads manage clients using `epoll'
  - All sockets are non blocking
  - Please see [Server Documentation](server/README.md) for more info
  - [Server Test Documentation](server/README.md) for server test documentation

### Client Architecture
- Implemented in Ncurses
- Client functionality
  - Join or create chat rooms
  - Communicate with others in the room
  - Leave rooms to return to the lobby and join or create new rooms
  - Please see [Client Documentation](client/README.md) for more info.
  - [Client Test Documentation](client/README.md)for server test documentation


## Performance Results
We conducted a  test to evaluate whether the server benefits from multithreading. While this test was not exhaustive, the results showed some improvement in performance as the number of threads increased.
For more details, please refer to [Performance Test Results](server/performance_results.md)


## Project Structure
```
chat-application/
├── server/         # Server-side implementation
    ├── test/         # Server-Side tests
├── client/         # Client-side implementation
├── protocol.md      # Defines the simple protocol that the server and client uses to communicate
```

## To Start
1. Start the server:
```bash
cd server
make clean
make #Optional make LOG=1  , if you would like to see logging information
./server
```

2. Launch the client:
```bash
cd client
make clean
make
./client
```
