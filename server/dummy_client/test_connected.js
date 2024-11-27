const net = require('net');

const CLIENT_CONFIG = {
    host: 'localhost',
    port: 30000
};

const MAX_CLIENTS = 10016;
let connectedClients = 0;
let failedConnections = 0;
const clients = [];

function setupClient(client, clientId) {
    client.on('data', () => {});
    client.on('close', () => connectedClients--);
    client.on('error', () => failedConnections++);
}

function createClient(clientId) {
    const client = new net.Socket();
    setupClient(client, clientId);

    client.connect(CLIENT_CONFIG.port, CLIENT_CONFIG.host, () => {
        connectedClients++;
        client.write(String.fromCharCode(0x02) + ` User${clientId}\r\n`);
    });

    return client;
}

async function test() {
    console.log("Phase 1: Testing maximum connections...");
    
    // Try to connect MAX_CLIENTS + 100 to verify limit
    for (let i = 0; i < MAX_CLIENTS + 100; i++) {
        clients.push(createClient(i));
        await new Promise(resolve => setTimeout(resolve, 10)); // Small delay between connections
    }

    // Wait for 30 seconds to see stable connection count
    await new Promise(resolve => setTimeout(resolve, 30000));
    console.log(`
    After max connection test:
    Connected Clients: ${connectedClients}
    Failed Connections: ${failedConnections}
    `);

    // Disconnect all clients
    console.log("\nPhase 2: Disconnecting all clients...");
    clients.forEach(client => client.destroy());
    await new Promise(resolve => setTimeout(resolve, 5000));

    // Clear clients array
    clients.length = 0;
    failedConnections = 0;

    // Try to connect MAX_CLIENTS again
    console.log("\nPhase 3: Testing reconnection of max clients...");
    for (let i = 0; i < MAX_CLIENTS; i++) {
        clients.push(createClient(i));
        await new Promise(resolve => setTimeout(resolve, 10));
    }

    // Wait for 30 seconds to see stable connection count
    await new Promise(resolve => setTimeout(resolve, 30000));
    console.log(`
    After reconnection test:
    Connected Clients: ${connectedClients}
    Failed Connections: ${failedConnections}
    `);
}

test();

process.on('SIGINT', () => {
    console.log('\nShutting down...');
    clients.forEach(client => client.destroy());
    process.exit();
});