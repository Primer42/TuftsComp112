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

    if (argc != 4) { 
	fprintf(stderr,"%s usage: %s address port message\n", argv[0], argv[0]);
        exit(1); 
    } 
    /* set target address */ 
    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = PF_INET;
    inet_pton(PF_INET, argv[1], &recv_addr.sin_addr);
    recv_addr.sin_port = htons(atoi(argv[2]));
    send_line = argv[3]; 

    sockfd = socket(PF_INET, SOCK_DGRAM, 0);

    sendto(sockfd, send_line, strlen(send_line), 0, 
	(struct sockaddr *)&recv_addr, sizeof(recv_addr)); 

}
