#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

using namespace std;

int player_count = 0;
pthread_mutex_t mutexcount;

void error(const char *msg)
{
    perror(msg);
    pthread_exit(NULL);
}

void write_client_int(int cli_sockfd, int msg)
{
    int n = write(cli_sockfd, &msg, sizeof(int));
    if (n < 0)
        error("ERROR writing int to client socket");
}

void write_client_msg(int cli_sockfd, const char *msg)
{
    int n = write(cli_sockfd, msg, strlen(msg));
    if (n < 0)
        error("ERROR writing msg to client socket");
}

void write_clients_msg(int *cli_sockfd, const char *msg)
{
    write_client_msg(cli_sockfd[0], msg);
    write_client_msg(cli_sockfd[1], msg);
}

void write_clients_int(int *cli_sockfd, int msg)
{
    write_client_int(cli_sockfd[0], msg);
    write_client_int(cli_sockfd[1], msg);
}

int setup_listener(int portno)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening listener socket.");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR binding listener socket.");

    #ifdef DEBUG
    cout << "[DEBUG] Listener set." << endl;
    #endif

    return sockfd;
}

int recv_int(int cli_sockfd)
{
    int msg = 0;
    int n = read(cli_sockfd, &msg, sizeof(int));
    if (n < 0 || n != sizeof(int))
        return -1;

    cout << "[DEBUG] Received int: " << msg << endl;
    return msg;
}

