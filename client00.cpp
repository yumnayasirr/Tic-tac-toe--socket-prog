#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

void error(const char *msg)
{
    perror(msg);
    cout << "Either the server shut down or the other player disconnected.\nGame over.\n";
    exit(0);
}

void recv_msg(int sockfd, string &msg)
{
    char buffer[4] = {0};
    int n = read(sockfd, buffer, 3);
    
    if (n < 0 || n != 3)
        error("ERROR reading message from server socket.");

    msg = buffer;
    cout << "[DEBUG] Received message: " << msg << endl;
}

int recv_int(int sockfd)
{
    int msg = 0;
    int n = read(sockfd, &msg, sizeof(int));
    
    if (n < 0 || n != sizeof(int)) 
        error("ERROR reading int from server socket");
    
    cout << "[DEBUG] Received int: " << msg << endl;
    
    return msg;
}

void write_server_int(int sockfd, int msg)
{
    int n = write(sockfd, &msg, sizeof(int));
    if (n < 0)
        error("ERROR writing int to server socket");
    
    cout << "[DEBUG] Wrote int to server: " << msg << endl;
}

int connect_to_server(char *hostname, int portno)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
 
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
    if (sockfd < 0) 
        error("ERROR opening socket for server.");
	
    server = gethostbyname(hostname);
	
    if (server == NULL) {
        cerr << "ERROR, no such host" << endl;
        exit(0);
    }
	
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memmove(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portno); 

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting to server");

    cout << "[DEBUG] Connected to server." << endl;
    return sockfd;
}

void draw_board(char board[][3])
{
    cout << " " << board[0][0] << " | " << board[0][1] << " | " << board[0][2] << " " << endl;
    cout << "-----------" << endl;
    cout << " " << board[1][0] << " | " << board[1][1] << " | " << board[1][2] << " " << endl;
    cout << "-----------" << endl;
    cout << " " << board[2][0] << " | " << board[2][1] << " | " << board[2][2] << " " << endl;
}

void take_turn(int sockfd)
{
    string buffer;
    
    while (true) {
        cout << "Enter 0-8 to make a move, or 9 for number of active players: ";
        getline(cin, buffer);
        int move = buffer[0] - '0';
        if (move <= 9 && move >= 0) {
            cout << endl;
            write_server_int(sockfd, move);   
            break;
        } else {
            cout << "\nInvalid input. Try again." << endl;
        }
    }
}

void get_update(int sockfd, char board[][3])
{
    int player_id = recv_int(sockfd);
    int move = recv_int(sockfd);
    board[move / 3][move % 3] = player_id ? 'X' : 'O';    
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        cerr << "usage " << argv[0] << " hostname port" << endl;
        exit(0);
    }

    int sockfd = connect_to_server(argv[1], atoi(argv[2]));
    int id = recv_int(sockfd);

#ifdef DEBUG
    cout << "[DEBUG] Client ID: " << id << endl;
#endif 

    string msg;
    char board[3][3] = { {' ', ' ', ' '}, 
                         {' ', ' ', ' '}, 
                         {' ', ' ', ' '} };

    cout << "Tic-Tac-Toe\n------------" << endl;

    do {
        recv_msg(sockfd, msg);
        if (msg == "HLD")
            cout << "Waiting for a second player..." << endl;
    } while (msg != "SRT");

    /* The game has begun. */
    cout << "Game on!" << endl;
    cout << "You are " << (id ? 'X' : 'O') << "'s" << endl;

    draw_board(board);

    while (true) {
        recv_msg(sockfd, msg);

        if (msg == "TRN") {
            cout << "Your move..." << endl;
            take_turn(sockfd);
        } else if (msg == "INV") {
            cout << "That position has already been played. Try again." << endl; 
        } else if (msg == "CNT") {
            int num_players = recv_int(sockfd);
            cout << "There are currently " << num_players << " active players." << endl; 
        } else if (msg == "UPD") {
            get_update(sockfd, board);
            draw_board(board);
        } else if (msg == "WAT") {
            cout << "Waiting for other player's move..." << endl;
        } else if (msg == "WIN") {
            cout << "You win!" << endl;
            break;
        } else if (msg == "LSE") {
            cout << "You lost." << endl;
            break;
        } else if (msg == "DRW") {
            cout << "Draw." << endl;
            break;
        } else {
            error("Unknown message.");
        }
    }

    cout << "Game over." << endl;
    close(sockfd);
    return 0;
}

