

# Multi Thread Chat Application

A real-time chat application supporting multiple concurrent users through multi-threading. The application consists of a 
server component handling multiple client connections and a client interface for users to communicate.

**Note**: Both client and server are configured to work over localhost in the repo. If you wish, this can be easily changed through the macros in the implementation files.

## Related Documentation
- [Server Documentation](./server/README.md)
- [Client Documentation](./client/README.md)
- [Protocol documentation](./protocol.md)

## Technologies Used
- **C Programming Language**: Server and client 
- **Linux**: Platform for development and execution. The server uses some Linux specific system calls - accept4()
- **ncurses**: For the client UI.
- **Java JUnit Framework**: For testing the server.

## To Start
**Note: This application requires running multiple terminal instances**

**Note: The server is configured to listen on localhost over port 30000**

**Note: When running the server, please change the linux's default file descriptor limit prior to running the server by enter the following in bash:** 
```bash
ulimit -n 99999
```

1. Open two terminals
2. In one terminal start the server:
```bash
cd server
make clean
make LOG=1 #LOG=1 is enable log messages, it can be omitted
./server
```

3. In the second terminal, Launch the client:
```bash
cd client
make clean
make
./client localhost 30000
```
**Additional Clients would need to follow step 3 in additional terminals**

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
- One thread for handling user input another for recieving server messages
- Client functionality
  - Join or create chat rooms
  - Communicate with others in the room
  - Leave rooms to return to the lobby and join or create new rooms
  - Please see [Client Documentation](client/README.md) for more info.


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
