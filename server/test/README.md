# Chat Server Test Documentation
This document outlines the test cases implemented for the Chat Server application. 

## Implementation
* The test suite is written in Java using the JUnit framework to verify server behaviour.
* All test cases follow this pattern:
  - Connect to the server
  - Send a message defined in the protocol.h
  - Validate server response with expected behaviour

## Test Environment
- **OS**: Ubuntu 22.04.4 LTS Jammy
* Network :  Both the server and the client were connected over localhost

## Prerequisites
* Makefile and Java compiler. Comes with the Junit Jar file needed to run the test
## To replicate
1. Open up two terminals
2. In the server terminal:
* Change the default fd limit:
  ```bash
  ulimit -n 99999
  ```
* (Tests will fail otherwise due to Linux default limit)

3. In the first terminal:
* Navigate to server directory
* Clean and rebuild server with logging enabled:
  ```bash
  make clean
  make LOG=1    
  ./server
  ```

4. Once the server has started, In the second terminal:
* Navigate to test folder
* Build and run tests:
  ```bash 
  make
  ```
**Note: The tests may take upto 5 minutes to finish.**

## System Constraints
* Maximum Clients: 6000
* Maximum Rooms: 50
* Maximum Username Length: 32 characters
* Maximum Room Name Length: 24 characters
* Maximum Clients per Room: 120
* Maximum Content Length: 128 characters
* Worker threads used By The Server: 4

# Test Cases

## Connection Tests

| Test Name                             | Purpose                                                                                                          | Expected Behavior                                                                                                        | Pass                                                    |
|---------------------------------------|------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------|
| `testBasicConnection`                 | Verifies basic client-server connection                                                                          | Server should accept the client connection and send a response with a welcome message containing "WELCOME TO THE SERVER" | ✓                                                       |
| `testMaxClientsConnections`           | Tests server's ability to handle maximum number of clients connecting to it                                      | Server should accept all connections up to MAX_CLIENTS and send welcome messages                                         | ✓                                                       |
| `testRejectClientsWhenAtFullCapacity` | Verifies server behavior when exceeding max clients                                                              | Server should reject connections beyond MAX_CLIENTS with an error message                                                | ✓                                                       |
| `testFreeUpConnectionAfterDisconnect` | Checks if server properly handles client disconnection. If a client leaves, their resources should be cleaned up | Server should allow new connection after an existing client disconnects                                                  | ✓                                                       |

## Test setting up user
| Test Name                                | Purpose                                                                             | Expected Behavior                                                    | Actual Result |
|------------------------------------------|-------------------------------------------------------------------------------------|----------------------------------------------------------------------|---------------|
| `testRejectEmptyUsername`                | Validates username submission constraints                                           | Server should reject empty usernames with "Content is Empty" message | ✓             |
| `testRejectLongUsername`                 | Tests username length validation                                                    | Server should reject usernames longer than MAX_USER_NAME_LENGTH      | ✓             |
| `testAcceptValidUsernamesFromMaxClients` | Verifies the server can correctly handle username registration from Maximum Clients | Server should accept valid usernames from maximum number of clients  | ✓             |

## Test Room creation

| Test Name                              | Purpose                                                   | Expected Behavior                                                       | Actual Result |
|----------------------------------------|-----------------------------------------------------------|-------------------------------------------------------------------------|---------------|
| `testCreateValidRoom`                  | Tests basic room creation functionality for a single user | Server should create room and confirm with a success message response   | ✓             |
| `testRejectLongRoomName`               | Validates room name length constraints                    | Server should reject room names longer than MAX_ROOM_LENGTH characters  | ✓             |
| `testCreateMaxRooms`                   | Tests server's ability to handle MAX_ROOMS creation       | Server should allow creation of MAX_ROOMS  rooms                        | ✓             |
| `testRejectRoomCreationWhenAtCapacity` | Verifies room limit enforcement                           | Server should reject room creation when at maximum Room number capacity | ✓             |
| `testProperListingOfCreatedRooms`      | Validates room listing functionality                      | Server should properly list all created rooms                           | ✓             |

## Test Room Management

