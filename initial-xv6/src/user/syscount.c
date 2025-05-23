#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(2, "Usage: syscount <mask> <command>\n");
        exit(1);
    }

    int mask_str = 0;
    int len = strlen(argv[1]);
    for (int i = 0; i < len; i++) {
        if (argv[1][i] <= '9' && argv[1][i] >= '0') {
            mask_str = mask_str * 10 + (argv[1][i] - '0');
        }
    }
    
    // resolving the mask
    int mask = 0;
    while (mask_str > 1) {
        mask++;
        mask_str /= 2;
    }
    
    // int i = 0;
    // while (argv[i]) {
    //     printf("%s\n", argv[i++]);
    // }
    // printf("mask in user mode: %d\n", mask);
    
    // create a new process for the rest of the process
    int child_pid = fork();
    
    if (child_pid < 0) {
        printf("Error: fork failed\n");
        return -1;
    }
    else if (child_pid == 0) {
        // update the mask
        updateMask(mask);

        // execute the command
        exec(argv[2], &argv[2]);
        printf("Error: exec failed\n");
        exit(1);
    }
    else {
        // parent process
        wait(0);
        getSysCount(mask);
    }

    return 0;
}
