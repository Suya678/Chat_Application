#ifndef PROTOCOL_H
#define PROTOCOL_H

// Message format: <1-byte-command><space><content>\r\n
// Content cannot be empty, so for /leave and /exit command, you can include dummy content that will be ignored
// Content has a max size of 128

// Client to Server Commands
#define CMD_EXIT 0x01
#define CMD_USERNAME_SUBMIT 0x02     // Client submitting their username
#define CMD_ROOM_CREATE_REQUEST 0x03 // Client requesting to create a room
#define CMD_ROOM_LIST_REQUEST 0x04   // Client requesting list of rooms
#define CMD_ROOM_JOIN_REQUEST 0x05   // Client requesting to join a room
#define CMD_LEAVE_ROOM 0x06          // Client requests to leave the room
#define CMD_ROOM_MESSAGE_SEND 0x07   // Client sending a message to room

// Server to Client Commands
#define CMD_WELCOME_REQUEST 0x16    // Server requesting username
#define CMD_ROOM_NOTIFY_JOINED 0x17 // Server confirming room entry
#define CMD_ROOM_CREATE_OK 0x18     // Server confirms room creation
#define CMD_ROOM_LIST_RESPONSE 0x1A // Server sending room list
#define CMD_ROOM_JOIN_OK 0x1B       // Server confirms room join
#define CMD_ROOM_MSG 0x1C           // Server is broadcasting a message in the room
#define CMD_ROOM_LEAVE_OK 0x1D

// Error Codes
#define ERR_ROOM_NAME_INVALID 0x24  // Room name is longer than MAX_ROOM_Name
#define ERR_ROOM_CAPACITY_FULL 0x25 // The room client requested to join is full
#define ERR_ROOM_NOT_FOUND 0x26     // Room number is either not active or invalid

#define ERR_PROTOCOL_INVALID_STATE_CMD                                                                                 \
    0x28 // Invalid command for the client's state,such as requesting to join a room when they are in AWAITING_USERNAME
         // state
#define ERR_PROTOCOL_INVALID_FORMAT 0x29 // THe message is not correctly formated
#define ERR_MSG_EMPTY_CONTENT                                                                                          \
    0x2A //  Every message should not have empty content, can put dummy content for /leave room and exit
#define ERR_SERVER_FULL 0x2B     // The server is currently full
#define ERR_CONNECTING 0x2C      // Something went wrong when trying to hand off the client to a worker thread
#define ERR_USERNAME_LENGTH 0x2D // The user name length is > MAX username length

// Size limits
#define MAX_USERNAME_LEN 32
#define MAX_ROOM_NAME_LEN 24
#define MAX_MESSAGE_LEN_TO_SERVER 132
// Maximum message length from server is calculated to
// accommodate a list of all room names plus some space for formatting
#define MAX_MESSAGE_LEN_FROM_SERVER (MAX_ROOM_NAME_LEN * MAX_ROOMS + 256)
#define MAX_CONTENT_LEN 128

#define MSG_TERMINATOR "\r\n"
#endif
