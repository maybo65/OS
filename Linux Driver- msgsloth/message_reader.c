#include "message_slot.h"
//tail -f /var/log/kern.log
#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc!=3){
        fprintf(stderr, "wrong number of arguments!\n");
		exit(1);
    }
    char* file_path= argv[1];
    unsigned long channel_id = atoi(argv[2]);
    char buffer[MAX_BUF_SIZE];

    int file_desc=open(file_path, O_RDWR);
    if (file_desc<0){
        fprintf(stderr, "open file failed.");
		exit(1);
    }

    if (ioctl(file_desc, MSG_SLOT_CHANNEL, channel_id)!=0){
        fprintf(stderr, "ioctl failed.");
		exit(1);
    }
    int msg_size=read(file_desc, buffer, MAX_BUF_SIZE);

    if (msg_size<1){
        fprintf(stderr, "read failed.");
		exit(1);
    }

    close(file_desc);

    if (write(1, buffer, msg_size) != msg_size) {
		fprintf(stderr, "print out message failed Error: %s\n", strerror(errno));
		exit(1);
	}
    
    exit(0);
}