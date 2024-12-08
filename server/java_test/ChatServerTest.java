import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.*;
import java.net.Socket;
import java.util.*;
import org.junit.Test;

/**
 * Tests for the Chat Server implementation. This class contains tests to verify the functionality
 * of a chat server.
 */
public class ChatServerTest {

  // Size Limits
  public static final int MAX_CLIENTS = 6000;
  public static final int MAX_ROOMS = 50;
  public static final int MAX_USERNAME_LEN = 32;
  public static final int MAX_ROOM_NAME_LEN = 24;
  public static final int MAX_CLIENTS_PER_ROOM = 120;
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
   * <p>
   *
   * @param count The number of clients to create and connect
   * @return List of connected Client objects
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
   * <p>
   *
   * @param clients List of Client objects to disconnect
   */
  private void disconnectClients(List<Client> clients) throws InterruptedException, IOException {
    for (Client client : clients) {
      client.close();
    }
  }

  /**
   * Generates a list of random strings for testing purposes.
   *
   * <p>
   *
   * @param count The number of random strings to generate
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
   * Creates a new client, and waits the for the welcome message from the server and sends the
   * userName to the server and returns the new Client
   *
   * <p>
   *
   * @param username The username for the new client
   * @return Client created with the given username
   */
  private Client setupClientWithUsername(String username) throws IOException {
    Client client = new Client();
    client.getResponse(CMD_WELCOME_REQUEST);
    client.sendMessage(CMD_USERNAME_SUBMIT, username);
    return client;
  }

  /**
   * Verifies that each client received every message in the provided message list. Messages can be
   * received in any order. Keeps checking each client's responses until all messages are accounted
   * for. This might get stuck in an infinite loop if a client does not receive the messages. Hence,
   * it verifies by returning back. A
   *
   * <p>
   *
   * @param clients List of clients to check messages for.
   * @param messages List of messages that each client should have received.
   * @implNote May get stuck in an infinite loop. Any test using this should use
   *     the @Test(timeout=...) functionality
   */
  public void verifyClientsReceivedMessages(List<Client> clients, List<String> messages)
      throws IOException, InterruptedException {
    for (Client client : clients) {
      List<String> messagesCopy = new ArrayList<>(messages);
      while (!messagesCopy.isEmpty()) {
        String response = client.getResponse(CMD_ROOM_MSG);
        messagesCopy.removeIf(response::contains);
      }
    }
  }

  /**
   * Sets up a specified number of clients and makes them join a given room. Each client is created
   * with a unique username and verified to have joined successfully.
   *
   * @param count Number of clients to create and add to the room
   * @param roomId ID(index) of the room for clients to join
   * @return List of created clients that joined the room
   * @implNote Will get stuck in an infinite loop if there are no slots in the room.
   */
  public List<Client> setupClientsWithinRoom(int count, int roomId) throws IOException {
    List<Client> clients = new ArrayList<>();
    for (int i = 0; i < count; i++) {
      Client joiner = setupClientWithUsername("Random user" + i);
      joiner.sendMessage(CMD_ROOM_JOIN_REQUEST, String.valueOf(roomId));
      assertTrue(joiner.getResponse(CMD_ROOM_JOIN_OK).contains("joined"));
      clients.add(joiner);
    }
    return clients;
  }

  /**
   * Creates a new client, sets their username, and makes them create a room. Verifies that room
   * creation was successful.
   *
   * @param userName Username for the client
   * @param roomName Name of the room to create
   * @return The created client that owns the new room
   * @implNote Will get stuck in an infinite loop if the server's room capacity is full
   */
  public Client setupRoomCreator(String userName, String roomName)
      throws IOException, InterruptedException {
    Client client = setupClientWithUsername(userName);
    client.sendMessage(CMD_ROOM_CREATE_REQUEST, roomName);
    assertTrue(client.getResponse(CMD_ROOM_CREATE_OK).contains("Room created successfully"));
    return client;
  }

