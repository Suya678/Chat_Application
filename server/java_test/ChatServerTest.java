import org.junit.Test;
import org.junit.jupiter.api.Timeout;

import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.util.*;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

public class ChatServerTest {


    // Size Limits
    private static final int MAX_CLIENTS = 2000;
    private static final int MAX_ROOMS = 50;
    private static final int MAX_USERNAME_LEN = 32;
    private static final int MAX_ROOM_NAME_LEN = 24;
    private static final int MAX_CLIENTS_PER_ROOM = 40;
    private static final int MAX_CONTENT_LENGTH = 128;

    // Command codes
    private static final char CMD_USERNAME_SUBMIT = 0x02;
    private static final char CMD_ROOM_CREATE_REQUEST = 0x03;
    private static final char CMD_WELCOME_REQUEST = 0x16;
    private static final char CMD_ROOM_LIST_RESPONSE = 0x1a;
    private static final char CMD_ROOM_CREATE_OK = 0x18;
    private static final char CMD_ROOM_JOIN_REQUEST = 0x05;
    private static final char CMD_ROOM_JOIN_OK = 0x1B;
    private static final char CMD_ROOM_MSG = 0x1c;
    private static final char CMD_ROOM_MESSAGE_SEND = 0x07;


    // Error codes
    private static final char ERR_SERVER_FULL = 0x2b;
    private static final char ERR_MSG_EMPTY_CONTENT = 0x2a;
    private static final char ERR_USERNAME_LENGTH = 0x2d;
    private static final char ERR_ROOM_NAME_INVALID = 0x24;
    private static final char ERR_ROOM_CAPACITY_FULL = 0x25;

    private List<Client> createConnectedClients(int count) throws IOException {
        List<Client> clients = new ArrayList<>();
        for (int i = 0; i < count; i++) {
            Client client = new Client();
            client.connect();
            clients.add(client);
        }
        return clients;
    }

    private void disconnectClients(List<Client> clients) throws IOException, InterruptedException {
        for (Client client : clients) {
            client.close();
        }
    }

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

    @Test
    public void testBasicConnection() throws IOException {
        try (Client client = new Client()) {
            client.connect();
            String welcomeMessage = client.getResponse(CMD_WELCOME_REQUEST);
            assertTrue("Welcome message should contain expected ",
                    welcomeMessage.contains("WELCOME TO THE SERVER"));
        } catch (IOException e) {
            fail(e.getMessage());
        }
    }

    @Test
    public void testMaxClientsConnection() throws IOException, InterruptedException {
        List<Client> clients = createConnectedClients(MAX_CLIENTS);
        try {
            for (Client client : clients) {
                String welcomeMessage = client.getResponse(CMD_WELCOME_REQUEST);
                assertTrue(welcomeMessage.contains("WELCOME TO THE SERVER"));

            }
        } catch (IOException e) {
            fail(e.getMessage());
        } finally {
            disconnectClients(clients);
        }
    }

    @Test
    public void testRejectTooManyClients() throws IOException, InterruptedException {
        List<Client> clients = createConnectedClients(MAX_CLIENTS);
        List<Client> rejectedClients = createConnectedClients(1);
        try {
            for (Client client : rejectedClients) {
                String response = client.getResponse(ERR_SERVER_FULL);
                assertTrue(response.contains("Please"));
            }
        } catch (IOException e) {
            fail(e.getMessage());
        } finally {
            disconnectClients(clients);
            disconnectClients(rejectedClients);

        }
    }

    @Test
    public void testFreeUpConnectionAfterDisconnect() throws IOException, InterruptedException {
        List<Client> clients = createConnectedClients(MAX_CLIENTS);
        try {
            clients.get(0).close();
            Client newClient = new Client();
            newClient.connect();
            String welcomeMessage = newClient.getResponse(CMD_WELCOME_REQUEST);
            assertTrue(welcomeMessage.contains("WELCOME TO THE SERVER"));
            newClient.close();
        } finally {
            disconnectClients(clients);

        }
    }

    @Test
    public void testRejectEmptyUsername() throws IOException {
        try (Client client = new Client()) {
            client.connect();
            client.getResponse(CMD_WELCOME_REQUEST);
            client.sendMessage(CMD_USERNAME_SUBMIT, " ");
            String response = client.getResponse(ERR_MSG_EMPTY_CONTENT);
            assertTrue(response.contains("Content is Empty"));
            client.close();
        }
    }

    @Test
    public void testRejectLongUsername() throws IOException {
        try (Client client = new Client()) {
            client.connect();
            client.getResponse(CMD_WELCOME_REQUEST);

            String longUsername = "a".repeat(MAX_USERNAME_LEN + 1);
            client.sendMessage(CMD_USERNAME_SUBMIT, longUsername);
            String response = client.getResponse(ERR_USERNAME_LENGTH);
            assertTrue(response.contains("User name too long"));
            client.close();
        }
    }

