This project is a simple multiplayer Tic-Tac-Toe game implemented in C++. The game uses sockets for communication between a server and clients,
and threads to handle multiple games simultaneously.

**Prerequisites**
A Unix-like operating system (Linux, macOS)
C++ compiler supporting C++11 or later
make utility (optional, for ease of building)

**Installation**

Clone the repository:

>git clone https://github.com/yourusername/tictactoe-multiplayer.git cd tictactoe-multiplayer

Compile the server and client programs:

**make**

If you don't have make, you can compile manually:

>g++ -o server server.cpp -lpthread
>g++ -o client client.cpp

**Running the Game**


-Starting the Server
To start the server, run the following command with a port number:

>./server <port>

-For example:

>./server 12345

-Connecting Clients
To connect a client to the server, run the following command with the server hostname and port number:

>./client <hostname> <port>

For example:

>./client localhost 12345

**Code Overview**

-Server

>server.cpp: The main server code.
Uses pthread to handle multiple games concurrently.
Manages client connections, game state, and communication.

>Key functions:
setup_listener(int portno): Sets up the server socket.
get_clients(int lis_sockfd, int *cli_sockfd): Waits for two clients to connect.
run_game(void *thread_data): Handles the game logic between two clients.

-Client

>client.cpp: The client-side code.
Connects to the server and interacts with the player.
Handles receiving updates from the server and sending player moves.

>Key functions:
connect_to_server(char *hostname, int portno): Connects to the server.
take_turn(int sockfd): Prompts the player to enter a move.
get_update(int sockfd, char board[][3]): Receives and applies updates from the server.
