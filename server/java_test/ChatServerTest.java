import org.junit.Test;

import java.io.*;
import java.net.Socket;
import java.util.*;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

/**
 * Tests for the Chat Server implementation.
 * This class contains tests to verify the functionality of a chat server.
 */
public class ChatServerTest {
    // Size Limits
    public static final int MAX_CLIENTS = 2000;
    public static final int MAX_ROOMS = 50;
    public static final int MAX_USERNAME_LEN = 32;
    public static final int MAX_ROOM_NAME_LEN = 24;
    public static final int MAX_CLIENTS_PER_ROOM = 40;
    public static final int MAX_CONTENT_LENGTH = 128;

    // Command codes
    public static final char CMD_USERNAME_SUBMIT = 0x02;
    public static final char CMD_ROOM_CREATE_REQUEST = 0x03;
    public static final char CMD_WELCOME_REQUEST = 0x16;
    public static final char CMD_ROOM_LIST_RESPONSE = 0x1a;
    public static final char CMD_ROOM_CREATE_OK = 0x18;
    public static final char CMD_ROOM_JOIN_REQUEST = 0x05;
    public static final char CMD_ROOM_JOIN_OK = 0x1B;
    public static final char CMD_ROOM_MSG = 0x1c;
    public static final char CMD_ROOM_MESSAGE_SEND = 0x07;
    public static final char CMD_LEAVE_ROOM = 0x06;
    public static final char CMD_ROOM_LEAVE_OK = 0x1D;
    public static final char CMD_ROOM_LIST_REQUEST = 0x04;

    // Error codes
    public static final char ERR_SERVER_FULL = 0x2b;
    public static final char ERR_MSG_EMPTY_CONTENT = 0x2a;
    public static final char ERR_USERNAME_LENGTH = 0x2d;
    public static final char ERR_ROOM_NAME_INVALID = 0x24;
    public static final char ERR_ROOM_CAPACITY_FULL = 0x25;
    public static final char CMD_EXIT = 0x01;

    /**
     * Creates and connects multiple test clients to the server.
     *
     * @param count The number of clients to create and connect
     * @return List of connected Client objects
     * @throws IOException If there is an error establishing connections
     */
    private List<Client> createConnectedClients(int count) throws IOException {
        List<Client> clients = new ArrayList<>();
        for (int i = 0; i < count; i++) {
            Client client = new Client();
            clients.add(client);
        }
        return clients;
    }


    /**
     * Disconnects all clients in the provided list.
     *
     * @param clients List of Client objects to disconnect
     * @throws IOException          If there is an error closing connections
     * @throws InterruptedException If the disconnection process is interrupted
     */
    private void disconnectClients(List<Client> clients) throws InterruptedException, IOException {
        for (Client client : clients) {
            Thread.sleep(50);
            client.close();
        }
    }


    /**
     * Generates a list of random strings for testing purposes.
     *
     * @param count     The number of random strings to generate
     * @param maxLength The maximum length of each generated string
     * @return List of random strings
     */
    private List<String> getRandomStrings(int count, int maxLength) {
        String chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        int charLength = chars.length();
        Random random = new Random();
        List<String> randomStrings = new ArrayList<>();
        StringBuilder string;
        for (int i = 0; i < count; i++) {
            string = new StringBuilder();
            for (int j = 0; j < maxLength; j++) {
                string.append(chars.charAt(random.nextInt(charLength)));
            }
            randomStrings.add(string.toString());
        }
        return randomStrings;

    }

    /**
     * @param username The username for the new client
     * @return List of random strings
     * sends the userName to the server and returns the new Clietn
     * @brief Creates a new client, and waits the for the welcome message from the server,
     */
    private Client setupClientWithUsername(String username) throws IOException {
        Client client = new Client();
        client.getResponse(CMD_WELCOME_REQUEST);
        client.sendMessage(CMD_USERNAME_SUBMIT, username);
        return client;
    }


    /**
     * Tests that a single client can connect to the server and receive
     * the correct welcome message.
     */
    @Test
    public void testBasicConnection() throws IOException, InterruptedException {
        Client client = new Client();
        String welcomeMessage = client.getResponse(CMD_WELCOME_REQUEST);
        assertTrue("Welcome message should contain expected ",
                welcomeMessage.contains("WELCOME TO THE SERVER"));
        client.close();
    }

