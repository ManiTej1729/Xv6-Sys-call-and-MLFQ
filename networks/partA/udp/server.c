#include "udp_xoxo.h"

char board[3][3] = {{' ', ' ', ' '},
                    {' ', ' ', ' '},
                    {' ', ' ', ' '}};

char server_ip[16];

char get_sender(char response[]) {
    char temp;
    int len = strlen(response);
    for (int i = 0; i < len; i++) {
        if (response[i] == 'X' || response[i] == 'O') {
            temp = response[i];
            response[i] = '\0';
            return temp;
        }
    }
    return '\0';
}

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

void send_board(int socket, char prefix[], struct sockaddr_in to_addr, socklen_t addr_len) {
    char response[1024];
    sprintf(response, "\n-----------------\n|R\\C| 1 | 2 | 3 |\n-----------------\n| 1 | %c | %c | %c |\n-----------------\n| 2 | %c | %c | %c |\n-----------------\n| 3 | %c | %c | %c |\n-----------------\n\n%s", board[0][0], board[0][1], board[0][2],
                                                                                                                                                                                                                     board[1][0], board[1][1], board[1][2],
                                                                                                                                                                                                                     board[2][0], board[2][1], board[2][2], prefix);
    sendto(socket, response, strlen(response), 0, (struct sockaddr *) &to_addr, addr_len);
}

bool handle_post_match(int server_socket, struct sockaddr_in server_address, struct sockaddr_in p1_addr, struct sockaddr_in p2_addr) {
    // clear the board
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';
        }
    }

    socklen_t p1_len = sizeof(p1_addr);
    socklen_t p2_len = sizeof(p2_addr);

    struct sockaddr_in f1_addr;
    struct sockaddr_in f2_addr;

    socklen_t f1_len = sizeof(f1_addr);
    socklen_t f2_len = sizeof(f2_addr);

    char sender;

    // response for rematching (request is already made in start_game function)
    int num_bytes;
    char response1[10];
    char response2[10];
    num_bytes = recvfrom(server_socket, response1, sizeof(response1) - 1, 0, (struct sockaddr *) &f1_addr, &f1_len);
    response1[num_bytes] = '\0';
    printf("num_bytes = %d\n", num_bytes);

    // get the sender
    sender = get_sender(response1);
    printf("Sender: %c\n", sender);
    if (sender == 'X') {
        p1_addr = f1_addr;
    }
    else {
        p2_addr = f1_addr;
    }
    
    printf("response1: %s\n", response1);
    
    num_bytes = recvfrom(server_socket, response2, sizeof(response2) - 1, 0, (struct sockaddr *) &f2_addr, &f2_len);
    response2[num_bytes] = '\0';
    printf("num_bytes = %d\n", num_bytes);

    // get the sender
    sender = get_sender(response2);
    printf("Sender: %c\n", sender);
    if (sender == 'O') {
        p2_addr = f2_addr;
    }
    else {
        p1_addr = f2_addr;
    }
    
    printf("response2: %s\n", response2);

    if (strcasecmp(response1, "yes") == 0 && strcasecmp(response2, "yes") == 0) {
        // play again
        send_board(server_socket, "It's your turn now!\n", p1_addr, p1_len);
        send_board(server_socket, "Waiting for Player 1\n", p2_addr, p2_len);
        return true;
    }
    else if (strcasecmp(response1, "yes") == 0 && strcasecmp(response2, "no") == 0) {
        sendto(server_socket, "Sorry, the other player refused to play\n", strlen("Sorry, the other player refused to play\n"), 0, (struct sockaddr *)&f1_addr, f1_len);
    }
    else if (strcasecmp(response1, "no") == 0 && strcasecmp(response2, "yes") == 0) {
        sendto(server_socket, "Sorry, the other player refused to play\n", strlen("Sorry, the other player refused to play\n"), 0, (struct sockaddr *)&f2_addr, f2_len);
    }

    // close the connection
    close (server_socket);
    exit(0);
}

