#include "tcp_xoxo.h"

char board[3][3] = {{' ', ' ', ' '},
                    {' ', ' ', ' '},
                    {' ', ' ', ' '}};

char server_ip[16];

int check_for_win_status() {

    // for row
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ') {
            return board[i][0] == 'X'? 1 : 2;
        }
    }

    // for column
    for (int i = 0; i < 3; i++) {
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ') {
            return board[0][i] == 'X'? 1 : 2;
        }
    }

    // for diagonal 1
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ') {
        return board[0][0] == 'X'? 1 : 2;
    }

    // for diagonal 2
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ') {
        return board[0][2] == 'X'? 1 : 2;
    }

    return 0;
}

void send_board(int socket, char prefix[]) {
    char response[1024];
    sprintf(response, "\n-----------------\n|R\\C| 1 | 2 | 3 |\n-----------------\n| 1 | %c | %c | %c |\n-----------------\n| 2 | %c | %c | %c |\n-----------------\n| 3 | %c | %c | %c |\n-----------------\n\n%s", board[0][0], board[0][1], board[0][2],
                                                                                                                                                                                                                     board[1][0], board[1][1], board[1][2],
                                                                                                                                                                                                                     board[2][0], board[2][1], board[2][2], prefix);
    send(socket, response, strlen(response), 0);
}

bool handle_post_match(int p1_socket, int p2_socket, int server_socket, struct sockaddr_in server_address) {
    
    // clear the board
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';
        }
    }

    // response for rematching (request is already made in start_game function)
    int num_bytes;
    char response1[10];
    char response2[10];
    num_bytes = recv(p1_socket, response1, sizeof(response1) - 1, 0);
    response1[num_bytes] = '\0';
    num_bytes = recv(p2_socket, response2, sizeof(response2) - 1, 0);
    response2[num_bytes] = '\0';
    printf("response1: %s\n", response1);
    printf("response2: %s\n", response2);

    if (strcasecmp(response1, "yes") == 0 && strcasecmp(response2, "yes") == 0) {
        // play again
        send_board(p1_socket, "It's your turn now!\n");
        send_board(p2_socket, "Waiting for Player 1\n");
        return true;
    }
    else if (strcasecmp(response1, "yes") == 0 && strcasecmp(response2, "no") == 0) {
        send(p1_socket, "Sorry, the other player refused to play\n", strlen("Sorry, the other player refused to play\n"), 0);
    }
    else if (strcasecmp(response1, "no") == 0 && strcasecmp(response2, "yes") == 0) {
        send(p2_socket, "Sorry, the other player refused to play\n", strlen("Sorry, the other player refused to play\n"), 0);
    }

    // close the connection
    close(p1_socket);
    close(p2_socket);
    close (server_socket);
    exit(0);
}

int start_game(int p1_socket, int p2_socket, int server_socket, struct sockaddr_in server_address) {
    
    // game logic starts here
    printf("Starting a new game\n");
    int num_bytes, row, column, win_status, curr_player_sock, opponent_player_sock;
    int num_empty_spaces = 9;
    char turn = 'X';
    char request[10];
    char buffer[1024];

    // loop until there is an empty space on the board
    while (num_empty_spaces > 0) {
        printf("turn: %c\n", turn);
        curr_player_sock = (turn == 'X') ? p1_socket : p2_socket;
        opponent_player_sock = (turn == 'X') ? p2_socket : p1_socket;
        num_bytes = recv(curr_player_sock, request, sizeof(request) - 1, 0);
        request[num_bytes] = '\0';
        printf("request: %s\n", request);

        // parse the request string
        sscanf(request, "%d %d\n", &row, &column);
        if (board[row - 1][column - 1] != ' ') { // if the cell is not empty
            snprintf(buffer, sizeof(buffer), "The cell (%d, %d) is already occupied\n%c", row, column, turn);
            buffer[strlen("The cell (a, b) is already occupied\nc")] = '\0';
            send_board(curr_player_sock, buffer);
        }
        else {
            num_empty_spaces--;
            board[row - 1][column - 1] = turn;
            turn = (turn == 'X') ? 'O' : 'X';
            win_status = check_for_win_status();
            if (win_status == 0) {
                if (num_empty_spaces == 0) {
                    // its a tie
                    send_board(curr_player_sock, "The game ended in a Tie\nWant to play again? ");
                    send_board(opponent_player_sock, "The game ended in a Tie\nWant to play again? ");
                    if (handle_post_match(p1_socket, p2_socket, server_socket, server_address)) {
                        return 1;
                    }
                    break;
                }
                else {
                    // normal move
                    snprintf(buffer, sizeof(buffer), "Waiting for your opponent...\n%c", turn);
                    buffer[strlen("Waiting for your opponent...\nc")] = '\0';
                    send_board(curr_player_sock, buffer);

                    snprintf(buffer, sizeof(buffer), "It's your turn now!\n%c", turn);
                    buffer[strlen("It's your turn now!\nc")] = '\0';
                    send_board(opponent_player_sock, buffer);
                }
            }
            else {
                // somebody won the game
                if (win_status == 1) {
                    // X won the game
                    send_board(p1_socket, "You won!\nWant to play again? ");
                    send_board(p2_socket, "Player X won the game!\nWant to play again? ");
                    if (handle_post_match(p1_socket, p2_socket, server_socket, server_address)) {
                        return 1;
                    }
                    break;
                }
                else {
                    // O won the game
                    send_board(p1_socket, "Player O won the game!\nWant to play again? ");
                    send_board(p2_socket, "You won!\nWant to play again? ");
                    if (handle_post_match(p1_socket, p2_socket, server_socket, server_address)) {
                        return 1;
                    }
                    break;
                }
            }
        }
    }

    return 0;
}

