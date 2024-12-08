# Quantum Chatroom and Documentation Standards

In this document, we outline the coding and documentation standards adopted by the Quantum chatroom development team. Adhering to these standards ensures that our codebase remains clear, correct, and readable, facilitating ease of maintenance, modification, extension, and reuse.


## Design Principles

* Modularity: Each module was broken down into smaller helper function
* Although the Client and Server were developed independently they were ultimately molded by the protocol design document
* Code duplication was reduced by abstracting common functionalities 
* The Chatroom functionalities were kept as simple enough to allow for the core purpose which is to send messages through a server

## Coding Standards

* Programming Language: C and Java for the server test suite
* IDE: Visual Studio Code
* Version Control: Server:GitHub Client:Email(lol)
* Naming Conventions: Snake case
* Code Formatting: Clang Format

## Documentation Standards

* README File: Project title, setup instructions, test cases

## Code Review Process

* In person meetings as well as consistent communication through discord 



## Server Development Major Milestones
| Date        | Development Activity                                                                                                      |
|-------------|---------------------------------------------------------------------------------------------------------------------------|
| Nov 18 - 22 | Research on server architectures                                                                                          |
| Nov 23 - 28 | Implemented basic server setup, epoll mechanism and worker thread handoff                                                 |
| Nov 28,     | Broadcasting system implementation                                                                                        |
| Nov 29 - 30 | Major code refactoring and documentation , Added client join/leave notifications and implemented leave room functionality |
| Nov 30      | Initial test suite implementation in Javascript                                                                           |
| Dec 3       | Enhanced logging system with color coding                                                                                 |
| Dec 4       | Migrated test suite to Java                                                                                               |
| Dec 7       | Finished capturing the performance test results                                                                           |
| Dec 8       | Finalized server configuration (4 worker threads) - 6000 max clients                                                      |
| Dec 8       | Server completed with all the necessary documents                                                                         |

## Client Development Major Milestones
| Date    | Development Activity                                                |
|---------|---------------------------------------------------------------------|
| Nov 19  | Basic client that can connect make a request to a server            |
| Nov 24  | Implemented sending and receiving messages and parsing user command |
| Nov 25  | Implemented multithreading and refined user command parser          |
| Dec 3   | Client can interact with the server component                       |
| Dec 5-6 | Client fully functional, doing final testing and documentation      |
| Dec 7   | Finished function coding and just testing                           |

## Features Not Implemented
Due to time constraints, we focused on completing the core components(sockets, multithreading) of the project and did not get enough time to implement the
authentication feature that was outlined in our initial memo.

