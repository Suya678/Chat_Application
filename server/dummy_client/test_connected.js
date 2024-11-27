const net = require('net');

const CLIENT_CONFIG = {
    host: 'localhost',
    port: 30000
};

const client = new net.Socket();

// Connect to server
client.connect(CLIENT_CONFIG.port, CLIENT_CONFIG.host, () => {
    console.log('Connected to server');
    
    const testUsername = 'TestUser\r\n';
    console.log(`Sent username: ${testUsername}`);
    setTimeout(()=> {
        client.write(testUsername);
    },1000);
});

client.on('data', (data) => {
    console.log('Received from server:');
    console.log(data.toString());
    
});

// Handle connection close
client.on('close', () => {
    console.log('Connection closed');
});

// Handle errors
client.on('error', (err) => {
    console.error('Connection error:', err);
});

// Clean up on process exit
process.on('SIGINT', () => {
    console.log('\nClosing connection...');
    client.destroy();
    process.exit();
});