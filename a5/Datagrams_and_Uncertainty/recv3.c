/* simple udp server based upon Stevens examples 
   Source: Stevens, "Unix Network Programming" */ 

#include <stdlib.h> 
#include <string.h>
#include <stdio.h>
#include <signal.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h> 

#include "shared.h"
#include "halligan.h"

#define MAXFORKS 10 
static int children=0; /* number of active children */ 

/* SIGCHLD reaper to read and discard exit codes of children */
static void reaper(int sig) {
    int stat=0;
    int pid=0; 
    /* fprintf(stderr, "called reaper for signal %d\n",sig); */ 
    if ((pid=waitpid((pid_t)-1,&stat,WNOHANG))>0) { 
	children--; 
	/* fprintf(stderr, "reaped pid %d, %d children active\n", 
		   pid, children); */ 
    } else { 
	/* fprintf(stderr, "waitpid returns %d %d\n", 
		   pid,stat); */
    } 
    signal(SIGCHLD, reaper); /* re-register reaper for forked children */ 
}

/* these routines insure that changes in the children variable are atomic 
   by blocking signals that might otherwise intervene */ 
void atomic_inc() { 	/* increment children with guaranteed atomicity */ 
    sigset_t mask; 
    sigemptyset(&(mask));
    sigaddset(&(mask),SIGCHLD);
    sigprocmask(SIG_BLOCK,&mask,NULL);
    children++; 
    sigprocmask(SIG_UNBLOCK,&mask,NULL);
} 

int atomic_read() { 	/* read value of children with guaranteed atomicity */ 
    int dummy; 
    sigset_t mask; 
    sigemptyset(&(mask));
    sigaddset(&(mask),SIGCHLD);
    sigprocmask(SIG_BLOCK,&mask,NULL);
    dummy=children; 
    sigprocmask(SIG_UNBLOCK,&mask,NULL);
    return dummy; 
} 

/* implement the server */ 
int main(int argc, char **argv)
{

/* server data */ 
    int sockfd;
    struct sockaddr_in recv_addr; 	/* server address */ 
/* message data */ 
    int mesglen; char message[MAX_MESG];
/* client data */ 
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

/* use primary address and preferred port */ 
    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family      = PF_INET;
    memcpy(&recv_addr.sin_addr,&primary,sizeof(struct in_addr)); 
    recv_addr.sin_port        = htons(recv_port);

/* bind the socket to an address and port */ 
    if (bind(sockfd, (struct sockaddr *) &recv_addr, sizeof(recv_addr))<0) 
	perror("can't bind local address"); 

    signal(SIGCHLD, reaper); /* register reaper for forked children */ 

    for ( ; ; ) {
	/* get a datagram */ 
        send_len = sizeof(send_addr); /* MUST initialize this */ 
        mesglen = recvfrom(sockfd, message, MAX_MESG, 0, 
	    (struct sockaddr *) &send_addr, &send_len);

	if (mesglen>=0) { 
	    while (atomic_read()>=MAXFORKS) { sleep(1); } 
	    int pid; 
	    if ((pid=fork())==0) { /* child */ 
		/* get numeric internet address */
		inet_ntop(PF_INET, (void *)&(send_addr.sin_addr.s_addr),  
		    send_dotted, MAX_ADDR);
		printf("server: connection from %s\n",send_dotted);

		/* convert numeric internet address to name */
		send_ulong = send_addr.sin_addr.s_addr; 
		send_hostent = gethostbyaddr((char *)&send_ulong, sizeof(send_ulong),PF_INET);
		if (send_hostent) {
		    printf("server: host name is %s\n", send_hostent->h_name);
		} else {
		    printf("server: no name for host\n");
		}

		message[mesglen]='\0'; 
		printf("server received: %s\n",message); 
		exit(0); 
	    } else { /* parent */ 
		atomic_inc(); 
		/* fprintf(stderr, "forked pid %d, %d children active\n", 
			   pid, children); */ 
	    } 
	} else if (errno != EINTR) { 
	    /* fprintf(stderr, "pid %d\n", getpid()); */ 
	    perror("receive failed"); 
	} 
    }
}
