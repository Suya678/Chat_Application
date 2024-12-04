# Chat Server Test Documentation

This document outlines the test cases implemented for the Chat Server application. The tests were
implemented using a java client sending messages to the server and checking the responses with the
protocol defined in protocol.h in the root folder of the repo.

## Test Environment

* Operating System : Ubuntu Noble/Jammy
* Network :  Both the server and the client were over local host

## Prerequisites

* Makefile and Java compiler. Comes with the Junit Jar in th directory

## To run

* Just type "make" in Shell.

![Demo](demo.gif)

## System Constraints

* Maximum Clients: 2000
* Maximum Rooms: 50
* Maximum Username Length: 32 characters
* Maximum Room Name Length: 24 characters
* Maximum Clients per Room: 40
* Maximum Content Length: 128 characters

# Test Cases

## Connection Tests

| Test Name                             | Purpose                                                                                                          | Expected Behavior                                                                | Actual Result                                           |
|---------------------------------------|------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------|---------------------------------------------------------|
| `testBasicConnection`                 | Verifies basic client-server connection                                                                          | Server should respond with welcome message containing "WELCOME TO THE SERVER"    | ✓ Welcome message received correctly                    |
| `testMaxClientsConnection`            | Tests server's ability to handle maximum number of clients connecting to it                                      | Server should accept all connections up to MAX_CLIENTS and send welcome messages | ✓ All connections accepted with proper welcome messages |
| `testRejectTooManyClients`            | Verifies server behavior when exceeding max clients                                                              | Server should reject connections beyond MAX_CLIENTS with error message           | ✓ Connection rejected with "capacity full" message      |
| `testFreeUpConnectionAfterDisconnect` | Checks if server properly handles client disconnection. If a client leaves, their resources should be cleaned up | Server should allow new connection after an existing client disconnects          | ✓ New connection accepted after disconnect              |

## Test setting up user

| Test Name                                | Purpose                                             | Expected Behavior                                                    | Actual Result                                       |
|------------------------------------------|-----------------------------------------------------|----------------------------------------------------------------------|-----------------------------------------------------|
| `testRejectEmptyUsername`                | Validates username submission constraints           | Server should reject empty usernames with "Content is Empty" message | ✓ Empty username rejected properly                  |
| `testRejectLongUsername`                 | Tests username length validation                    | Server should reject usernames longer than MAX_USER_NAME_LENGTH      | ✓ Long username rejected with an appopriate message |
| `testAcceptValidUsernamesFromMaxClients` | Verifies username registration from Maximum Clients | Server should accept valid usernames from maximum number of clients  | ✓ Valid usernames accepted from all clients         |

## Test Room creation

| Test Name                              | Purpose                                                   | Expected Behavior                                                      | Actual Result                                |
|----------------------------------------|-----------------------------------------------------------|------------------------------------------------------------------------|----------------------------------------------|
| `testCreateValidRoom`                  | Tests basic room creation functionality for a single user | Server should create room and confirm with success message             | ✓ Room created successfully                  |
| `testRejectLongRoomName`               | Validates room name length constraints                    | Server should reject room names longer than MAX_ROOM_LENGTH characters | ✓ Long room name rejected with error message |
| `testCreateMaxRooms`                   | Tests server's ability to handle maximum rooms            | Server should allow creation of MAX_ROOMS (50) rooms                   | ✓ All rooms created successfully             |
| `testRejectRoomCreationWhenAtCapacity` | Verifies room limit enforcement                           | Server should reject room creation when at maximum capacity            | ✓ Room creation rejected when at capacity    |
| `testProperListingOfCreatedRooms`      | Validates room listing functionality                      | Server should properly list all created rooms                          | ✓ All rooms listed correctly                 |
| `testMaxUsersShouldBeAbleToJoinARoom`  | Tests room joining capacity                               | Server should allow MAX_CLIENTS_PER_ROOM users to join                 | Maximum users successfully joined            |

## Test Room Management

| Test Name                                | Purpose                                                                                         | Expected Behavior                                                                                                             | Actual Result                                                                                            |
|------------------------------------------|-------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------|
| `testuserCanLeaveRoom`                   | Tests if a user is able to leave a room, back to the lobby and enter another room               | User should be able to exit the room and then either create a new room or join a new room or re enter the room that they left | ✓ User was able to leave the room and perform all of the actions outline in expected                     |
| `testUserJoinIsBroadcasted`              | Tests when a user joins if other members are notifited                                          | Room members should get a 'name: joined...' whenever a new user joins the room                                                | ✓ Previous room members were informed via a message that a new user has joined                           |
| `testRoompersistsAfterCreatorLeaves`     | Checks if the room is  not falsely cleaned up after the creater leaves with other members in it | User leaves the room with other memebres and the room persists                                                                | ✓ User was able to leave the room and the room persisted as other memebrs could chat after the user left |
| `testMaxUsersShouldBeAbleToJChatInARoom` | Validates chat functionality with maximum users                                                 | All users should pre-defined messages in a full room                                                                          | Messages delivered to all users                                                                          |

## Template

| Test Name              | Purpose | Expected Behavior | Actual Result |
|------------------------|---------|-------------------|---------------|
| `testuserCanLeaveRoom` |         |                   | ✓             |
| `testuserCanLeaveRoom` |         |                   | ✓             |
| `testuserCanLeaveRoom` |         |                   | ✓             |
| `testuserCanLeaveRoom` |         |                   | ✓             |
| `testuserCanLeaveRoom` |         |                   | ✓             |
| `testuserCanLeaveRoom` |         |                   | ✓             |
| `testuserCanLeaveRoom` |         |                   | ✓             |







