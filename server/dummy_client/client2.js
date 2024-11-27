const net = require('net');

const NUM_CLIENTS = 10;  // Create 10 test clients
const PORT = 30000;
const HOST = '127.0.0.1';

function createClient(clientId) {
    const client = net.createConnection({
        port: PORT,
        host: HOST
    });

    client.on('connect', () => {
        console.log(`Client ${clientId}: Connected to server`);
        setInterval(() => {
            client.write(`Hello from client ${clientId}`);
        }, 2000 + (clientId * 100));  
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

    return client;
}




console.log(`Creating ${NUM_CLIENTS} test clients...`);
for(let i = 0; i < NUM_CLIENTS; i++) {
    setTimeout(() => {
        createClient(i);
    }, i * 500);  // Stagger connection attempts by 500ms
}

process.on('SIGINT', () => {
    console.log('Shutting down clients...');
    process.exit();
});