void get_clients(int lis_sockfd, int *cli_sockfd)
{
    socklen_t clilen;
    struct sockaddr_in cli_addr;

    #ifdef DEBUG
    cout << "[DEBUG] Listening for clients..." << endl;
    #endif

    int num_conn = 0;
    while (num_conn < 2)
    {
        listen(lis_sockfd, 253 - player_count);

        memset(&cli_addr, 0, sizeof(cli_addr));
        clilen = sizeof(cli_addr);

        cli_sockfd[num_conn] = accept(lis_sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if (cli_sockfd[num_conn] < 0)
            error("ERROR accepting a connection from a client.");

        #ifdef DEBUG
        cout << "[DEBUG] Accepted connection from client " << num_conn << endl;
        #endif

        write(cli_sockfd[num_conn], &num_conn, sizeof(int));

        #ifdef DEBUG
        cout << "[DEBUG] Sent client " << num_conn << " its ID." << endl;
        #endif

        pthread_mutex_lock(&mutexcount);
        player_count++;
        cout << "Number of players is now " << player_count << "." << endl;
        pthread_mutex_unlock(&mutexcount);

        if (num_conn == 0)
        {
            write_client_msg(cli_sockfd[0], "HLD");

            #ifdef DEBUG
            cout << "[DEBUG] Told client 0 to hold." << endl;
            #endif
        }

        num_conn++;
    }
}

int get_player_move(int cli_sockfd)
{
    #ifdef DEBUG
    cout << "[DEBUG] Getting player move..." << endl;
    #endif

    write_client_msg(cli_sockfd, "TRN");
    return recv_int(cli_sockfd);
}

int check_move(char board[][3], int move, int player_id)
{
    if ((move == 9) || (board[move / 3][move % 3] == ' '))
    {
        #ifdef DEBUG
        cout << "[DEBUG] Player " << player_id << "'s move was valid." << endl;
        #endif
        return 1;
    }
    else
    {
        #ifdef DEBUG
        cout << "[DEBUG] Player " << player_id << "'s move was invalid." << endl;
        #endif
        return 0;
    }
}

void update_board(char board[][3], int move, int player_id)
{
    board[move / 3][move % 3] = player_id ? 'X' : 'O';

    #ifdef DEBUG
    cout << "[DEBUG] Board updated." << endl;
    #endif
}

void draw_board(char board[][3])
{
    cout << " " << board[0][0] << " | " << board[0][1] << " | " << board[0][2] << " " << endl;
    cout << "-----------" << endl;
    cout << " " << board[1][0] << " | " << board[1][1] << " | " << board[1][2] << " " << endl;
    cout << "-----------" << endl;
    cout << " " << board[2][0] << " | " << board[2][1] << " | " << board[2][2] << " " << endl;
}

void send_update(int *cli_sockfd, int move, int player_id)
{
    #ifdef DEBUG
    cout << "[DEBUG] Sending update..." << endl;
    #endif

    write_clients_msg(cli_sockfd, "UPD");
    write_clients_int(cli_sockfd, player_id);
    write_clients_int(cli_sockfd, move);

    #ifdef DEBUG
    cout << "[DEBUG] Update sent." << endl;
    #endif
}

void send_player_count(int cli_sockfd)
{
    write_client_msg(cli_sockfd, "CNT");
    write_client_int(cli_sockfd, player_count);

    #ifdef DEBUG
    cout << "[DEBUG] Player Count Sent." << endl;
    #endif
}

int check_board(char board[][3], int last_move)
{
    #ifdef DEBUG
    cout << "[DEBUG] Checking for a winner..." << endl;
    #endif

    int row = last_move / 3;
    int col = last_move % 3;

    if (board[row][0] == board[row][1] && board[row][1] == board[row][2])
    {
        #ifdef DEBUG
        cout << "[DEBUG] Win by row " << row << "." << endl;
        #endif
        return 1;
    }
    else if (board[0][col] == board[1][col] && board[1][col] == board[2][col])
    {
        #ifdef DEBUG
        cout << "[DEBUG] Win by column " << col << "." << endl;
        #endif
        return 1;
    }
    else if (!(last_move % 2))
    {
        if ((last_move == 0 || last_move == 4 || last_move == 8) && (board[1][1] == board[0][0] && board[1][1] == board[2][2]))
        {
            #ifdef DEBUG
            cout << "[DEBUG] Win by backslash diagonal." << endl;
            #endif
            return 1;
        }
        if ((last_move == 2 || last_move == 4 || last_move == 6) && (board[1][1] == board[0][2] && board[1][1] == board[2][0]))
        {
            #ifdef DEBUG
            cout << "[DEBUG] Win by frontslash diagonal." << endl;
            #endif
            return 1;
        }
    }

    #ifdef DEBUG
    cout << "[DEBUG] No winner, yet." << endl;
    #endif
    return 0;
}

void *run_game(void *thread_data)
{
    int *cli_sockfd = (int *)thread_data;
    char board[3][3] = {{' ', ' ', ' '},
                        {' ', ' ', ' '},
                        {' ', ' ', ' '}};

    cout << "Game on!" << endl;

    write_clients_msg(cli_sockfd, "SRT");

    #ifdef DEBUG
    cout << "[DEBUG] Sent start message." << endl;
    #endif

    draw_board(board);

    int prev_player_turn = 1;
    int player_turn = 0;
    int game_over = 0;
    int turn_count = 0;
    while (!game_over)
    {
        if (prev_player_turn != player_turn)
            write_client_msg(cli_sockfd[(player_turn + 1) % 2], "WAT");

        int valid = 0;
        int move = 0;
        while (!valid)
        {
            move = get_player_move(cli_sockfd[player_turn]);
            if (move == -1)
                break;
            cout << "Player " << player_turn << " played position " << move << endl;

            valid = check_move(board, move, player_turn);
            if (!valid)
            {
                cout << "Move was invalid. Let's try this again..." << endl;
                write_client_msg(cli_sockfd[player_turn], "INV");
            }
        }

        if (move == -1)
        {
            cout << "Player disconnected." << endl;
            break;
        }
        else if (move == 9)
        {
            prev_player_turn = player_turn;
            send_player_count(cli_sockfd[player_turn]);
        }
        else
        {
            update_board(board, move, player_turn);
            send_update(cli_sockfd, move, player_turn);

            draw_board(board);

            game_over = check_board(board, move);

            if (game_over == 1)
            {
                write_client_msg(cli_sockfd[player_turn], "WIN");
                write_client_msg(cli_sockfd[(player_turn + 1) % 2], "LSE");
                cout << "Player " << player_turn << " won." << endl;
            }
            else if (turn_count == 8)
            {
                cout << "Draw." << endl;
                write_clients_msg(cli_sockfd, "DRW");
                game_over = 1;
            }

            prev_player_turn = player_turn;
            player_turn = (player_turn + 1) % 2;
            turn_count++;
        }
    }

    cout << "Game over." << endl;

    close(cli_sockfd[0]);
    close(cli_sockfd[1]);

    pthread_mutex_lock(&mutexcount);
    player_count--;
    cout << "Number of players is now " << player_count << "." << endl;
    player_count--;
    cout << "Number of players is now " << player_count << "." << endl;
    pthread_mutex_unlock(&mutexcount);

    free(cli_sockfd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "ERROR, no port provided" << endl;
        exit(1);
    }

    int lis_sockfd = setup_listener(atoi(argv[1]));
    pthread_mutex_init(&mutexcount, NULL);

    while (1)
    {
        if (player_count <= 252)
        {
            int *cli_sockfd = (int *)malloc(2 * sizeof(int));
            memset(cli_sockfd, 0, 2 * sizeof(int));

            get_clients(lis_sockfd, cli_sockfd);

            #ifdef DEBUG
            cout << "[DEBUG] Starting new game thread..." << endl;
            #endif

            pthread_t thread;
            int result = pthread_create(&thread, NULL, run_game, (void *)cli_sockfd);
            if (result)
            {
                cout << "Thread creation failed with return code " << result << endl;
                exit(-1);
            }

            cout << "[DEBUG] New game thread started." << endl;
        }
    }

    close(lis_sockfd);
    pthread_mutex_destroy(&mutexcount);
    pthread_exit(NULL);
}