    /**
     * Tests that the server can handle the maximum number of concurrent client
     * connections and send welcome messages to all clients.
     */
    @Test
    public void testMaxClientsConnection() throws IOException, InterruptedException {
        List<Client> clients = createConnectedClients(MAX_CLIENTS);
        for (Client client : clients) {
            String welcomeMessage = client.getResponse(CMD_WELCOME_REQUEST);
            assertTrue(welcomeMessage.contains("WELCOME TO THE SERVER"));
        }
        disconnectClients(clients);
    }


    /**
     * Tests that the server correctly rejects connection when the server capacity is full
     * by sending a rejection message and closing the connection
     */
    @Test
    public void testRejectTooManyClients() throws IOException, InterruptedException {
        List<Client> clients = createConnectedClients(MAX_CLIENTS);
        List<Client> rejectedClients = createConnectedClients(1);

        for (Client client : rejectedClients) {
            String response = client.getResponse(ERR_SERVER_FULL);
            assertTrue(response.contains("Please"));
        }
        disconnectClients(clients);
        disconnectClients(rejectedClients);

    }

    /**
     * Tests that the server correctly frees up connections that have left the server.
     */
    @Test
    public void testFreeUpConnectionAfterDisconnect() throws IOException, InterruptedException {
        List<Client> clients = createConnectedClients(MAX_CLIENTS);
        clients.get(0).close();
        Client newClient = new Client();

        String welcomeMessage = newClient.getResponse(CMD_WELCOME_REQUEST);
        assertTrue(welcomeMessage.contains("WELCOME TO THE SERVER"));
        newClient.close();
        disconnectClients(clients);

    }

    /**
     * Tests that the server correctly rejects usernames that are empty
     */
    @Test
    public void testRejectEmptyUsername() throws IOException, InterruptedException {
        Client client = new Client();
        client.getResponse(CMD_WELCOME_REQUEST);
        client.sendMessage(CMD_USERNAME_SUBMIT, " ");
        String response = client.getResponse(ERR_MSG_EMPTY_CONTENT);
        assertTrue(response.contains("Content is Empty"));
        client.close();
    }

    /**
     * Tests that the server correctly rejects usernames that are longer than the maximum length in the protocol
     */
    @Test
    public void testRejectLongUsername() throws IOException, InterruptedException {
        Client client = new Client();
        client.getResponse(CMD_WELCOME_REQUEST);

        String longUsername = "a".repeat(MAX_USERNAME_LEN + 1);
        client.sendMessage(CMD_USERNAME_SUBMIT, longUsername);
        String response = client.getResponse(ERR_USERNAME_LENGTH);
        assertTrue(response.contains("User name too long"));
        client.close();

    }

    /**
     * Tests that the server correctly accepts username with a valid length.
     */
    @Test
    public void testAcceptValidUsernamesFromMaxClients() throws IOException, InterruptedException {
        List<Client> clients = new ArrayList<>();

        for (Client client : clients) {
            client.getResponse(CMD_WELCOME_REQUEST);
            client.sendMessage(CMD_USERNAME_SUBMIT, "User");
            String response = client.getResponse(CMD_ROOM_LIST_RESPONSE);
            assertTrue(response.contains("Rooms"));
        }

        disconnectClients(clients);

    }

    /**
     * Tests that the server correctly accepts creation of a room with a room name of valid length
     */
    @Test
    public void testCreateValidRoom() throws IOException, InterruptedException {
        Client client = setupClientWithUsername("a");
        client.sendMessage(CMD_ROOM_CREATE_REQUEST, "ValidRoom");
        String response = client.getResponse(CMD_ROOM_CREATE_OK);
        assertTrue(response.contains("Room"));
        client.close();
    }

    /**
     * Tests that the server rejects room creation when the room name exceeds the maximum allowed length.
     */
    @Test
    public void testRejectLongRoomName() throws IOException, InterruptedException {
        Client client = setupClientWithUsername("a");

        String longRoomName = "a".repeat(MAX_ROOM_NAME_LEN + 1);
        client.sendMessage(CMD_ROOM_CREATE_REQUEST, longRoomName);
        String response = client.getResponse(ERR_ROOM_NAME_INVALID);
        assertTrue(response.contains("Room name length invalid"));
        client.close();


    }


    /**
     * Tests that the server allows creating the maximum number of rooms without errors.
     */
    @Test
    public void testCreateMaxRooms() throws IOException, InterruptedException {
        List<Client> clients = new ArrayList<>();

        for (int i = 0; i < MAX_ROOMS; i++) {
            clients.add(setupClientWithUsername("Random name"));
        }

        for (Client client : clients) {
            client.sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
            String response = client.getResponse(CMD_ROOM_CREATE_OK);
            assertTrue(response.contains("Room created successfully"));
        }
        disconnectClients(clients);

    }