| Test Name                               | Purpose                                                                                                                                     | Expected Behavior                                                                                                                                                                                        | Actual Result |
|-----------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------|
| `testMaxUsersShouldBeAbleToJoinARoom`   | Tests that the server is able to handle MAX_CLIENTS_PER_ROM joining a single room                                                           | Server should allow MAX_CLIENTS_PER_ROOM users to join                                                                                                                                                   | ✓             |
| `testusersCanLeaveRoom`                 | Tests multiple users can leave a room and go back to the chat chat lobby                                                                    | Multiple users after creating a room should be able to leave a room and get a response that contains "left the room"                                                                                     | ✓             |
| `testroomIsCleanedUpAfterALlUsersLeave` | Tests that the server correctly cleans up the resources used by a room when all clients leave                                               | After a client creates a room, leaves it and then sends a list room command, the server should respond back with a list that contains "No chat rooms available", indicating that the room was cleaned up | ✓             |
| `testUsersCanCreateRoomAfterLeaving`    | Tests that users after leaving a room can create another room                                                                               | After a client creates a room, leaves it and then sends a create room command, the server successfully completes the request                                                                             | ✓             |
| `testRoomPersistsAfterUserLeaves`       | Checks if the room is not falsely cleaned up after the room creator leaves with other members in it                                         | Room creator leaves the room with other clients in it. When the create sends the list command, server responds with a list which includes the room that the room creator had created                     | ✓             |
| `testUsersCanJoinSameRoomAfterLeaving`  | Tests if the user can join the same room if it still had some clients after leaving it                                                      | After leaving a room, the client should be able to rejoin the room they left with other clients in it.                                                                                                   | ✓             |
| `testAllClientsReceiveMessagesInARoom`  | Tests that a message sent in a room is broadcast to all clients in the room except for the sender. This was tested with MAX_CLIENTS_IN_ROOM | After sending a message in a room, other clients should correctly receive the message send by the client                                                                                                 | ✓             |
| `testRoomJoinMessageToExistingUser`     | Tests when a user joins if other members are notified.                                                                                      | Room members should get a 'name: joined...' whenever a new user joins the room                                                                                                                           | ✓             |
| `testMessageIsolationBetweenRooms`      | Tests that messages in a room are only broadcast to the clients in the same room                                                            | After a client sends a message in a room, clients in the same room should be able to get that message. Clients in other rooms should not get that message.                                               | ✓             |


## EXIT

| Test Name                                   | Purpose                                                                                    | Expected Behavior                                                           | Actual Result |
|---------------------------------------------|--------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------|---------------|
| `testIfServerClosesConnectionOnExitCommand` | Tests that the server correctly closes the connection when the client sends a exit command | Server should close client connection after the client sends a exit command | ✓             |




## Invalid Commands Being Sent

| Test Name                                           | Purpose                                                                                                                                        | Expected Behavior                                                                                                                                                                                     | Actual Result |
|-----------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------|
| `testServerRejectsInvalidCommands`                  | Tests that the server correctly sends an error message when it receives a message with  command not defined in protocol.h                      | When the client sedn the message with the invalid command, the server should respond back with a message that contains "Command not found"                                                            | ✓             |
| `testServerHandlesLeaveRoomFromLobbyWithError`      | Tests that the server correctly sends an error message when a client attempts to leave a room when they are currently in the lobby state       | When the client sends the message with  LEAVE_ROOM command while in the lobby state, the server should respond back with a message that contains "Invalid command for lobby state"                    | ✓             |
| `testServerHandlesSubmitUsernameFromLobbyWithError` | Tests that the server correctly sends an error message when a client attempts to  submit a username when they are currently in the lobby state | When the client sends the message with the username submit command while in the lobby state, the server should respond back with a message that contains "Invalid command for lobby state"            | ✓             |
| `testServerErrorOnJoinRoomWhileAlreadyInRoom`       | Tests that the server correctly sends an error message when a client attempts to join a room when they are currently in a room                 | When the client sends the message with the join room command while in the IN_CHAT_ROOM state, the server should respond back with a message that contains "Invalid command for  in chat room state"   | ✓             |
| `testServerErrorOnCreateRoomWhileAlreadyInRoom`     | Tests that the server correctly sends an error message when a client attempts to create a room when they are currently in a room               | When the client sends the message with the create room command while in the IN_CHAT_ROOM state, the server should respond back with a message that contains "Invalid command for  in chat room state" | ✓             |
| `testServerErrorOnListRoomsWhileInARoom`            | Tests that the server correctly sends an error message when a client attempts to list the rooms in a server when they are currently in a room  | When the client sends the message with the list rooms command while in the IN_CHAT_ROOM state, the server should respond back with a message that contains "Invalid command for  in chat room state"  | ✓             |



