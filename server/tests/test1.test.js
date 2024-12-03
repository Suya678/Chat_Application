const net = require("net");

const SERVER_CONFIG = {
	host: "localhost",
	port: 30000,
};

const ClientToServerCommands = {
	CMD_EXIT: 0x01,
	CMD_USERNAME_SUBMIT: 0x02,
	CMD_ROOM_CREATE_REQUEST: 0x03,
	CMD_ROOM_LIST_REQUEST: 0x04,
	CMD_ROOM_JOIN_REQUEST: 0x05,
	CMD_LEAVE_ROOM: 0x06,
	CMD_ROOM_MESSAGE_SEND: 0x07,
};

const ServerToClientCommands = {
	CMD_WELCOME_REQUEST: 0x16,
	CMD_ROOM_NOTIFY_JOINED: 0x17,
	CMD_ROOM_CREATE_OK: 0x18,
	CMD_ROOM_LIST_RESPONSE: 0x1a,
	CMD_ROOM_JOIN_OK: 0x1b,
	CMD_ROOM_MSG: 0x1c,
	CMD_ROOM_LEAVE_OK: 0x1d,
};

const SERVER_ERROR_CODES = {
	ERR_USERNAME_MISSING: 0x20,
	ERR_USERNAME_INVALID: 0x21,
	ERR_ROOM_NAME_EXISTS: 0x23,
	ERR_ROOM_NAME_INVALID: 0x24,
	ERR_ROOM_CAPACITY_FULL: 0x25,
	ERR_ROOM_NOT_FOUND: 0x26,
	ERR_SERVER_ROOM_FULL: 0x27,
	ERR_PROTOCOL_INVALID_STATE_CMD: 0x28,
	ERR_PROTOCOL_INVALID_FORMAT: 0x29,
	ERR_MSG_EMPTY_CONTENT: 0x2a,
	ERR_SERVER_FULL: 0x2b,
	ERR_CONNECTING: 0x2c,
	ERR_USERNAME_LENGTH: 0x2d,
};

/*
  #define MAX_THREADS 2
#define MAX_CLIENTS_PER_THREAD 1000 // How many client does each thread handles

#define MAX_CLIENTS_ROOM 40                        // Max clients per room
#define MAX_ROOMS 50                               // Max rooms
#define MAX_CLIENTS (MAX_CLIENTS_ROOM * MAX_ROOMS) // Total possible clients
  */
const SizeLimits = {
	MAX_USERNAME_LEN: 32,
	MAX_PASSWORD_LEN: 32,
	MAX_ROOM_NAME_LEN: 24,
	MAX_MESSAGE_LEN_TO_SERVER: 132,
	MAX_MESSAGE_LEN_FROM_SERVER: 2700,
	MAX_CONTENT_LEN: 128,
	MAX_CLIENTS: 2000,
	MAX_ROOMS: 50,
	MAX_CLIENTS_PER_ROOM: 40
};

class Client {
	constructor() {
		this.socket = new net.Socket();
		this.messageBuffer = "";
		this.fullMessage = new Map();
	}

	connect(PORT = SERVER_CONFIG.port, HOST = SERVER_CONFIG.host) {
		return new Promise((resolve, reject) => {
			this.socket.connect(PORT, HOST, () => resolve());

			this.socket.on("data", (data) => {
				this.messageBuffer += data.toString();
				if (!this.messageBuffer.includes("\r\n")) {
					return;
				}

				const messages = this.messageBuffer.split("\r\n");
				this.messageBuffer = messages.pop();

				if (this.messageBuffer.includes("\r\n")) {
					messages.push(this.messageBuffer);
					this.messageBuffer = "";
				}
				messages.forEach((msg) => {
					const cmdType = msg.charCodeAt(0);
					if (cmdType !== ServerToClientCommands.CMD_WELCOME_REQUEST) {
					}
					const content = msg.slice(2);
					this.fullMessage.set(cmdType, content);
				});
			});

			this.socket.on("end", () => {
				this.socket.destroy();
			});

			this.socket.on("error", () => {
				this.socket.destroy();
				reject(error);
			});
		});
	}

