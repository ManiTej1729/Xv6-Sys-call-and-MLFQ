#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#define PORT 8080
#define MAX_MSG_SIZE 4096
#define CHUNK_SIZE 5 // decrease this value while testing

char server_ip[] = "127.0.0.1";

typedef struct data_packet {
    int seq_no;
    int num_chunks;
    char chunk[CHUNK_SIZE + 1];
} data_packet;

#define STRUCT_CHUNK_SIZE sizeof(data_packet)

typedef struct args_struct {
    data_packet *data;
    int *ack_arr;
    int num_chunks;
} args_struct;
