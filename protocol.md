# Protocol Specification for Chat Application

All messages exchanged between the client and server must follow this format:

COMMAND [options]

- **`[]`** indicates optional parameters
- Commands would be defined as MACROS in a protcol.h file

## Client to Server:

- **Login:**\
    `LOGIN <USERNAME> <PASSWORD>`

- **Creating an Account:**\
    `CREATE_ACCOUNT <USERNAME> <PASSWORD>`

- **Creating a Room without Password Protection:**\
    `CREATE_ROOM <room_name>`

- **Creating a Room with Password Protection:** \
    `CREATE_ROOM_PASSWORD <room_name> <password>`

- **List Available Rooms:**\
    `LIST`

- **Joining a Room:**\
    `JOIN_ROOM [Password]`

---

## Server to Client:

- **Login Success:**\
    `LOGIN_SUCCESS`

- **Login Failure:**\
    `LOGIN_FAILURE <REASON>`

- **Account Creation Success:**\
    `ACCOUNT_CREATED`

- **Account Creation Failure:**\
    `ACCOUNT_CREATION_FAILED <REASON>`

- **Room Creation Success:**\
    `ROOM_CREATED <room_name>`

- **Room Creation Failure:**\
    `ROOM_CREATION_FAILED <room_name> <REASON>`

- **Room List:**\
    `ROOM_LIST <room_0> <room_1> ...`

- **Join Room Success:**\
    `JOIN_ROOM_SUCCESS <room_name>`

- **Join Room Failure:**\
    `JOIN_ROOM_FAILED <room_name> <REASON>`

- **Message Received in Room:**\
    `ROOM_MESSAGE <sender_username> <message>`

