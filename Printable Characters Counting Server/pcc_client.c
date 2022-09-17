#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/sendfile.h>


int main(int argc, char *argv[])
{
    int remain, sockfd, rc, offset;

    if (argc!=4){
        fprintf(stderr, "invalid number of arguments. \n");
        exit(1);
    }
    
    char* server_ip=argv[1];
    int server_port=atoi(argv[2]);
    char* file_path=argv[3];


    //open the file
    FILE *f;
    if ((f = fopen(file_path,"r")) == NULL){
       fprintf(stderr,"Error with opening file.\n");
       exit(1);
    }

    //determinate N
    fseek(f, 0, SEEK_END); 
    uint32_t N = ftell(f); 
    fseek(f, 0, SEEK_SET); 

    char * data=malloc(N*sizeof(char));

    if (data== NULL){
		fprintf(stderr, "malloc failed: %s\n", strerror(errno));
        exit(1);
	}
    //this is a buffer that holding the data from the file.
    if (fread(data, 1, N, f) != N){
		fprintf(stderr, "Error with file reading: %s\n", strerror(errno));
        exit(1);
	}


    struct sockaddr_in serv_addr; // where we Want to get to

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr,"Error : Could not create socket. Error:  %s\n", strerror(errno));
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    inet_pton(AF_INET, server_ip, &serv_addr.sin_addr.s_addr);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port); 
    
    if( connect(sockfd,(struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "\n Error : Connect Failed. %s \n", strerror(errno));
        exit(1);

    }
    
    // Sending the number N
    uint32_t net_N= (htonl(N));
    char *buf=(char*)&net_N;
    remain = sizeof(net_N);
    while (remain > 0)
    {   
        rc = write(sockfd, buf, remain);
        if (rc<0){
            fprintf(stderr, "\n Error : writing Failed. %s \n", strerror(errno));
            exit(1);
        }
        buf+=rc; 
        remain-=rc;
    }
    //Sending the file data 
    offset = 0;
    remain = N;

    while(remain>0){
        rc = write(sockfd, data+offset, remain);
        if (rc<0){
            fprintf(stderr, "\n Error : writing Failed. %s \n", strerror(errno));
            exit(1);
        }
        offset+=rc; 
        remain-=rc;
    }

    //read C from the server
    uint32_t C_net;
    char *recv_buf = (char*)&C_net;
    remain = sizeof(C_net);
    while(remain>0){
        rc = read(sockfd, recv_buf, remain);
        if (rc<0){
            fprintf(stderr, "\n Error : reading Failed. %s \n", strerror(errno));
            exit(1);
        }
        recv_buf += rc;
        remain -= rc;
    }
    uint32_t C = ntohl(C_net);

    close(sockfd); 
    fclose(f);
    fprintf(stdout,"# of printable characters: %u\n", C);
    exit(0);
}