    @Test
    public void testAcceptValidUsernamesFromMaxClients() throws IOException, InterruptedException {
        List<Client> clients = createConnectedClients(MAX_ROOMS);
        try {
            for (Client client : clients) {
                client.getResponse(CMD_WELCOME_REQUEST);
                client.sendMessage(CMD_USERNAME_SUBMIT, "User");
                String response = client.getResponse(CMD_ROOM_LIST_RESPONSE);
                assertTrue(response.contains("Rooms"));
                client.close();
            }
        } finally {
            disconnectClients(clients);
        }
    }

    @Test
    public void testCreateValidRoom() throws IOException {
        try (Client client = new Client()) {
            client.connect();
            client.getResponse(CMD_WELCOME_REQUEST);

            client.sendMessage(CMD_USERNAME_SUBMIT, "ValidUser");
            client.getResponse(CMD_ROOM_LIST_RESPONSE);
            client.sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
            String response = client.getResponse(CMD_ROOM_CREATE_OK);
            assertTrue(response.contains("Room"));
            client.close();
        } catch (IOException e) {
            fail(e.getMessage());
        }

    }

    @Test
    public void testRejectLongRoomName() throws IOException {
        try (Client client = new Client()) {
            client.connect();
            client.getResponse(CMD_WELCOME_REQUEST);
            client.sendMessage(CMD_USERNAME_SUBMIT, "User");
            client.getResponse(CMD_ROOM_LIST_RESPONSE);

            String longRoomName = "a".repeat(MAX_ROOM_NAME_LEN + 1);
            client.sendMessage(CMD_ROOM_CREATE_REQUEST, longRoomName);
            String response = client.getResponse(ERR_ROOM_NAME_INVALID);
            assertTrue(response.contains("Room name length invalid"));
            client.close();
        } catch (IOException e) {
            fail(e.getMessage());
        }
    }

    @Test
    public void testCreateMaxRooms() throws IOException, InterruptedException {
        List<Client> clients = createConnectedClients(MAX_ROOMS);
        try {
            for (Client client : clients) {
                client.getResponse(CMD_WELCOME_REQUEST);
                client.sendMessage(CMD_USERNAME_SUBMIT, "User");
                client.getResponse(CMD_ROOM_LIST_RESPONSE);
                client.sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
                String response = client.getResponse(CMD_ROOM_CREATE_OK);
                assertTrue(response.contains("Room created successfully"));
                client.close();
            }
        } finally {
            disconnectClients(clients);
        }
    }

    @Test
    public void testRejectRoomCreationWhenAtCapacity() throws IOException, InterruptedException {
        List<Client> initialClients = createConnectedClients(MAX_ROOMS);
        List<Client> overflowClients = new ArrayList<>();
        try {
            for (Client client : initialClients) {
                client.getResponse(CMD_WELCOME_REQUEST);
                client.sendMessage(CMD_USERNAME_SUBMIT, "User");
                client.getResponse(CMD_ROOM_LIST_RESPONSE);
                client.sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
                client.getResponse(CMD_ROOM_CREATE_OK);
            }

            overflowClients = createConnectedClients(MAX_ROOMS);
            for (Client client : overflowClients) {
                client.getResponse(CMD_WELCOME_REQUEST);
                client.sendMessage(CMD_USERNAME_SUBMIT, "User");
                client.getResponse(CMD_ROOM_LIST_RESPONSE);
                client.sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
                String response = client.getResponse(ERR_ROOM_CAPACITY_FULL);
                assertTrue(response.contains("Room creation failed"));
            }
        } finally {
            disconnectClients(initialClients);
            disconnectClients(overflowClients);
        }
    }

    @Test
    public void testProperListingOfCreatedRooms() throws IOException, InterruptedException {
        List<Client> roomCreators = createConnectedClients(MAX_ROOMS);
        Client listChecker = new Client();
        try {
            for (int i = 0; i < roomCreators.size(); i++) {


                roomCreators.get(i).getResponse(CMD_WELCOME_REQUEST);
                roomCreators.get(i).sendMessage(CMD_USERNAME_SUBMIT, "User");
                roomCreators.get(i).getResponse(CMD_ROOM_LIST_RESPONSE);
                roomCreators.get(i).sendMessage(CMD_ROOM_CREATE_REQUEST, "Room" + i);
                String response = roomCreators.get(i).getResponse(CMD_ROOM_CREATE_OK);
                assertTrue(response.contains("Room created successfully"));
            }
            listChecker.connect();
            listChecker.getResponse(CMD_WELCOME_REQUEST);
            listChecker.sendMessage(CMD_USERNAME_SUBMIT, "User");
            String response = listChecker.getResponse(CMD_ROOM_LIST_RESPONSE);
            for (int i = 0; i < roomCreators.size(); i++) {
                assertTrue(response.contains("Room" + i));
            }
        } finally {
            listChecker.close();
            disconnectClients(roomCreators);
        }
    }

