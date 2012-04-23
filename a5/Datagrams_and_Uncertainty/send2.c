/* simple tcp client based upon Stevens examples 
   Source: Stevens, "Unix Network Programming" */ 

#include <stdlib.h> 
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include "shared.h"

int main(int argc, char **argv)
{
    int	sockfd;
    struct sockaddr_in	recv_addr;
    char *send_line = NULL; 
    struct hostent *host_ptr; 

    if (argc != 4) { 
	fprintf(stderr,"usage: client hostname port message\n");
        exit(1); 
    } 

    /* get the IP address of the host */
    if((host_ptr = gethostbyname(argv[1])) == NULL) {
        perror("gethostbyname error");
        exit(1);
    }
  
    if(host_ptr->h_addrtype !=  PF_INET) {
       perror("unknown address type");
       exit(1);
    }
  
    /* construct an address for the socket 
       consisting of the address and port of the host */ 
    memset((char *) &recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = PF_INET;
    /* address from gethostbyname */ 
    recv_addr.sin_addr.s_addr = 
       ((struct in_addr *)host_ptr->h_addr_list[0])->s_addr;
    /* port from command line or data */ 
    recv_addr.sin_port = htons(atoi(argv[2]));

    send_line = argv[3]; 

    sockfd = socket(PF_INET, SOCK_DGRAM, 0);

    sendto(sockfd, send_line, strlen(send_line), 0, 
	(struct sockaddr *)&recv_addr, sizeof(recv_addr)); 

}
