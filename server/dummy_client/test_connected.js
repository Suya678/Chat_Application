const net = require('net');
const CLIENT_CONFIG = {
    host: 'localhost',
    port: 30000
};

const client1 = new net.Socket();
const client2 = new net.Socket();

// Helper function for client setup
function setupClient(client, name) {
    client.on('data', (data) => {
        console.log(`${name} received from server:`);
        console.log(data.toString());
    });

    client.on('close', () => {
        console.log(`${name} connection closed`);
    });

    client.on('error', (err) => {
        console.error(`${name} connection error:`, err);
    });
}

setupClient(client1, 'Client1');
setupClient(client2, 'Client2');

client1.connect(CLIENT_CONFIG.port, CLIENT_CONFIG.host, () => {
    console.log('Client1 connected to server');
    
    // Send username
    setTimeout(() => {
        const username = String.fromCharCode(0x01) + ' Creator' + '\r\n';
        console.log(`Client1 sending username: ${username}`);
        client1.write(username);
    }, 1000);

    // Create room
    setTimeout(() => {
        const createRoom = String.fromCharCode(0x02) + ' TestRoom1' + '\r\n';
        console.log(`Client1 creating room: ${createRoom}`);
        client1.write(createRoom);
    }, 2000);
});

// Connect second client (room lister) with delay
setTimeout(() => {
    client2.connect(CLIENT_CONFIG.port, CLIENT_CONFIG.host, () => {
        console.log('Client2 connected to server');
        
        // Send username
        setTimeout(() => {
            const username = String.fromCharCode(0x01) + ' Viewer' + '\r\n';
            console.log(`Client2 sending username: ${username}`);
            client2.write(username);
        }, 1000);

        // List rooms
        setTimeout(() => {
            const listRooms = String.fromCharCode(0x03) + ' list' + '\r\n';
            console.log(`Client2 requesting room list: ${listRooms}`);
            client2.write(listRooms);
        }, 2000);
    });
}, 3000);  

process.on('SIGINT', () => {
    console.log('\nClosing connections...');
    client1.destroy();
    client2.destroy();
    process.exit();
});