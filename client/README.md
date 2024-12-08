# Quantum Chatroom Client Test Documentation

This document outlines the test cases implemented for the Chat Client application. The tests were
implemented using integration testing by ensuring the client could connect to the server, and once connected attempting various scenarios to ensure the client performs as expected. 

## Related Documentation
- [Server Documentation](../server/README.md)
- [Main Project Documentation](../README.md)
- [Protocol documentation](../protocol.md)

## Prerequisites
* Running the client on Visual Studio Code will have a much cleaner look for the user interface.
  
## To run
* Start server by typing ./server
* After the server starts, on a new terminal start the client by running make and then resizing the terminal to the full size of the screen then continue by typing ./client <server_ip> <port_number>(ex: ./client localhost 30000)
**example**
```bash
*example*

cd client
make clean
make
./client 10.65.255.63 30000  #Run this with your local ip if you already compiled it are in the directory
```
* Optionally if you wish to test the chatroom messages between users, start a new terminal and execute the client again, create a chatroom using /create <name> then join on the other client using /join <room#(eg.0)> 

## To keep in mind

* Two or more clients running on the same monitor window cause issues with the output displayed. 
* Any resizing of the windows once execution has began will definitely cause issues and potential crashes.
* Any use of the scroll wheel on the mouse will not work.

## Server Configuration
* Maximum Rooms: 50
* Maximum Username Length: 31 characters
* Maximum Room Name Length: 24 characters
* Maximum Clients per Room: 120
* Maximum Content Length: 128 characters


# Test Cases

## Test Environment
* Operating System : Ubuntu
  *Version ="22.04.5 LTS (Jammy Jellyfish)"
* Network :  Both the server and the client were over local host

## Connection and Username Tests

| Test Name                  | Purpose                                                                        | Expected Behavior                                                             | Actual Result |
|----------------------------|--------------------------------------------------------------------------------|-------------------------------------------------------------------------------|---------------|
| `testBasicConnection`      | Verifies basic client-server connection                                        | Server should respond with welcome message containing "WELCOME TO THE SERVER" | ✓             |
| `testEmptyUserNameEntered` | Tests clients ability to reject sending empty usernames                        | Client should display descriptive error message                               | ✓             |
| `testTooLongofAUserName`   | Tests clients ability to reject sending usernames that exceed allowable length | Client should display descriptive error message                               | ✓             |
| `testSpacesInUsername`     | Tests clients ability to reject sending usernames that include spaces          | Client should display descriptive error message                               | ✓             |
| `testUsernameCorrect`      | Tests clients ability to to accept valid usernames                             | Client should display welcoming message that includes the username entered    | ✓             |

## Test incorrect commands entered by user

| Test Name                             | Purpose                                                     | Expected Behavior                                      | Actual Result                                                                                                                  |
|---------------------------------------|-------------------------------------------------------------|--------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------|
| `testIncorrectCommandEntered`         | Ensures client does not send garbage commands to the server | Client should display error message and refuse to send | ✓                                                                                                                              |
| `testCorrectCommandWithIncorrectArgC` | Ensure client does not accept extra arguments               | Client should display error message and refuse to send | X Extra arguments are ignored and command with proper number of arguments in sent (eg. /join 0 31241 will simply send /join 0) |

## Test Chat Room Messaging

| Test Name               | Purpose                                          | Expected Behavior                                                                                                                        | Actual Result |
|-------------------------|--------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------|---------------|
| `testMessageSentToRoom` | Tests client sends chat message to server        | Message should display on other clients machines as well as your own                                                                     | ✓             |
| `testCommandSentToRoom` | Tests that commands in chatroom do nothing       | Client should display appropriate message as some commands are accepted in chatroom but should not send the command you type to the room | ✓             |
| `testChatSynchronicity` | Tests clients ability to prevent race conditions | Multiple messages should all arrive as well as not change any other client                                                               | X The display properly shows multiple messages but a message recieved changes another clients cursor to the message window            |



## Test Room Commands

| Test Name              | Purpose                                                                           | Expected Behavior                                                                                                             | Actual Result |
|------------------------|-----------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------|---------------|
| `testuserCanLeaveRoom` | Tests if a user is able to leave a room, back to the lobby and enter another room | User should be able to exit the room and then either create a new room or join a new room or re enter the room that they left | ✓             |                                                                  |