bool validate_ip(char *ip_addr) {
    int first, second, third, fourth;
    sscanf(ip_addr, "%d.%d.%d.%d", &first, &second, &third, &fourth);
    if (first > 255 || second > 255 || third > 255 || fourth > 255) {
        printf("Invalid IP address.\n");
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        // if no ip address is specified in the command line, assign localhost
        strcpy(server_ip, "127.0.0.1");
    }
    else {
        if (argc > 2) {
            printf("Usage: ./executable <server-ip>");
            exit(0);
        }
        
        // validate the given ip address
        if (validate_ip(argv[1])) {
            strcpy(server_ip, argv[1]);
        }
    }

    int num_bytes;
    char request_buffer[1024];
    request_buffer[0] = '\0';

    // creating a socket for server
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        printf("Could not create socket\n");
        return -1;
    }

    // binding the socket
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    // server_address.sin_addr.s_addr = INADDR_ANY;
    if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    socklen_t addr_len = sizeof(server_address);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        printf("Failed to bind socket\n");
        return -1;
    }

    // listening for incoming connections
    if (listen(server_socket, 2) < 0) {
        printf("Failed to listen\n");
        return -1;
    }

    printf("Server is listening on port %d...\n", PORT);

    int p1_socket, p2_socket;

    // accepting a new connection
    p1_socket = accept(server_socket, (struct sockaddr *)& server_address, &addr_len);
    if (p1_socket < 0) {
        printf("Failed to accept connection\n");
        return -1;
    }
    printf("Player 1 connected! Waiting for Player 2\n");

    // deal with player 1
    num_bytes = recv(p1_socket, request_buffer, sizeof(request_buffer), 0);
    request_buffer[num_bytes] = '\0';
    printf("Player 1's request: %s\n", request_buffer);
    if (strcmp(request_buffer, "What's my status?\n") == 0) {
        send(p1_socket, "Welcome to TIC-TAC-TOE, you are X. Waiting for another player...\n", strlen("Welcome to TIC-TAC-TOE, you are X. Waiting for another player...\n"), 0);
    }

    p2_socket = accept(server_socket, (struct sockaddr *)&server_address, &addr_len);
    if (p2_socket < 0) {
        printf("Failed to accept connection\n");
        return -1;
    }
    printf("Player 2 connected!\n");

    // deal with player 2
    request_buffer[0] = '\0';
    num_bytes = recv(p2_socket, request_buffer, sizeof(request_buffer) - 1, 0);
    request_buffer[num_bytes] = '\0';
    printf("Player 2's request: %s\n", request_buffer);
    if (strcmp(request_buffer, "What's my status?\n") == 0) {
        send_board(p2_socket, "Welcome to TIC-TAC-TOE, you are O. You're Player 2.\nPlayer 1 is going first.\n");
    }

    // send a message to player 1 that the match is ready
    request_buffer[0] = '\0';
    num_bytes = recv(p1_socket, request_buffer, sizeof(request_buffer) - 1, 0);
    request_buffer[num_bytes] = '\0';
    printf("Player 1's request: %s\n", request_buffer);
    if (strcmp(request_buffer, "Match status?\n") == 0) {
        send_board(p1_socket, "2 players are connected.\nIt's your turn. Go ahead!\n");
    }

    // start the game
    while (1) {
        if (start_game(p1_socket, p2_socket, server_socket, server_address) == 0) {
            break;
        }
    }

    // close the socket
    close(server_socket);
    close(p1_socket);
    close(p2_socket);

    return 0;
}