    /**
     * Tests that the server prevents creating new rooms once the maximum room capacity is reached.
     */
    @Test
    public void testRejectRoomCreationWhenAtCapacity() throws IOException, InterruptedException {
        List<Client> initialClients = new ArrayList<>();

        for (int i = 0; i < MAX_ROOMS; i++) {
            initialClients.add(setupClientWithUsername("Random name"));
            initialClients.get(i).sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
            String response = initialClients.get(i).getResponse(CMD_ROOM_CREATE_OK);
            assertTrue(response.contains("Room created successfully"));
        }

        List<Client> overflowClients = new ArrayList<>();

        for (int i = 0; i < MAX_ROOMS; i++) {
            overflowClients.add(setupClientWithUsername("Random name"));
            overflowClients.get(i).sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
            String response = overflowClients.get(i).getResponse(ERR_ROOM_CAPACITY_FULL);
            assertTrue(response.contains("Room creation failed"));
        }


        disconnectClients(initialClients);
        disconnectClients(overflowClients);

    }

    /**
     * Tests that the server returns a correct list of all created rooms.
     */
    @Test
    public void testProperListingOfCreatedRooms() throws IOException, InterruptedException {
        List<Client> roomCreators = new ArrayList<>();

        for (int i = 0; i < MAX_ROOMS; i++) {
            roomCreators.add(setupClientWithUsername("Random name"));
            roomCreators.get(i).sendMessage(CMD_ROOM_CREATE_REQUEST, "Room" + i);
            String response = roomCreators.get(i).getResponse(CMD_ROOM_CREATE_OK);
            assertTrue(response.contains("Room created successfully"));
        }


        Client listChecker = setupClientWithUsername("List checker");

        String response = listChecker.getResponse(CMD_ROOM_LIST_RESPONSE);
        for (int i = 0; i < roomCreators.size(); i++) {
            assertTrue(response.contains("Room" + i));
        }


        listChecker.close();
        disconnectClients(roomCreators);

    }


    /**
     * Tests that the server allows the maximum number of users to join a single room.
     */
    @Test
    public void testMaxUsersShouldBeAbleToJoinARoom() throws IOException, InterruptedException {
        Client roomCreator = setupClientWithUsername("Room Creator");
        roomCreator.sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
        String response = roomCreator.getResponse(CMD_ROOM_CREATE_OK);
        assertTrue(response.contains("Room created successfully"));


        List<Client> joiners = new ArrayList<>();

        for (int i = 0; i < MAX_CLIENTS_PER_ROOM - 1; i++) {
            joiners.add(setupClientWithUsername("Random name"));
            joiners.get(i).sendMessage(CMD_ROOM_JOIN_REQUEST, "0");
            response = joiners.get(i).getResponse(CMD_ROOM_JOIN_OK);
            assertTrue(response.contains("joined"));

        }


        roomCreator.close();
        disconnectClients(joiners);

    }

    /**
     * Tests that the server returns a correct list of all created rooms.
     */
    @Test
    public void testAllClientsReceiveMessages() throws IOException, InterruptedException {
        List<String> messages = getRandomStrings(30, MAX_CONTENT_LENGTH - 1);
        Client roomCreator = setupClientWithUsername("Room Creator");
        roomCreator.sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
        String response = roomCreator.getResponse(CMD_ROOM_CREATE_OK);
        assertTrue(response.contains("Room created successfully"));
        List<Client> joiners = new ArrayList<>();


        for (int i = 0; i < MAX_CLIENTS_PER_ROOM - 1; i++) {
            joiners.add(setupClientWithUsername("Random name"));
            joiners.get(i).sendMessage(CMD_ROOM_JOIN_REQUEST, "0");
            response = joiners.get(i).getResponse(CMD_ROOM_JOIN_OK);
            assertTrue(response.contains("joined"));
        }

        for (String message : messages) {
            roomCreator.sendMessage(CMD_ROOM_MESSAGE_SEND, message);
        }

        // Checking if the joiners recieved all the messages that room creator sent
        for (Client client : joiners) {
            List<String> messagesCopy = new ArrayList<>(messages);
            while (!messagesCopy.isEmpty()) {
                response = client.getResponse(CMD_ROOM_MSG);
                messagesCopy.removeIf(response::contains);
            }
            assertTrue(true);

        }

        disconnectClients(joiners);
        roomCreator.close();


    }

