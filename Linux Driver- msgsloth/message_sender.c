#include "message_slot.h"

#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc!=4){
        fprintf(stderr, "wrong number of arguments!\n");
		exit(1);
    }
    unsigned long channel_id = atoi(argv[2]);
    char* msg= argv[3];
    int msg_size=strlen(msg);

    int file_desc=open(argv[1], O_RDWR);
    if (file_desc<0){
        fprintf(stderr, "open file failed\n.");
		exit(1);
    }

    if (ioctl(file_desc, MSG_SLOT_CHANNEL, channel_id)!=0){
        fprintf(stderr, "ioctl failed\n.");
		exit(1);
    }

    if (write(file_desc, msg, msg_size)!=msg_size){
        fprintf(stderr, "write failed\n.");
		exit(1);
    }

    close(file_desc);
    exit(0);
}