int start_game(int server_socket, struct sockaddr_in server_address, struct sockaddr_in p1_addr, struct sockaddr_in p2_addr) {

    // game logic starts here
    printf("Starting a new game\n");
    int num_bytes, row, column, win_status;
    struct sockaddr_in curr, opp;
    int num_empty_spaces = 9;
    char turn = 'X';
    char request[10];
    char buffer[1024];

    int p1_len = sizeof(p1_addr);
    int p2_len = sizeof(p2_addr);

    // loop until there is an empty space on the board
    while (num_empty_spaces > 0) {
        printf("turn: %c\n", turn);
        curr = (turn == 'X') ? p1_addr : p2_addr;
        opp = (turn == 'X')? p2_addr : p1_addr;
        socklen_t curr_len = sizeof(curr);
        socklen_t opp_len = sizeof(opp);
        num_bytes = recvfrom(server_socket, request, sizeof(request) - 1, 0, (struct sockaddr *) &curr, &curr_len);
        request[num_bytes] = '\0';
        printf("request: %s\n", request);

        // parse the request string
        sscanf(request, "%d %d\n", &row, &column);
        if (board[row - 1][column - 1] != ' ') { // if the cell is not empty
            snprintf(buffer, sizeof(buffer), "The cell (%d, %d) is already occupied\n%c", row, column, turn);
            buffer[strlen("The cell (a, b) is already occupied\nc")] = '\0';
            send_board(server_socket, buffer, curr, curr_len);
        }
        else {
            num_empty_spaces--;
            board[row - 1][column - 1] = turn;
            turn = (turn == 'X') ? 'O' : 'X';
            win_status = check_for_win_status();
            if (win_status == 0) {
                if (num_empty_spaces == 0) {
                    // its a tie
                    send_board(server_socket, "The game ended in a Tie\nWant to play again? ", curr, curr_len);
                    send_board(server_socket, "The game ended in a Tie\nWant to play again? ", opp, opp_len);
                    if (handle_post_match(server_socket, server_address, p1_addr, p2_addr)) {
                        return 1;
                    }
                    break;
                }
                else {
                    // normal move
                    snprintf(buffer, sizeof(buffer), "Waiting for your opponent...\n%c", turn);
                    buffer[strlen("Waiting for your opponent...\nc")] = '\0';
                    send_board(server_socket, buffer, curr, curr_len);

                    snprintf(buffer, sizeof(buffer), "It's your turn now!\n%c", turn);
                    buffer[strlen("It's your turn now!\nc")] = '\0';
                    send_board(server_socket, buffer, opp, opp_len);
                }
            }
            else {
                // somebody won the game
                if (win_status == 1) {
                    // X won the game
                    send_board(server_socket, "You won!\nWant to play again? ", p1_addr, p1_len);
                    send_board(server_socket, "Player X won the game!\nWant to play again? ", p2_addr, p2_len);
                    if (handle_post_match(server_socket, server_address, p1_addr, p2_addr)) {
                        return 1;
                    }
                    break;
                }
                else {
                    // O won the game
                    send_board(server_socket, "Player O won the game!\nWant to play again? ", p1_addr, p1_len);
                    send_board(server_socket, "You won!\nWant to play again? ", p2_addr, p2_len);
                    if (handle_post_match(server_socket, server_address, p1_addr, p2_addr)) {
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

    struct sockaddr_in p1_addr;
    socklen_t p1_len = sizeof(p1_addr);

    struct sockaddr_in p2_addr;
    socklen_t p2_len = sizeof(p2_addr);


    // creating a socket for server
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
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

    printf("Server is running on port %d...\n", PORT);

    // waiting for incoming connections
    // player 1 whats my status
    num_bytes = recvfrom(server_socket, request_buffer, sizeof(request_buffer), 0, (struct sockaddr *) &p1_addr, &p1_len);
    request_buffer[num_bytes] = '\0';
    printf("Player 1's request 1: %s\n", request_buffer);
    printf("Number of bytes received: %d\n", num_bytes);

    // send the response for p1
    if (strcmp(request_buffer, "What's my status?") == 0) {
        sendto(server_socket, "Welcome to TIC-TAC-TOE, you are X. Waiting for another player...\n", strlen("Welcome to TIC-TAC-TOE, you are X. Waiting for another player...\n"), 0, (struct sockaddr *) &p1_addr, p1_len);
        printf("Player 1 connected! Waiting for Player 2...\n");
    }

    // player 1 match status
    request_buffer[0] = '\0';
    num_bytes = recvfrom(server_socket, request_buffer, sizeof(request_buffer), 0, (struct sockaddr *) &p1_addr, &p1_len);
    request_buffer[num_bytes] = '\0';
    printf("Player 1's request 2: %s\n", request_buffer);
    printf("Number of bytes received: %d\n", num_bytes);

    // make it wait until p2 arrives
    if (strcmp(request_buffer, "Match status?") == 0) {
        // player 2 whats my status
        request_buffer[0] = '\0';
        num_bytes = recvfrom(server_socket, request_buffer, sizeof(request_buffer), 0, (struct sockaddr *) &p2_addr, &p2_len);
        request_buffer[num_bytes] = '\0';
        printf("Player 2's request: %s\n", request_buffer);
        printf("Number of bytes received: %d\n", num_bytes);
        
        // send the response for p2
        if (strcmp(request_buffer, "What's my status?") == 0) {
            send_board(server_socket, "Welcome to TIC-TAC-TOE, you are O. You're Player 2.\nPlayer 1 is going first.\n", p2_addr, p2_len);
            printf("Both players are here\n");
        }
        
        // send the data to player 1
        send_board(server_socket, "2 players are connected.\nIt's your turn. Go ahead!\n", p1_addr, p1_len);
    }

    // start the game
    while (1) {
        if (start_game(server_socket, server_address, p1_addr, p2_addr) == 0) {
            break;
        }
    }

    // close the socket
    close(server_socket);

    return 0;
}