    /**
     * Tests that users can leave a room and receive an appropriate confirmation from the server.
     */
    @Test
    public void testUserSCanLeaveRoom() throws IOException, InterruptedException {

        List<Client> clients = new ArrayList<>();

        for (int i = 0; i < MAX_ROOMS; i++) {
            clients.add(setupClientWithUsername("Random name"));
            clients.get(i).sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
            String response = clients.get(i).getResponse(CMD_ROOM_CREATE_OK);
            assertTrue(response.contains("Room created successfully"));
        }

        for (Client client : clients) {
            client.sendMessage(CMD_LEAVE_ROOM, "dummy");
            String response = client.getResponse(CMD_ROOM_LEAVE_OK);
            assertTrue(response.contains("left the room"));

        }
        disconnectClients(clients);

    }

    /**
     * Tests that a room remains active even after its creator leaves.
     */
    @Test
    public void testRoomPersistsAfterUserLeaves() throws IOException, InterruptedException {
        List<Client> clients = new ArrayList<>();
        clients.add(setupClientWithUsername("roomCreator"));
        clients.get(0).getResponse(CMD_ROOM_LIST_RESPONSE);
        clients.get(0).sendMessage(CMD_ROOM_CREATE_REQUEST, "Dummy Room");
        String response = clients.get(0).getResponse(CMD_ROOM_CREATE_OK);
        assertTrue(response.contains("Room created successfully"));

        for (int i = 1; i < MAX_CLIENTS_PER_ROOM; i++) {
            clients.add(setupClientWithUsername("Random name"));
            clients.get(i).sendMessage(CMD_ROOM_JOIN_REQUEST, "0");
            response = clients.get(i).getResponse(CMD_ROOM_JOIN_OK);
            assertTrue(response.contains("joined"));
        }

        clients.get(0).sendMessage(CMD_LEAVE_ROOM, "dummy");
        response = clients.get(0).getResponse(CMD_ROOM_LEAVE_OK);
        assertTrue(response.contains("left"));

        clients.get(0).sendMessage(CMD_ROOM_LIST_REQUEST, "dummy");
        response = clients.get(0).getResponse(CMD_ROOM_LIST_RESPONSE);
        assertTrue(response.contains("Dummy Room"));


        disconnectClients(clients);


    }

    /**
     * Tests that users can create new rooms after leaving a previously joined room.
     */
    @Test
    public void testUsersCanCreateRoomAfterLeaving() throws IOException, InterruptedException {

        List<Client> clients = new ArrayList<>();

        for (int i = 0; i < MAX_ROOMS; i++) {
            clients.add(setupClientWithUsername("Random name"));
            clients.get(i).sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
            String response = clients.get(i).getResponse(CMD_ROOM_CREATE_OK);
            assertTrue(response.contains("Room created successfully"));
        }

        for (Client client : clients) {
            client.sendMessage(CMD_LEAVE_ROOM, "dummy");
            String response = client.getResponse(CMD_ROOM_LEAVE_OK);
            assertTrue(response.contains("left the room"));
            client.sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
            response = client.getResponse(CMD_ROOM_CREATE_OK);
            assertTrue(response.contains("Room created successfully"));
        }


        disconnectClients(clients);

    }

    /**
     * Tests that users can rejoin the same room after leaving it.
     */

    @Test
    public void testUsersCanJoinSameRoomAfterLeaving() throws IOException, InterruptedException {

        List<Client> roomCreators = new ArrayList<>();
        List<Client> roomJoiners = new ArrayList<>();


        for (int i = 0; i < MAX_ROOMS; i++) {
            roomCreators.add(setupClientWithUsername("Random name"));
            roomCreators.get(i).sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
            String response = roomCreators.get(i).getResponse(CMD_ROOM_CREATE_OK);
            assertTrue(response.contains("Room created successfully"));
        }

        for (int i = 0; i < MAX_ROOMS; i++) {
            roomJoiners.add(setupClientWithUsername("Random name"));
            roomJoiners.get(i).sendMessage(CMD_ROOM_JOIN_REQUEST, String.valueOf(i));
            String response = roomJoiners.get(i).getResponse(CMD_ROOM_JOIN_OK);
            assertTrue(response.contains("joined"));
        }

        for (Client client : roomCreators) {
            client.sendMessage(CMD_LEAVE_ROOM, "dummy");
            String response = client.getResponse(CMD_ROOM_LEAVE_OK);
            assertTrue(response.contains("left the room"));
        }

        for (int i = 0; i < MAX_ROOMS; i++) {
            roomCreators.get(i).sendMessage(CMD_ROOM_JOIN_REQUEST, String.valueOf(i));
            String response = roomCreators.get(i).getResponse(CMD_ROOM_JOIN_OK);
            assertTrue(response.contains("joined"));
        }

        disconnectClients(roomCreators);
        disconnectClients(roomJoiners);

    }