	getResponse(cmdType, tries = 100) {
		return new Promise((resolve, reject) => {
			const attempt = () => {
				if (this.fullMessage.has(cmdType)) {
					const message = this.fullMessage.get(cmdType);
					this.fullMessage.delete(cmdType);
					resolve(message);
				} else if (tries > 0) {
					tries--;
					setTimeout(attempt, 500);
				} else {
					reject(`Timeout waiting for the cmd ${cmdType} ${this.fullMessage.keys()}`);
				}
			};
			attempt();
		});
	}

	sendMessage(cmdType, content) {
		this.socket.write(`${String.fromCharCode(cmdType)} ${content}\r\n`);
	}

	disconnect() {
		this.socket.destroy();
	}
}

async function createConnectedClients(numOfClients) {
	const clients = Array.from({ length: numOfClients }, () => new Client());

	const promises = [];
	for (const client of clients) {
		promises.push(client.connect());
	}
	await Promise.all(promises);
	return clients;
}

function disconnectClients(clients) {
	clients.forEach((client) => client.disconnect());
}

jest.setTimeout(100000);
for(let i = 0; i < 2; i++) {
describe(`Connection`, () => {
	test("Should establish connection and receive welcome message", async () => {
		const client = new Client();
		await client.connect();
		const welcomeMessage = await client.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
		expect(welcomeMessage).toContain("WELCOME TO THE SERVER");
		client.disconnect();
	});

	test(`Max clients (${SizeLimits.MAX_CLIENTS}) should be able to connect `, async () => {
		const clients = await createConnectedClients(SizeLimits.MAX_CLIENTS);

		await Promise.all(
			clients.map(async (client) => {
				const welcomeMessage = await client.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
				expect(welcomeMessage).toContain("WELCOME TO THE SERVER");
			}),
		);
		disconnectClients(clients);
	});

	test("Should reject connection when server is at capacity,", async () => {
		const clients = await createConnectedClients(SizeLimits.MAX_CLIENTS);

		// Try to connect more clients, should get rejection message
		const overflowClients = await createConnectedClients(SizeLimits.MAX_CLIENTS);

		await Promise.all(
			overflowClients.map(async (client) => {
				const rejectionMessage = await client.getResponse(SERVER_ERROR_CODES.ERR_SERVER_FULL);
				expect(rejectionMessage).toContain("the server is currently at full");
			}),
		);
		disconnectClients(clients);
		disconnectClients(overflowClients);
	});

	test("Should free up connection spots after client disconnectes", async () => {
		const clients = await createConnectedClients(SizeLimits.MAX_CLIENTS);

		// Try to connect one more client
		const extraClient = new Client();
		await extraClient.connect();
		const rejectionMessage = await extraClient.getResponse(SERVER_ERROR_CODES.ERR_SERVER_FULL);

		expect(rejectionMessage).toContain("the server is currently at full");
		extraClient.disconnect();

		for (let l = 0; l < clients.length / 2; l++) {
			clients[l].disconnect();
		}

		// Try reconnecting
		for (let l = 0; l < clients.length / 2; l++) {
			await clients[l].connect();
			const welcomeMessage = await clients[l].getResponse(
				ServerToClientCommands.CMD_WELCOME_REQUEST,
			);
			expect(welcomeMessage).toContain("WELCOME TO THE SERVER");
		}

		disconnectClients(clients);
	});
});

describe("Username Registration", () => {
	test("Should reject empty username", async () => {
		const client = new Client();
		await client.connect();
		await client.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
		client.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, " ");
		const response = await client.getResponse(SERVER_ERROR_CODES.ERR_MSG_EMPTY_CONTENT);
		expect(response).toContain("Content is Empty");
		client.disconnect();
	});

	test(`Should reject username with length greater than ${SizeLimits.MAX_USERNAME_LEN}`, async () => {
		const client = new Client();
		await client.connect();
		await client.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
		client.sendMessage(
			ClientToServerCommands.CMD_USERNAME_SUBMIT,
			"lkjsadjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlkdsajjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj",
		);
		const response = await client.getResponse(SERVER_ERROR_CODES.ERR_USERNAME_LENGTH);
		expect(response).toContain("User name too long");
		client.disconnect();
	});

	test(`Should accept valid usernames from Max number of clients ${SizeLimits.MAX_CLIENTS}`, async () => {
		const clients = await createConnectedClients(SizeLimits.MAX_CLIENTS);

		await Promise.all(
			clients.map(async (client) => {
				await client.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
				client.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "ValidUser");
				const response = await client.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
				expect(response).toContain("Available Chat Rooms");
			}),
		);

		disconnectClients(clients);
	});

	test(`Should  reject usernames longer than ${SizeLimits.MAX_USERNAME_LEN} from multiple clients`, async () => {
		const clients = await createConnectedClients(SizeLimits.MAX_CLIENTS);

		await Promise.all(
			clients.map(async (client) => {
				await client.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
				client.sendMessage(
					ClientToServerCommands.CMD_USERNAME_SUBMIT,
					"lkjsadjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjalskdkjldsajkla",
				);
				const response = await client.getResponse(SERVER_ERROR_CODES.ERR_USERNAME_LENGTH);
				expect(response).toContain("User name too long");
			}),
		);
		disconnectClients(clients);
	});
});

