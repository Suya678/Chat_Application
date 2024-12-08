# Protocol Specification for Chat Application
# Protocol Documentation

This document describes the protocol used for communication between the client and server in the  chat application. 



## Message Format

- **Structure**: `<1-byte-command><space><content>` followed by a message terminator (`\r\n`).
- **Rules**:
    - Content cannot be empty.
    - For commands like `/leave` or `/exit`, dummy content should be included and will be ignored.
  
-**Message not formated as described here may cause buffer overflow**

---

## Commands

### Client-to-Server Commands

| Command Name               | Code   | Description                                 |
|----------------------------|--------|---------------------------------------------|
| `CMD_EXIT`                 | `0x01` | Disconnect from the server.                 |
| `CMD_USERNAME_SUBMIT`      | `0x02` | Submit username to the server.              |
| `CMD_ROOM_CREATE_REQUEST`  | `0x03` | Request to create a new chat room.          |
| `CMD_ROOM_LIST_REQUEST`    | `0x04` | Request a list of available chat rooms.     |
| `CMD_ROOM_JOIN_REQUEST`    | `0x05` | Request to join a specified chat room.      |
| `CMD_LEAVE_ROOM`           | `0x06` | Leave the current chat room.                |
| `CMD_ROOM_MESSAGE_SEND`    | `0x07` | Send a message to the current chat room.    |

### Server-to-Client Commands

| Command Name             | Code   | Description                                   |
|--------------------------|--------|-----------------------------------------------|
| `CMD_WELCOME_REQUEST`    | `0x16` | Request username from the client.             |
| `CMD_ROOM_NOTIFY_JOINED` | `0x17` | Confirm client has joined a room.             |
| `CMD_ROOM_CREATE_OK`     | `0x18` | Confirm room creation was successful.         |
| `CMD_ROOM_LIST_RESPONSE` | `0x1A` | Send a list of available rooms to the client. |
| `CMD_ROOM_JOIN_OK`       | `0x1B` | Confirm client has joined the requested room. |
| `CMD_ROOM_MSG`           | `0x1C` | Broadcast a message to all room members.      |
| `CMD_ROOM_LEAVE_OK`      | `0x1D` | Confirm client has left the room.             |

---

## Error Codes

| Error Name                       | Code   | Description                                            |
|----------------------------------|--------|--------------------------------------------------------|
| `ERR_ROOM_NAME_INVALID`          | `0x24` | Room name exceeds maximum length.                      |
| `ERR_ROOM_CAPACITY_FULL`         | `0x25` | Room is at maximum capacity.                           |
| `ERR_ROOM_NOT_FOUND`             | `0x26` | Specified room does not exist or is inactive.          |
| `ERR_PROTOCOL_INVALID_STATE_CMD` | `0x28` | Command not valid in the client’s current state.       |
| `ERR_PROTOCOL_INVALID_FORMAT`    | `0x29` | Message does not conform to the format described here. |
| `ERR_MSG_EMPTY_CONTENT`          | `0x2A` | Message content is empty.                              |
| `ERR_SERVER_FULL`                | `0x2B` | Server has reached maximum client capacity.            |
| `ERR_CONNECTING`                 | `0x2C` | Error during client handoff to worker thread.          |
| `ERR_USERNAME_LENGTH`            | `0x2D` | Username exceeds maximum length.                       |

---

## Size Limits

|                               | Limit      | Description                                                   |
|-------------------------------|------------|---------------------------------------------------------------|
| `MAX_USERNAME_LEN`            | `32`       | Maximum username length.                                      |
| `MAX_ROOM_NAME_LEN`           | `24`       | Maximum room name length.                                     |
| `MAX_MESSAGE_LEN_TO_SERVER`   | `132`      | Maximum message length from client to server.                 |
| `MAX_MESSAGE_LEN_FROM_SERVER` | `variable` | Maximum message length from server, accommodating room lists. |
| `MAX_CONTENT_LEN`             | `128`      | Maximum content size in a message from the client.                            |


---
## Available Commands For Each Client State

| State                                                      | Available Commands                                                                |   
|------------------------------------------------------------|-----------------------------------------------------------------------------------|
| `Just connected\AWAITING_USERNAME`                         | `CMD_USERNAME_SUBMIT, CMD_EXIT`                                                   |                
| `After successfully submitting the username\IN_CHAT_LOBBY` | `CMD_EXIT, CMD_ROOM_CREATE_REQUEST, CMD_ROOM_LIST_REQUEST, CMD_ROOM_JOIN_REQUEST` |                 
| `After joining a room\IN_CHAT_ROOM`                        | `CMD_EXIT, CMD_ROOM_MESSAGE_SEND, CMD_LEAVE_ROOM`                                 |  