    @Test
    public void testMaxUsersShouldBeAbleToJoinARoom() throws IOException, InterruptedException {
        Client roomCreator = new Client();
        List<Client> joiners = createConnectedClients(MAX_CLIENTS_PER_ROOM - 1);

        try {
            roomCreator.connect();
            roomCreator.getResponse(CMD_WELCOME_REQUEST);
            roomCreator.sendMessage(CMD_USERNAME_SUBMIT, "User");
            roomCreator.getResponse(CMD_ROOM_LIST_RESPONSE);
            roomCreator.sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
            String response = roomCreator.getResponse(CMD_ROOM_CREATE_OK);
            assertTrue(response.contains("Room created successfully"));

            for (Client joiner : joiners) {
                joiner.getResponse(CMD_WELCOME_REQUEST);
                joiner.sendMessage(CMD_USERNAME_SUBMIT, "User");
                joiner.getResponse(CMD_ROOM_LIST_RESPONSE);
                joiner.sendMessage(CMD_ROOM_JOIN_REQUEST, "0");
                String response1 = joiner.getResponse(CMD_ROOM_JOIN_OK);
                assertTrue(response1.contains("joined"));
            }

        } finally {
            roomCreator.close();
            disconnectClients(joiners);
        }
    }

    @Test
    @Timeout(value = 20, unit = TimeUnit.MILLISECONDS)
    public void testMaxUsersShouldBeAbleToJChatInARoom() throws IOException, InterruptedException {
        Client roomCreator = new Client();
        List<Client> joiners = createConnectedClients(MAX_CLIENTS_PER_ROOM - 1);
        List<String> messages = getRandomStrings(2, MAX_CONTENT_LENGTH - 1);
        try {
            roomCreator.connect();
            roomCreator.getResponse(CMD_WELCOME_REQUEST);
            roomCreator.sendMessage(CMD_USERNAME_SUBMIT, "User");
            roomCreator.getResponse(CMD_ROOM_LIST_RESPONSE);
            roomCreator.sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
            String response = roomCreator.getResponse(CMD_ROOM_CREATE_OK);
            assertTrue(response.contains("Room created successfully"));

            for (Client joiner : joiners) {
                joiner.getResponse(CMD_WELCOME_REQUEST);
                joiner.sendMessage(CMD_USERNAME_SUBMIT, "User");
                joiner.getResponse(CMD_ROOM_LIST_RESPONSE);
                joiner.sendMessage(CMD_ROOM_JOIN_REQUEST, "0");

                String joinResponse = joiner.getResponse(CMD_ROOM_JOIN_OK);
                assertTrue(joinResponse.contains("joined"));
            }
            for (String message : messages) {
                roomCreator.sendMessage(CMD_ROOM_MESSAGE_SEND, message);

            }
            assert (true);

            for (Client joiner : joiners) {
                List<String> expectedMessages = new ArrayList<>(messages);

                while (!expectedMessages.isEmpty()) {
                    String messageReceived = joiner.getResponse(CMD_ROOM_MSG);
                    Iterator<String> iterator = expectedMessages.iterator();

                    while (iterator.hasNext()) {
                        String expectedMessage = iterator.next();
                        if (messageReceived.contains(expectedMessage)) {
                            iterator.remove();
                            break;
                        }

                    }
                }
                assertTrue(true);

            }


        } finally {
            roomCreator.close();
            disconnectClients(joiners);

        }
  }

    class Client implements AutoCloseable {
        // Server Config
        private static final String HOST = "localhost";
        private static final int PORT = 30000;
        private Socket socket;
        private BufferedWriter writer;
        private final Map<Character, String> message = new HashMap<>();
        private final StringBuilder messageBuffer = new StringBuilder();
        private InputStream in;

        public void connect() throws IOException {
            socket = new Socket(HOST, PORT);
            writer = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
            in = socket.getInputStream();
        }

        public String getResponse(char expectedCmd) throws IOException, IndexOutOfBoundsException {
            try {
                while (!message.containsKey(expectedCmd)) {

                    int temp = in.read();

                    if (temp == -1) {
                        throw new IOException("Connection closed");
                    }

                    messageBuffer.append((char) temp);

                    while (messageBuffer.toString().contains("\r\n")) {
                        char cmdType = messageBuffer.charAt(0);
                        String content = messageBuffer.substring(2, messageBuffer.indexOf("\r\n"));
                        message.put(cmdType, content);
                        messageBuffer.setLength(0);
                    }
                }
            } catch (IndexOutOfBoundsException e) {
                if (messageBuffer.charAt(0) == '\r') {
                }
                fail(e.getMessage() + " " + messageBuffer);
            }
            return message.remove(expectedCmd);

        }

        public void sendMessage(char cmdType, String content) throws IOException {
            writer.write(cmdType + " " + content + "\r\n");
            writer.flush();
        }

        public void close() throws IOException {
            if (socket != null && !socket.isClosed()) {
                socket.close();
            }
        }
    }
}



