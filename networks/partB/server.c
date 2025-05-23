#include "udp_tcp.h"

int server_socket, num_bytes;
struct sockaddr_in client_addr;
socklen_t client_addr_len = sizeof(client_addr);

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

char inc_msg[MAX_MSG_SIZE];
char out_msg[MAX_MSG_SIZE];

void split_data_into_chunks(data_packet data[], int num_chunks, char out_msg[]) {
    char buffer[CHUNK_SIZE + 1];
    for (int i = 0; i < num_chunks; i++) {
        // Split the message into chunks
        strncpy(buffer, out_msg + (i * CHUNK_SIZE), CHUNK_SIZE);
        buffer[CHUNK_SIZE] = '\0';

        // Populate the data packet
        i = i % num_chunks;
        data[i].seq_no = i % num_chunks;
        data[i].num_chunks = num_chunks;
        strncpy(data[i].chunk, buffer, CHUNK_SIZE);
    }
}

void *delayed_ack(void *args_void) {
    // retrieve the args struct
    args_struct *args = (args_struct *)args_void;

    int num_chunks = args -> num_chunks;
    bool return_condition = true;

    // open the error log file
    FILE *fptr = fopen("./serverlog.txt", "a+");

    // lets start the logic
    while (1) {
        usleep(100000); // wait for 0.1s for the acknowledgement
        
        return_condition = true;
        
        // check for the condition to stop
        for (int i = 0; i < num_chunks; i++) {
            i = i % num_chunks;
            // printf("i: %d, ith ack: %d\n", i, args -> ack_arr[i]);
            if (args -> ack_arr[i] != 1) {
                fprintf(fptr, "Missing acknowledgement for chunk %d. Hence resending it again.\n", i);

                // dont return
                return_condition = false;

                // send that particular packet again
                sendto(server_socket, &args -> data[i], STRUCT_CHUNK_SIZE, 0, (struct sockaddr *)&client_addr, client_addr_len);
            }
        }

        if (return_condition) {
            break;
        }
    }

    fclose(fptr);
    return NULL;
}