describe("Room Creation", () => {
	test("Should be able to create a room with a valid name and lenght", async () => {
		const user = new Client();
		await user.connect();
		await user.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
		user.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "User");
		await user.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
		user.sendMessage(ClientToServerCommands.CMD_ROOM_CREATE_REQUEST, "Room");
		const response = await user.getResponse(ServerToClientCommands.CMD_ROOM_CREATE_OK);
		expect(response).toContain("Room created successfully");
		user.disconnect();
	});

	test(`Should reject room creation when name exceeds maximum length ${SizeLimits.MAX_ROOM_NAME_LEN} `, async () => {
		const user = new Client();
		await user.connect();
		await user.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
		user.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "User");
		await user.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
		user.sendMessage(
			ClientToServerCommands.CMD_ROOM_CREATE_REQUEST,
			"jiasdkjasdkjladsjlkaskljdkjlakjjhsadhashdjkhasdkjhdsadklsjakljsadkljsakljdkljsakldjsakljkl",
		);
		const response = await user.getResponse(SERVER_ERROR_CODES.ERR_ROOM_NAME_INVALID);
		expect(response).toContain("Room name length invalid");
		user.disconnect();
	});

	test(`Should successfully create maximum number of rooms ${SizeLimits.MAX_ROOMS}`, async () => {
		const clients = await createConnectedClients(SizeLimits.MAX_ROOMS);

		await Promise.all(
			clients.map(async (client) => {
				await client.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
				client.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "User");
				await client.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
				client.sendMessage(ClientToServerCommands.CMD_ROOM_CREATE_REQUEST, "Room");
				const response = await client.getResponse(ServerToClientCommands.CMD_ROOM_CREATE_OK);
				expect(response).toContain("Room created successfully");
			}),
		);
		disconnectClients(clients);
	});

	test(`Should reject room creation when server reaches maximum capacity of ${SizeLimits.MAX_ROOMS} rooms`, async () => {
		const initialRoomClients = await createConnectedClients(SizeLimits.MAX_ROOMS);

		await Promise.all(
			initialRoomClients.map(async (client) => {
				await client.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
				client.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "User");
				await client.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
				client.sendMessage(ClientToServerCommands.CMD_ROOM_CREATE_REQUEST, "Room");
				const response = await client.getResponse(ServerToClientCommands.CMD_ROOM_CREATE_OK);
				expect(response).toContain("Room created successfully");
			}),
		);

		const overflowClients = await createConnectedClients(SizeLimits.MAX_ROOMS);

		await Promise.all(
			overflowClients.map(async (client) => {
				await client.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
				client.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "User");
				await client.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
				client.sendMessage(ClientToServerCommands.CMD_ROOM_CREATE_REQUEST, "Room");
				const response = await client.getResponse(SERVER_ERROR_CODES.ERR_ROOM_CAPACITY_FULL);
				expect(response).toContain("Room creation failed");
			}),
		);

		disconnectClients(initialRoomClients);
		disconnectClients(overflowClients);
	});



});


