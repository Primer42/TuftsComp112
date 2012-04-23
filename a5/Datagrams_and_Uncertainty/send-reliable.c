/* simple udp server based upon Stevens examples 
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
    int sockfd;

/* message data */ 

/* sender data */ 
    struct sockaddr_in send_addr; 	/* raw send address */ 
    int send_len; 			/* length used */ 
    char send_dotted[MAX_ADDR]; 	/* string ip address */ 
    char *send_message; 

/* receiver data */ 
    struct sockaddr_in recv_addr; 	/* raw recv address */ 
    int recv_len; 			/* length used */ 
    char recv_dotted[MAX_ADDR]; 	/* string ip address */ 

/* response data */ 
    struct sockaddr_in resp_addr; 	/* raw recv address */ 
    int resp_len; 			/* length used */ 
    char resp_dotted[MAX_ADDR]; 	/* string ip address */ 
    int resp_mesglen; char resp_message[MAX_MESG]; 

/* receiver and sender share a port*/ 
    int port = 0; 

    char *send_line = NULL; 

    if (argc != 4) { 
	fprintf(stderr,"%s usage: %s address port message\n", argv[0], argv[0]);
        exit(1); 
    } 
    port=atoi(argv[2]); 
    if (port<9000 || port>32767) { 
	fprintf(stderr, "%s: port %d is not allowed\n", argv[0], port); 
	exit(1); 
    } 
    send_message = argv[3]; 

/* get the primary IP address of this host */ 
    struct in_addr primary; 
    get_primary_addr(&primary); 
    char primary_dotted[INET_ADDRSTRLEN]; 
    inet_ntop(AF_INET, &primary, primary_dotted, INET_ADDRSTRLEN);
    fprintf(stderr, "%s: Running on %s, port %d\n", argv[0],
	primary_dotted, port); 

/* make a socket*/ 
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);

/* construct an endpoint address with primary address and desired port */ 
    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family      = PF_INET;
    memcpy(&recv_addr.sin_addr,&primary,sizeof(struct in_addr)); 
    send_addr.sin_port        = htons(port);

/* bind it to an address and port on the sender */ 
    if (bind(sockfd, (struct sockaddr *) &send_addr, sizeof(send_addr))<0) { 
	perror("can't bind local address"); 
	exit(1); 
    } 

/* make an address for the receiver */ 
    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family      = PF_INET;
    inet_pton(PF_INET, argv[1], &recv_addr.sin_addr);
    recv_addr.sin_port        = htons(port);

/* send a message */ 
    sendto(sockfd, send_message, strlen(send_message), 0, 
	(struct sockaddr *)&recv_addr, sizeof(recv_addr)); 

/* wait for a response */ 
   fd_set rfds;
   struct timeval tv;
   int retval;

/* Watch sockfd to see when it has input. */
   FD_ZERO(&rfds);
   FD_SET(sockfd, &rfds);

/* Wait up to two seconds. */
   tv.tv_sec = 2; tv.tv_usec = 0;

   while ((retval = select(sockfd+1, &rfds, NULL, NULL, &tv))==0) { 
       /* Don't rely on the value of tv now! */
       /* retransmit the message */ 
       sendto(sockfd, send_message, strlen(send_message), 0, 
	    (struct sockaddr *)&recv_addr, sizeof(recv_addr)); 
       /* must re-initialize select inputs */
       FD_ZERO(&rfds);
       FD_SET(sockfd, &rfds);
       tv.tv_sec = 2; tv.tv_usec = 0;
   } 

   if (retval == -1) { 
       perror("select failed");
       exit(1); 
   } 
/* response is available: get it */ 
    resp_len = sizeof(resp_addr); /* MUST initialize this */ 
    resp_mesglen = recvfrom(sockfd, resp_message, MAX_MESG, 0, 
	(struct sockaddr *) &resp_addr, &resp_len);

    if (memcmp(&resp_addr.sin_addr,&recv_addr,sizeof(recv_addr))!=0) { 
	fprintf(stderr, "%s: different responder than recipient\n", argv[0]); 
    } 
    if (resp_mesglen>=0) { 
	/* get numeric internet address */
	inet_ntop(PF_INET, (void *)&(resp_addr.sin_addr.s_addr),  
	    resp_dotted, MAX_ADDR);
	fprintf(stderr, "%s: message from %s\n",argv[0],resp_dotted);

	resp_message[resp_mesglen]='\0'; 
	fprintf(stderr, "%s: sender received: %s\n",argv[0],resp_message); 
    } else { 
	perror("empty receive"); 
    } 
}
