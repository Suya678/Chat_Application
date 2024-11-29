const net = require('net');
const readline = require('readline');
const help = `
Available commands:
    /list   - List available rooms
    /create - Create a new room
    /join   - Join a room
    /exit   - Exit the chat
    /help   - Show this help message

Just type to send a message when in a room
Once in room these, only exit and help will work
                    `;

let client_in_room = false;
const SERVER_CONFIG = {
    host: 'localhost',
    port: 30000
};

const COMMANDS = {
    EXIT: 0x01,
    USERNAME: 0x02,
    CREATE_ROOM: 0x03,
    LIST_ROOMS: 0x04,
    JOIN_ROOM: 0x05,
    SEND_MESSAGE: 0x06,
};



const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});
const client = new net.Socket();


client.connect(SERVER_CONFIG.port, SERVER_CONFIG.host, () => {
    console.log('Connected to server');
    rl.setPrompt('> ');
    rl.prompt();

    rl.on('line', (input)=> {
        const userInput = input.trim();
        if(userInput == ''){
            rl.prompt();
        }
        if(userInput.startsWith('/')) {
            const command = userInput.slice(1).toLowerCase();

            switch(command) {
                case('list'):
                    client.write(`${String.fromCharCode(COMMANDS.LIST_ROOMS)} list\r\n`);
                    break;

                case('create'):
                    rl.question("Enter the room name to create:", (room)=> {
                        client.write(`${String.fromCharCode(COMMANDS.CREATE_ROOM)} ${room}\r\n`);
                        client_in_room = true;
                    });
                    break;    

                case('join'):
                    rl.question("Enter a room number to join:", (room)=> {
                        client.write(`${String.fromCharCode(COMMANDS.JOIN_ROOM)} ${room}\r\n`);
                    });
                    break;

                case 'exit':
                    client.write(`${String.fromCharCode(COMMANDS.EXIT)} exit\r\n`);
                    client.end();
                    rl.close();
                    process.exit();
                    break;

                default:
                    console.log(help);
                    break;
            }
        } else {
            client.write(`${String.fromCharCode(COMMANDS.SEND_MESSAGE)} ${userInput}\r\n`);
        }
    
        rl.prompt();

        


    });


});


client.on('data', (data) => {
    console.log("\n"+ data.toString().slice(1).trim("\r\n") );
    rl.prompt(true);
  });

client.on('connect', () => {
    rl.question('Enter your username: ', (username) => {
        client.write(`${String.fromCharCode(COMMANDS.USERNAME)} ${username}\r\n`);
        console.log('\nType /help to see available commands');
        rl.prompt();
    });
});




client.on('close', () => {
    console.log('\nDisconnected from server');
    rl.close();
    process.exit(0);
});

client.on('error', (err) => {
    console.error('Connection error:', err.message);
    process.exit(1);
});



process.on('SIGINT', () => {
    client.end();
    rl.close();
    process.exit();
});