describe("Room Management", () => {
	test("Should be able to join an already created room", async () => {
		const creator = new Client();
		await creator.connect();
		await creator.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
		creator.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "Creator");
		await creator.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
		creator.sendMessage(ClientToServerCommands.CMD_ROOM_CREATE_REQUEST, "Room");
		const response = await creator.getResponse(ServerToClientCommands.CMD_ROOM_CREATE_OK);
		expect(response).toContain("Room created successfully");

		const joiner = new Client();
		await joiner.connect();
		await joiner.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
		joiner.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "joiner");
		await joiner.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
		joiner.sendMessage(ClientToServerCommands.CMD_ROOM_JOIN_REQUEST, "0");
		const response2 = await joiner.getResponse(ServerToClientCommands.CMD_ROOM_JOIN_OK);
		expect(response2).toContain("joined room");
		   
		joiner.disconnect();
		creator.disconnect();
	});

	test(`Should send an error message when a client tries to join a non exsisting room`, async () => {
		const creator = new Client();
		await creator.connect();
		await creator.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
		creator.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "Creator");
		await creator.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
		creator.sendMessage(ClientToServerCommands.CMD_ROOM_CREATE_REQUEST, "Room");
		const response = await creator.getResponse(ServerToClientCommands.CMD_ROOM_CREATE_OK);
		expect(response).toContain("Room created successfully");

		const joiner = new Client();
		await joiner.connect();
		await joiner.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
		joiner.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "joiner");
		await joiner.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
		joiner.sendMessage(ClientToServerCommands.CMD_ROOM_JOIN_REQUEST, "1");
		const response2 = await joiner.getResponse(SERVER_ERROR_CODES.ERR_ROOM_NOT_FOUND);
		expect(response2).toContain("Room does not exist");
		   
		joiner.disconnect();
		creator.disconnect();
	});

	test(`Max number of clients per room (${SizeLimits.MAX_CLIENTS_PER_ROOM}) should be able to join a room`, async () => {
		const creator = new Client();
		await creator.connect();
		await creator.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
		creator.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "Creator");
		await creator.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
		creator.sendMessage(ClientToServerCommands.CMD_ROOM_CREATE_REQUEST, "Room");
		const response = await creator.getResponse(ServerToClientCommands.CMD_ROOM_CREATE_OK);
		expect(response).toContain("Room created successfully");

		const joiners = await createConnectedClients(SizeLimits.MAX_CLIENTS_PER_ROOM - 1);
		await Promise.all(
			joiners.map(async (joiner) => {
				await joiner.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
				joiner.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "joiner");
				await joiner.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
				joiner.sendMessage(ClientToServerCommands.CMD_ROOM_JOIN_REQUEST, "0");
				const response2 = await joiner.getResponse(ServerToClientCommands.CMD_ROOM_JOIN_OK);
				expect(response2).toContain("joined room");
			}),
		);
		disconnectClients(joiners);
		creator.disconnect();
	});


	test(`Should send an error message when a client tries to join a room at capacity, clients - (${SizeLimits.MAX_CLIENTS_PER_ROOM})`, async () => {
		const creator = new Client();
		await creator.connect();
		await creator.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
		creator.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "Creator");
		await creator.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
		creator.sendMessage(ClientToServerCommands.CMD_ROOM_CREATE_REQUEST, "Room");
		const response = await creator.getResponse(ServerToClientCommands.CMD_ROOM_CREATE_OK);
		expect(response).toContain("Room created successfully");

		// These should be able to join the room
		const successJoiners = await createConnectedClients(SizeLimits.MAX_CLIENTS_PER_ROOM - 1);
		await Promise.all(
			successJoiners.map(async (joiner) => {
				await joiner.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
				joiner.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "joiner");
				await joiner.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
				joiner.sendMessage(ClientToServerCommands.CMD_ROOM_JOIN_REQUEST, "0");
				const response2 = await joiner.getResponse(ServerToClientCommands.CMD_ROOM_JOIN_OK);
				expect(response2).toContain("joined room");
			}),
		);

		// These should not be able to
		const unsucessfullJoiners = await createConnectedClients(SizeLimits.MAX_CLIENTS_PER_ROOM );
		await Promise.all(
			unsucessfullJoiners.map(async (joiner) => {
				await joiner.getResponse(ServerToClientCommands.CMD_WELCOME_REQUEST);
				joiner.sendMessage(ClientToServerCommands.CMD_USERNAME_SUBMIT, "joiner");
				await joiner.getResponse(ServerToClientCommands.CMD_ROOM_LIST_RESPONSE);
				joiner.sendMessage(ClientToServerCommands.CMD_ROOM_JOIN_REQUEST, "0");
				const response2 = await joiner.getResponse(SERVER_ERROR_CODES.ERR_ROOM_CAPACITY_FULL);
				expect(response2).toContain("Room is full");
			}),
		);
		disconnectClients(successJoiners);
		disconnectClients(unsucessfullJoiners);
		creator.disconnect();
	});





	
});

}