    /**
     * Tests the server properly closes the connection when a client sends the exit command.
     */
    @Test
    public void testIfServerClosesConnectionOnExitCommand() throws IOException, InterruptedException {
        Client client = new Client();
        client.getResponse(CMD_WELCOME_REQUEST);
        client.sendMessage(CMD_EXIT, "d");
        if (client.socket.getInputStream().read() == -1) {
            assert (true);
        } else {
            assert (false);
        }

        client.close();
    }

    /**
     * Tests that all clients in all rooms receive messages from their respective room creators
     */
    @Test
    public void testBroadcastMessagesToAllRoomsAndClients() throws IOException, InterruptedException {
        List<Client> allRoomCreators = new ArrayList<>();
        List<Client> allJoiners = new ArrayList<>();

        for (int i = 0; i < MAX_ROOMS; i++) {
            Client roomCreator = setupClientWithUsername("room creator");

            allRoomCreators.add(roomCreator);
            roomCreator.sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
            assertTrue(roomCreator.getResponse(CMD_ROOM_CREATE_OK).contains("Room created successfully"));
            List<Client> joiners = createConnectedClients(MAX_CLIENTS_PER_ROOM - 1);

            for (Client client : joiners) {
                allJoiners.add(client);
                client.sendMessage(CMD_USERNAME_SUBMIT, "Joiner");
                client.sendMessage(CMD_ROOM_JOIN_REQUEST, String.valueOf(i));
                String response = client.getResponse(CMD_ROOM_JOIN_OK);
                assertTrue(response.contains("joined"));
            }


        }
        disconnectClients(allRoomCreators);
        disconnectClients(allJoiners);


    }


    public class Client {
        // Server Config
        private static final String HOST = "localhost";
        private static final int PORT = 30000;
        private final Map<Character, String> message = new HashMap<>();
        private final StringBuilder messageBuffer = new StringBuilder();
        private final Socket socket;
        private final BufferedWriter writer;
        private final InputStream in;

        /**
         * @brief Initializes the client by connecting to the server and setting up writing and reading streams.
         * <p>
         * Establishes a connection to the server at the specified host and port.
         */
        public Client() throws IOException {
            socket = new Socket(HOST, PORT);
            writer = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
            in = socket.getInputStream();

        }

        /**
         * @param expectedCmd The command type to wait for in the server's response.
         * @return The content portion of the server's response associated with the expected command.
         * @brief Retrieves the server's response to a previously sent command.
         * <p>
         * This method reads data from the server socket until the expected command type is found in the response.
         * It processes incoming data, extracts messages based on the expected command, and returns the associated content.
         */
        public String getResponse(char expectedCmd) throws IOException, IndexOutOfBoundsException {
            try {
                while (!message.containsKey(expectedCmd)) {

                    int temp = in.read();

                    if (temp == -1) {
                        throw new IOException("Connection closed");
                    }

                    messageBuffer.append((char) temp);

                    while (messageBuffer.toString().contains("\r\n")) {
                        if (messageBuffer.length() == 2) {
                            messageBuffer.setLength(0);
                            continue;
                        }
                        char cmdType = messageBuffer.charAt(0);

                        String content = messageBuffer.substring(2, messageBuffer.indexOf("\r\n"));
                        message.put(cmdType, content);
                        messageBuffer.setLength(0);
                    }
                }
            } catch (IndexOutOfBoundsException e) {
                fail(e.getMessage() + " " + (int) messageBuffer.charAt(0) + " " + (int) messageBuffer.charAt(1));
            }
            return message.remove(expectedCmd);

        }

        /**
         * @param cmdType Command of the message
         * @param content Content portion of the message
         * @brief Sends the given content with the cmdtype to the server socket
         */
        public void sendMessage(char cmdType, String content) throws IOException {
            writer.write(cmdType + " " + content + "\r\n");
            writer.flush();
        }

        /**
         * @brief Closes the socket. Introduces a small delay between rapid sleep calls
         */
        public void close() throws IOException, InterruptedException {
            if (socket != null && !socket.isClosed()) {
                socket.close();
            }
        }
    }
}