  /**
   * Tests that the server correctly: 1. Accepts connections from a single 2. Sends proper welcome
   * messages to the client
   */
  @Test
  public void testBasicConnection() throws IOException, InterruptedException {
    Client client = new Client();
    String welcomeMessage = client.getResponse(CMD_WELCOME_REQUEST);
    assertTrue(
        "Welcome message should contain expected ",
        welcomeMessage.contains("WELCOME TO THE SERVER"));
    client.close();
  }

  /**
   * Tests that the server correctly: 1. Accepts connections up to maximum server capacity 2. Sends
   * proper welcome messages to each connected client
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
   * Tests that the server correctly: Rejects connection when the server is at full client capacity
   * by sending a Server full message and closing the connection
   */
  @Test
  public void testRejectClientsWhenAtFullCapacity() throws IOException, InterruptedException {
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
   * Tests that the server correctly: 1. Frees a connection slot when a client disconnects 2.
   * Accepts a new client after in the spot of the client that disconnected
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
   * Tests that the server correctly: Rejects usernames that are empty by sending an error message.
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
   * Tests that the server correctly: Rejects usernames that are longer than the maximum allowed
   * username length
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

  /** Tests that the server correctly: Accepts usernames with a valid length */
  @Test
  public void testAcceptValidUsernamesFromMaxClients() throws IOException, InterruptedException {
    List<Client> clients = createConnectedClients(MAX_CLIENTS);

    for (Client client : clients) {
      client.getResponse(CMD_WELCOME_REQUEST);
      client.sendMessage(CMD_USERNAME_SUBMIT, "User");
      String response = client.getResponse(CMD_ROOM_LIST_RESPONSE);
      assertTrue(response.contains("Rooms"));
    }

    disconnectClients(clients);
  }

  /** Tests that the server correctly: Creates a room with a valid room name length */
  @Test
  public void testCreateValidRoom() throws IOException, InterruptedException {
    Client client = setupClientWithUsername("a");
    client.sendMessage(CMD_ROOM_CREATE_REQUEST, "ValidRoom");
    String response = client.getResponse(CMD_ROOM_CREATE_OK);
    assertTrue(response.contains("Room"));
    client.close();
  }

  /**
   * Tests that the server correctly: Rejects room creation if the user sends a room name wit the
   * length greater than the maximum allowed username length
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
   * Tests that the server correctly: Allows the creation of the maximum number of rooms at the same
   * time
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
   * Tests that the server correctly: Rejects creating new rooms once the maximum room capacity is
   * reached.
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

  /** Tests that the server correctly: Returns the correct list of all created rooms. */
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
   * Tests that the server correctly: Allows the maximum number of users per room to join a single
   * room.
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
   * Tests that the server correctly: Broadcasts all message by one client to other clients in a
   * single room.
   */
  @Test
  public void testAllClientsReceiveMessagesInARoom() throws IOException, InterruptedException {
    List<String> testMessages = getRandomStrings(30, MAX_CONTENT_LENGTH - 1);
    Client roomCreator = setupClientWithUsername("Room Creator");
    roomCreator.sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
    String response = roomCreator.getResponse(CMD_ROOM_CREATE_OK);
    assertTrue(response.contains("Room created successfully"));
    List<Client> joiners = setupClientsWithinRoom(MAX_CLIENTS_PER_ROOM - 1, 0);

    for (String message : testMessages) {
      roomCreator.sendMessage(CMD_ROOM_MESSAGE_SEND, message);
    }

    // Checking if the joiners received all the messages that room creator sent
    verifyClientsReceivedMessages(joiners, testMessages);

    disconnectClients(joiners);
    roomCreator.close();
  }

  /** Tests that the server correctly: Broadcasts messages to all clients in max rooms */
  @Test
  public void testMessageDeliveryAcrossAllRooms() throws IOException, InterruptedException {
    // Need to maintain clients in a list,otherwise they will be garbage collected
    List<Client> allClients = new ArrayList<>();
    System.out.println(
        "\nStarting message delivery test across " + MAX_ROOMS + " rooms...\nMight take a while");
    // Test for all rooms in the server
    for (int i = 0; i < MAX_ROOMS; i++) {
      System.out.println("Testing Room " + i + "/" + MAX_ROOMS);
      List<String> testMessages = getRandomStrings(30, MAX_CONTENT_LENGTH - 1);

      Client roomCreator = setupClientWithUsername("Room Creator");
      allClients.add(roomCreator);
      roomCreator.sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
      String response = roomCreator.getResponse(CMD_ROOM_CREATE_OK);
      assertTrue(response.contains("Room created successfully"));

      List<Client> joiners = setupClientsWithinRoom(MAX_CLIENTS_PER_ROOM - 1, i);
      allClients.addAll(joiners);

      for (String message : testMessages) {
        roomCreator.sendMessage(CMD_ROOM_MESSAGE_SEND, message);
      }

      // Checking if the joiners received all the messages that room creator sent
      verifyClientsReceivedMessages(joiners, testMessages);
      System.out.println("✓ All messages received correctly\n");
    }
    disconnectClients(allClients);
  }

  /**
   * Tests that the server correctly: Allows the user to leave a room and go back to the chat lobby
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
   * Tests that the server correctly: Cleans up the resources used by a room when all clients have
   * left the room
   */
  @Test
  public void roomIsCleanedUpAfterALlUsersLeave() throws IOException, InterruptedException {
    Client roomCreator = setupRoomCreator("Room Creator", "Dummy Room");

    roomCreator.sendMessage(CMD_LEAVE_ROOM, "Room");

    assertTrue(roomCreator.getResponse(CMD_ROOM_LEAVE_OK).contains("left the room"));

    roomCreator.sendMessage(CMD_ROOM_LIST_REQUEST, "Dummy content");
    assertTrue(roomCreator.getResponse(CMD_ROOM_LIST_RESPONSE).contains("No chat rooms available"));

    roomCreator.close();
  }

  /**
   * Tests that the server correctly: Does not clean up the room resources if a client leaves if
   * there are other clients in it.
   */
  @Test
  public void testRoomPersistsAfterUserLeaves() throws IOException, InterruptedException {
    List<Client> clients = new ArrayList<>();
    clients.add(setupClientWithUsername("roomCreator"));
    clients.getFirst().getResponse(CMD_ROOM_LIST_RESPONSE);
    clients.getFirst().sendMessage(CMD_ROOM_CREATE_REQUEST, "Dummy Room");
    String response = clients.getFirst().getResponse(CMD_ROOM_CREATE_OK);
    assertTrue(response.contains("Room created successfully"));

    for (int i = 1; i < MAX_CLIENTS_PER_ROOM; i++) {
      clients.add(setupClientWithUsername("Random name"));
      clients.get(i).sendMessage(CMD_ROOM_JOIN_REQUEST, "0");
      response = clients.get(i).getResponse(CMD_ROOM_JOIN_OK);
      assertTrue(response.contains("joined"));
    }

    clients.getFirst().sendMessage(CMD_LEAVE_ROOM, "dummy");
    response = clients.getFirst().getResponse(CMD_ROOM_LEAVE_OK);
    assertTrue(response.contains("left"));

    clients.getFirst().sendMessage(CMD_ROOM_LIST_REQUEST, "dummy");
    response = clients.getFirst().getResponse(CMD_ROOM_LIST_RESPONSE);
    assertTrue(response.contains("Dummy Room"));

    disconnectClients(clients);
  }

  /**
   * Tests that the server correctly: Allows a user to create a room after they have left a room.
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
   * Tests that the server correctly: Allows a user to join the same room they left if there were
   * other users in it to stop the cleanup.
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

  /** Tests that the server correctly: Closes the connection when the user requests to exit */
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
   * Tests that the server correctly: Broadcasts a message that a new member has joined to existing
   * members whenever a new client joins a room.
   */
  @Test
  public void testRoomJoinMessageToExistingUser() throws IOException, InterruptedException {
    Client roomCreator = setupClientWithUsername("room creator");

    roomCreator.sendMessage(CMD_ROOM_CREATE_REQUEST, "Room");
    assertTrue(roomCreator.getResponse(CMD_ROOM_CREATE_OK).contains("Room created successfully"));
    List<Client> joiners = createConnectedClients(MAX_CLIENTS_PER_ROOM - 1);

    for (Client client : joiners) {
      client.sendMessage(CMD_USERNAME_SUBMIT, "Joiner");
      client.sendMessage(CMD_ROOM_JOIN_REQUEST, String.valueOf(0));
      client.getResponse(CMD_ROOM_JOIN_OK);
      String response = roomCreator.getResponse(CMD_ROOM_MSG);
      assertTrue(response.contains("entered the room"));
    }

    roomCreator.close();
    disconnectClients(joiners);
  }

  /**
   * Tests that the server correctly: Only broadcasts room to clients in the same room. Maintains
   * messaging isolation between rooms
   */
  @Test(timeout = 100000) // To avoid infinite loops in getResponse calls
  public void testMessageIsolationBetweenRooms() throws IOException, InterruptedException {
    // Create two rooms with their creators
    Client room1Creator = setupRoomCreator("Room1Creator", "Room1");

    Client room2Creator = setupRoomCreator("Room2Creator", "Room2");

    // Add some users to Room 1
    List<Client> room1Joiners = setupClientsWithinRoom(MAX_CLIENTS_PER_ROOM - 1, 0);
    // Add some users to Room 2
    List<Client> room2Joiners = setupClientsWithinRoom(MAX_CLIENTS_PER_ROOM - 1, 1);

    // Send test messages in room 1
    List<String> room1TestMessages = getRandomStrings(100, MAX_CONTENT_LENGTH);
    for (String message : room1TestMessages) {
      room1Creator.sendMessage(CMD_ROOM_MESSAGE_SEND, message);
    }
    // Send test messages in room 2
    List<String> room2TestMessages = getRandomStrings(100, MAX_CONTENT_LENGTH);
    for (String message : room2TestMessages) {
      room2Creator.sendMessage(CMD_ROOM_MESSAGE_SEND, message);
    }

    // Verify Room 1 users received only Room 1's message
    verifyClientsReceivedMessages(room1Joiners, room1TestMessages);
    for (Client client : room1Joiners) {
      assertFalse(client.hasMessages()); // Checks all messages have been consumed
    }

    // Verify Room 2 users received only Room 1's message
    verifyClientsReceivedMessages(room2Joiners, room2TestMessages);
    for (Client client : room2Joiners) {
      assertFalse(client.hasMessages()); // Checks all messages have been consumed
    }

    room1Creator.close();
    room2Creator.close();
    disconnectClients(room1Joiners);
    disconnectClients(room2Joiners);
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
     * Initializes the client by connecting to the server and setting up writing and reading
     * streams.
     *
     * <p>Establishes a connection to the server at the specified host and port.
     */
    public Client() throws IOException {
      socket = new Socket(HOST, PORT);
      writer = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
      in = socket.getInputStream();
    }

    /**
     * Retrieves the server's response to a previously sent command.
     *
     * <p>This method reads data from the server socket until the expected command type is found in
     * the response. It processes incoming data, extracts messages based on the expected command,
     * and returns the associated content.
     *
     * @param expectedCmd The command type to wait for in the server's response.
     * @return The content portion of the server's response associated with the expected command.
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
        fail(
            e.getMessage()
                + " "
                + (int) messageBuffer.charAt(0)
                + " "
                + (int) messageBuffer.charAt(1));
      }
      return message.remove(expectedCmd);
    }

    /**
     * Sends the given content with the cmdtype to the server socket
     *
     * @param cmdType Command of the message
     * @param content Content portion of the message
     */
    public void sendMessage(char cmdType, String content) throws IOException {
      writer.write(cmdType + " " + content + "\r\n");
      writer.flush();
    }

    /** Checks if the socket has messages with a 100 milliseconds */
    public boolean hasMessages() throws IOException {
      socket.setSoTimeout(100);
      try {
        return socket.getInputStream().available() > 0;
      } finally {
        socket.setSoTimeout(0);
      }
    }

    /** Closes the socket. Introduces a small delay between rapid sleep calls */
    public void close() throws IOException, InterruptedException {
      if (socket != null && !socket.isClosed()) {
        socket.close();
      }
    }
  }
}