void *handle_ack(void *args_void) {
    // retrieve the args struct
    args_struct *args = (args_struct *)args_void;
    pthread_t delayed_ack_thread;

    // printf("data packet: seq_no = %d, num_chunks = %d, string = %s\n", args -> data[0].seq_no, args -> num_chunks, args -> data[0].chunk);
    
    int ack_packet_idx;
    char buffer_ack[100];
    int num_chunks = args -> num_chunks;

    pthread_create(&delayed_ack_thread, NULL, delayed_ack, (void *)args);
    // printf("num_chunks = %d\n", num_chunks);
    for (int i = 0; i < num_chunks; i++) {
        // receive acknowledgement
        num_bytes = recvfrom(server_socket, buffer_ack, sizeof(buffer_ack) - 1, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        buffer_ack[num_bytes] = '\0';
        // printf("received acknowledgement: %s\n", buffer_ack);

        pthread_mutex_lock(&lock);
        if (strncmp("ACK", buffer_ack, 3) == 0) {
            // get the index which client / server acknowledged
            sscanf(buffer_ack, "ACK <%d>", &ack_packet_idx);

            // update the acknowledgement array
            args -> ack_arr[ack_packet_idx % num_chunks] = 1;
        }
        else {
            printf("Well this is not supposed to happen <%s> <%d>\n", buffer_ack, num_bytes);
        }
        pthread_mutex_unlock(&lock);
    }
    // join the thread
    pthread_join(delayed_ack_thread, NULL);
    return NULL;
}

void handle_sendto() {
    pthread_t recv_ack_thread;
    data_packet buffer_data;
    int num_chunks;

    // define the arguments for thread function
    data_packet *data;
    int *ack_arr; // ack_arr[i] = 1 represents that acknowledgement for data packet data[i] is received
    
    // get the input
    printf("\033[0m\n\033[34mChat with Client: \033[0m\033[32m");
    fgets(out_msg, sizeof(out_msg), stdin);
    printf("\033[0m");
    out_msg[strlen(out_msg) - 1] = '\0';

    // calculate number of chunks
    num_chunks = strlen(out_msg) / CHUNK_SIZE;
    if (strlen(out_msg) % CHUNK_SIZE != 0) {
        num_chunks++;
    }

    // allocate memory to those arguments
    data = (data_packet *)malloc(num_chunks * sizeof(data_packet));
    ack_arr = (int *)malloc(num_chunks * sizeof(int));

    // initialize the array of acknowledgements
    for (int i = 0; i < num_chunks; i++) {
        ack_arr[i] = 0; // initially nothing is sent => nothing to acknowledge
    }

    // create chunks
    split_data_into_chunks(data, num_chunks, out_msg);

    // create args struct
    args_struct *args = (args_struct *)malloc(sizeof(args));
    args -> data = data;
    args -> ack_arr = ack_arr;
    args -> num_chunks = num_chunks;

    // send it to the client
    pthread_create(&recv_ack_thread, NULL, handle_ack, (void *)args); // thread for handling acknowledgements
    
    // send the data in small chunks
    for (int i = 0; i < num_chunks; i++) {
        buffer_data = data[i];
        // printf("sending chunk: %s\n", buffer_data.chunk);
        sendto(server_socket, &buffer_data, STRUCT_CHUNK_SIZE, 0, (struct sockaddr *)&client_addr, client_addr_len);
    }
    
    pthread_join(recv_ack_thread, NULL);
    // handle_ack((void *)args);

    // free everything
    free(args -> data);
    free(args -> ack_arr);
    free(args);

    if (strcmp(out_msg, "bye") == 0) {
        // close all the sockets
        close(server_socket);
        printf("\n\n\033[31mExiting...\033[0m\n");
        exit(0);
    }

    return;
}

void handle_recvfrom() {
    printf("\n\nWaiting for client to send message\n");
    data_packet buffer_data;
    int max_chunks = (MAX_MSG_SIZE / CHUNK_SIZE) + 1;
    data_packet storage[max_chunks];
    int num_chunks = 1;
    int data_len = 0;
    int break_flag = 0;
    char ack_string[100];
    
    data_len = 0;
    inc_msg[0] = '\0';

    int num_misses = 0;
    bool miss_prank = true; // set to false if you don't want to skip acks
    
    for (int i = 0; i < num_chunks; i++) {
        num_bytes = recvfrom(server_socket, &buffer_data, sizeof(buffer_data) - 1, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        num_chunks = buffer_data.num_chunks;
        // printf("received chunk: %s\n", buffer_data.chunk);

        if (i % 3 != 0 || !miss_prank) {
            // send the acknowledgement for the chunk
            sprintf(ack_string, "ACK <%d>", buffer_data.seq_no % num_chunks);
            storage[buffer_data.seq_no % num_chunks] = buffer_data; // store it
            // printf("sending acknowledgement: %s\n", ack_string);
            sendto(server_socket, ack_string, strlen(ack_string), 0, (struct sockaddr *)&client_addr, client_addr_len);
        }
        else {
            num_misses++;
        }
    }

    for (int i = 0; i < num_misses; i++) {
        num_bytes = recvfrom(server_socket, &buffer_data, sizeof(buffer_data) - 1, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        num_chunks = buffer_data.num_chunks;
        // printf("received chunk missed: %s\n", buffer_data.chunk);

        // send the acknowledgement for the chunk
        sprintf(ack_string, "ACK <%d>", buffer_data.seq_no % num_chunks);
        storage[buffer_data.seq_no % num_chunks] = buffer_data; // store it
        // printf("sending acknowledgement missed: %s\n", ack_string);
        sendto(server_socket, ack_string, strlen(ack_string), 0, (struct sockaddr *)&client_addr, client_addr_len);
    }

    // generate the strings from chunks received
    inc_msg[0] = '\0';
    for (int i = 0; i < num_chunks; i++) {
        // printf("stored chunk: %s\n", storage[i % num_chunks].chunk);
        strncat(inc_msg, storage[i % num_chunks].chunk, CHUNK_SIZE);
    }

    printf("\n\n\033[33mFrom Client: %s\033[0m\n\n", inc_msg);
    if (strcmp(inc_msg, "bye") == 0) {
        // close all the sockets
        close(server_socket);
        printf("\n\n\033[31mChat with Client has ended\033[0m\n");
        exit(0);
    }
    fflush(stdout);
    return;
}

int main() {

    // clear serverlog.txt file
    FILE *fp;
    fp = fopen("./serverlog.txt", "w");
    fclose(fp);

    // creating a socket for server
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1) {
        printf("Could not create socket\n");
        return -1;
    }

    // binding the socket
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
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

    // first let the client connect
    num_bytes = recvfrom(server_socket, inc_msg, sizeof(inc_msg), 0, (struct sockaddr *)&client_addr, &client_addr_len);
    inc_msg[num_bytes] = '\0';
    printf("We got a client.\n");

    // send the acknowledgement
    sendto(server_socket, "Oh hello client! lets talk..", sizeof("Oh hello client! lets talk.."), 0, (struct sockaddr *)&client_addr, client_addr_len);
    
    // while loop for server assuming that server woulg send the data first
    while (1) {
        handle_sendto();
        handle_recvfrom();
    }

    // closing the socket
    close(server_socket);
    pthread_mutex_destroy(&lock);
    
    return 0;
}
