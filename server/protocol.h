#ifndef PROTOCOL_H
#define PROTOCOL_H

// Message format: <1-byte-command><space><content>\r\n 

// Client to Server Commands
#define CMD_EXIT                    0x01
#define CMD_USERNAME_SUBMIT         0x02  // Client submitting their username
#define CMD_ROOM_CREATE_REQUEST     0x03  // Client requesting to create a room
#define CMD_ROOM_LIST_REQUEST       0x04  // Client requesting list of rooms
#define CMD_ROOM_JOIN_REQUEST       0x05  // Client requesting to join a room
#define CMD_ROOM_MESSAGE_SEND       0x06  // Client sending a message to room




// Server to Client Commands
#define CMD_WELCOME_REQUEST         0x16  // Server requesting username
#define CMD_ROOM_NOTIFY_JOINED      0x17  // Server confirming room entry
#define CMD_ROOM_CREATE_OK          0x18  // Server confirms room creation
#define CMD_ROOM_CREATE_FAIL        0x19  // Server reports room creation failed
#define CMD_ROOM_LIST_RESPONSE      0x1A  // Server sending room list
#define CMD_ROOM_JOIN_OK            0x1B  // Server confirms room join
#define CMD_ROOM_MSG                 0x1C  


// Error Codes
// Server Error Response Codes
#define ERR_USERNAME_MISSING        0x20  
#define ERR_USERNAME_INVALID        0x21  


// Room Operation Errors (0x20-0x2F)
#define ERR_ROOM_NAME_EXISTS        0x23  
#define ERR_ROOM_NAME_INVALID       0x24  
#define ERR_ROOM_CAPACITY_FULL      0x25  
#define ERR_ROOM_NOT_FOUND         0x26 
#define ERR_SERVER_ROOM_FULL       0x27

#define ERR_PROTOCOL_INVALID_STATE_CMD  0x28
#define ERR_PROTOCOL_INVALID_FORMAT  0x29   
#define ERR_MSG_EMPTY_CONTENT       0x2A

#define ERR_SERVER_FULL 0x2B
#define ERR_CONNECTING 0x2C


// Size limits
#define MAX_USERNAME_LEN                    32
#define MAX_PASSWORD_LEN                    32
#define MAX_ROOM_NAME_LEN                   24
#define MAX_MESSAGE_LEN_TO_SERVER           132
#define MAX_MESSAGE_LEN_FROM_SERVER         2700

#define MAX_CONTENT_LEN                     128



#define MSG_TERMINATOR          "\r\n"
#endif 