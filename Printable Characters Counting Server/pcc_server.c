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
#include <signal.h>
//kill -SIGUSR1 
//gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 pcc_server.c

int sigusr_flag=0;
uint32_t count[127];
int connfd=-1;

void close_server(){
    char c;
    int i;
    for(i=32; i<127; i++){
        c=(char)i;
        fprintf(stdout,"char '%c' : %u times\n", c ,count[i]);
    }
    exit(0);
}

void sigusr_handler(){
    if (connfd==-1){
        close_server();
    }
    sigusr_flag=1;
}

int sigusr_init(){
    struct sigaction sigusr;    
	sigusr.sa_handler = &sigusr_handler;
	sigemptyset(&sigusr.sa_mask);
	sigusr.sa_flags = SA_RESTART;
	if (sigaction(SIGUSR1, &sigusr, NULL) != 0) {
		return 1;
	}
    return 0;
}


void count_init(){
    int i;
    for (i=32; i<127;i++){
        count[i]=0;
    }
}

int error_handler(int reading_flag){
    if (reading_flag){
        fprintf(stderr, "\n Error : reading Failed. %s \n", strerror(errno));
    }
    else{
        fprintf(stderr, "\n Error : writing Failed. %s \n", strerror(errno));
    }
    if (errno ==ETIMEDOUT || errno== ECONNRESET || errno== EPIPE){
        return 1;
    }
    else{
        return 0;
    }
    
    
}

int main(int argc, char *argv[])
{
    int rc =-1;
    int error;
    // initiate sigusr handler
    if (sigusr_init()==1){
        fprintf(stderr, "problem with signal handle initializtion.\n");
        exit(1);
    }
    
    //puts zero in the arrey
    count_init();

    if (argc!=2){
        fprintf(stderr, "invalid number of arguments. \n");
        exit(1);
    }

    int server_port=atoi(argv[1]);
    uint32_t C=0;

    struct sockaddr_in serv_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );

    int listenfd = socket( AF_INET, SOCK_STREAM, 0 );
    memset( &serv_addr, 0, addrsize );
    
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))<0){
        fprintf(stderr,"\n Error : setsockopt Failed. %s \n", strerror(errno));
        exit (1);
    }
     serv_addr.sin_family = AF_INET;
    // INADDR_ANY = any local machine address
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(server_port);

    if( 0 != bind( listenfd, (struct sockaddr*) &serv_addr,  addrsize ) )
    {
        fprintf(stderr,"\n Error : Bind Failed. %s \n", strerror(errno));
        exit (1);
    }

    if( 0 != listen( listenfd, 10 ) )
    {
        fprintf(stderr,"\n Error : Listen Failed. %s \n", strerror(errno));
        exit(1);
    }

    while(1)
    {
    if (sigusr_flag){
        close_server();
    }
    // Accept a connection.
    connfd = accept( listenfd, (struct sockaddr*) &peer_addr,  &addrsize);
    if( connfd < 0 )
    {
      fprintf(stderr,"\n Error : Accept Failed. %s \n", strerror(errno));
      return 1;
    }
    

    //recive the number of printable characters back from the server.
    uint32_t N_net;
    char *recv_buf = (char*)&N_net;
    int remain =sizeof(N_net);

    while(remain>0){
        rc = read(connfd, recv_buf, remain);
        if (rc<0){
            error=error_handler(1);
            if (error){
                continue;
            }
            else{
                exit(1);
            }
        }
        recv_buf += rc;
        remain -= rc;
    }

    uint32_t N = ntohl(N_net);
    //allocate a buffer of size N for readind the file from the client
    
    char* buff;
    buff=malloc(N*sizeof(char));

    //read from client
    remain = N;
    int offset = 0;
    while(remain>0){
         rc = read(connfd, buff+offset, remain);
         if (rc<0){
            error=error_handler(1);
            if (error){
                continue;
            }
            else{
                exit(1);
            }
        }
        offset += rc;
        remain -= rc;
    }

     //calculate C 
    int i;
    for (i=0; i<N; i++){
        if (buff[i]>31 && buff[i]<127){
            C++;
        }
    }
    //send C back to the client
    uint32_t net_C= (htonl(C));
    char *C_buf=(char*)&net_C;
    remain = sizeof(net_C);
    while (remain > 0)
    {
        rc = write(connfd, C_buf, remain);
        if (rc<0){
            error=error_handler(0);
            if (error){
                continue;
            }
            else{
                exit(1);
            }
        }
        C_buf+=rc; 
        remain-=rc;
    }

        
    //update the data structure 
    for (i=0; i<N; i++){
        if (buff[i]>31 && buff[i]<127){
            count[(int)buff[i]]++;
        }
    }

    //were done with the data. free the buffer.
    free(buff);

    close(connfd);
    //restart the varibles for next iteration. 
    connfd=-1;
    C=0; 
  }
}


