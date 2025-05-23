#include "tcp_xoxo.h"

char server_ip[16];

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

    int socket1, num_bytes;
    char piece;

    // create socket
    socket1 = socket(AF_INET, SOCK_STREAM, 0);
    if (socket1 == -1) {
        printf("Could not create socket\n");
        return -1;
    }

    // server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    // server_address.sin_addr.s_addr = INADDR_ANY;
    if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    // connect to the server
    if (connect(socket1, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        printf("Connection failed\n");
        return -1;
    }

    // ask for the lobby status
    send(socket1, "What's my status?\n", sizeof("What's my status\n"), 0);

    // receive the lobby status
    char response[1024];
    response[0] = '\0';
    num_bytes = recv(socket1, response, sizeof(response), 0);
    response[num_bytes] = '\0';
    printf("%s", response);

    if (strcmp(response, "Welcome to TIC-TAC-TOE, you are X. Waiting for another player...\n") == 0) {
        // this is player 1. request for match status
        send(socket1, "Match status?\n", sizeof("Match status?\n"), 0);

        // receive the match status
        response[0] = '\0';
        num_bytes = recv(socket1, response, sizeof(response), 0);
        response[num_bytes] = '\0';
        printf("%s", response);
        piece = 'X';
    }
    else {
        // this is player 2 (board is already received)
        piece = 'O';
    }

    int row, column;
    char turn = 'X';
    char rematch[4];
    char move[10];
    while (1) {
        if (piece == turn) {
            printf("Enter <row> <column> where you want to place your piece: ");
            scanf("%d %d", &row, &column);
            if (row < 1 || row > 3) {
                printf("Invalid row. Please enter a number between 1 and 3.\n");
                continue;
            }
            if (column < 1 || column > 3) {
                printf("Invalid column. Please enter a number between 1 and 3.\n");
                continue;
            }

            // create a new request and send it to the server
            snprintf(move, 4, "%d %d\n", row, column);
            send(socket1, move, 4, 0);

            // receive the verdict
            response[0] = '\0';
            num_bytes = recv(socket1, response, sizeof(response), 0);
            response[num_bytes] = '\0';
            if (response[num_bytes - 1] == 'X' || response[num_bytes - 1] == 'O') {
                turn = response[num_bytes - 1];
                response[num_bytes - 1] = '\0';
            }
            printf("%s", response);
            if (response[num_bytes - 1] == ' ') {
                // rematch request
                scanf("%s", rematch);

                // validate the command
                if (strcasecmp(rematch, "yes") == 0 || strcasecmp(rematch, "no") == 0) {
                    // send the rematch response
                    send(socket1, rematch, strlen(rematch), 0);
                    if (strcasecmp(rematch, "no") == 0) {
                        printf("Thanks for playing\n");
                        break;
                    }

                    // if i say yes, i should get a response
                    response[0] = '\0';
                    num_bytes = recv(socket1, response, sizeof(response), 0);
                    response[num_bytes] = '\0';
                    printf("%s", response);
                    if (strcmp(response, "Sorry, the other player refused to play\n") == 0) {
                        printf("Anyways, Thanks for playing\n");
                        break;
                    }
                    else { // agreed for rematch
                        turn = 'X';
                        continue;
                    }
                }
                else {
                    break;
                }
            }
        }
        else {
            // wait for response
            response[0] = '\0';
            num_bytes = recv(socket1, response, sizeof(response), 0);
            response[num_bytes] = '\0';
            if (response[num_bytes - 1] == 'X' || response[num_bytes - 1] == 'O') {
                turn = response[num_bytes - 1];
                response[num_bytes - 1] = '\0';
            }
            printf("%s", response);

            if (response[num_bytes - 1] == ' ') {
                // rematch request
                scanf("%s", rematch);

                // validate the command
                if (strcasecmp(rematch, "yes") == 0 || strcasecmp(rematch, "no") == 0) {
                    
                    // send the rematch response
                    send(socket1, rematch, strlen(rematch), 0);
                    if (strcasecmp(rematch, "no") == 0) {
                        printf("Thanks for playing\n");
                        break;
                    }

                    // if i say yes, i should get a response
                    response[0] = '\0';
                    num_bytes = recv(socket1, response, sizeof(response), 0);
                    response[num_bytes] = '\0';
                    printf("%s", response);
                    if (strcmp(response, "Sorry, the other player refused to play\n") == 0) {
                        printf("Anyways, Thanks for playing\n");
                        break;
                    }
                    else { // agreed for rematch
                        turn = 'X';
                        continue;
                    }
                }
                else {
                    break;
                }
            }
        }
    }

    close(socket1);

    return 0;
}
