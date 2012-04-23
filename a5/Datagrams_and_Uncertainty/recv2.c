/* simple udp server based upon Stevens examples with Halligan hall extensions
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

/* server data */ 
    int sockfd;
    struct sockaddr_in recv_addr; 	/* server address */ 
/* message data */ 
    int mesglen; char message[MAX_MESG];
/* sender data */ 
    struct sockaddr_in send_addr; 	/* raw client address */ 
    int send_len; 			/* length used */ 
    char send_dotted[MAX_ADDR]; 	/* string ip address */ 
    unsigned long send_ulong; 		/* packed ip address */ 
    struct hostent *send_hostent; 	/* host entry */ 
    int recv_port = 0; 

    if (argc != 2) { 
	fprintf(stderr, "%s usage: %s port\n", argv[0], argv[0]); 
	exit(1);
    } 
    recv_port=atoi(argv[1]); 
    if (recv_port<9000 || recv_port>32767) { 
	fprintf(stderr, "%s: port %d is not allowed\n", argv[0], recv_port); 
	exit(1); 
    } 
/* get the primary IP address of this host */ 
    struct in_addr primary; 
    get_primary_addr(&primary); 
    char primary_dotted[INET_ADDRSTRLEN]; 
    inet_ntop(AF_INET, &primary, primary_dotted, INET_ADDRSTRLEN);
    fprintf(stderr, "%s: Running on %s, port %d\n", argv[0],
	primary_dotted, recv_port); 

/* make a socket*/ 
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family      = PF_INET;
    memcpy(&recv_addr.sin_addr,&primary,sizeof(struct in_addr)); 
    recv_addr.sin_port        = htons(recv_port);

/* bind it to an address and port */ 
    if (bind(sockfd, (struct sockaddr *) &recv_addr, sizeof(recv_addr))<0) 
	perror("can't bind local address"); 

    for ( ; ; ) {
	/* get a datagram */ 
        send_len = sizeof(send_addr); /* MUST initialize this */ 
        mesglen = recvfrom(sockfd, message, MAX_MESG, 0, 
	    (struct sockaddr *) &send_addr, &send_len);

	if (mesglen>=0) { 
	    /* get numeric internet address */
	    inet_ntop(PF_INET, (void *)&(send_addr.sin_addr.s_addr),  
		send_dotted, MAX_ADDR);
	    fprintf(stderr,"%s: connection from %s\n",argv[0],send_dotted);

	    /* convert numeric internet address to name */
	    send_ulong = send_addr.sin_addr.s_addr; 
	    send_hostent = gethostbyaddr((char *)&send_ulong, sizeof(send_ulong),PF_INET);
	    if (send_hostent) {
		fprintf(stderr,"server: host name is %s\n", send_hostent->h_name);
	    } else {
		fprintf(stderr,"server: no name for host\n");
	    }

	    message[mesglen]='\0'; 
	    fprintf(stderr,"server received: %s\n",message); 
	} else { 
	    perror("receive failed"); 
	} 
    }
}
