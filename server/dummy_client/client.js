const net = require('net');

// Configuration
const NUM_CLIENTS = 200;  // Create 10 test clients
const PORT = 30000;
const HOST = '127.0.0.1';

// Function to create a single client with an ID
function createClient(clientId) {
    const client = net.createConnection({
        port: PORT,
        host: HOST
    });

    client.on('connect', () => {
        console.log(`Client ${clientId}: Connected to server`);
        
    });

    client.on('data', (data) => {
        console.log(`Client ${clientId} received: ${data}`);
    });

    client.on('error', (err) => {
        console.log(`Client ${clientId} error: ${err.message}`);
    });

    client.on('close', () => {
        console.log(`Client ${clientId}: Connection closed`);
        // Reconnect after a delay
        setTimeout(() => createClient(clientId), 5000);
    });

}

// Create multiple clients
console.log(`Creating ${NUM_CLIENTS} test clients...`);
for(let i = 0; i < NUM_CLIENTS; i++) {
    setTimeout(() => {
        createClient(i);
    }, i );  // Stagger connection attempts by 500ms
}



process.on('SIGINT', () => {
    console.log('Shutting down clients...');
    process.exit();
});