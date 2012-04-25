#include <stdio.h> 
#include <stdarg.h> 
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <stdlib.h> 
#include <unistd.h> 

#include "storage.h"
#include "operations.h"
#include "nose.h"

#define TRUE 1
#define FALSE 0

/* logging of server actions */ 
#define MAXOUT 256 		/* maximum number of output chars for flog */ 
static void flog(const char *fmt, ...) {
    va_list ap;
    char *p; 
    char buf[MAXOUT]; 
    va_start(ap, fmt);
    fprintf(stderr,"[operations: "); 
    vsnprintf(buf,MAXOUT,fmt,ap); 
    for (p=buf; *p && p-buf<MAXOUT; p++) 
	if ((*p)>=' ') 
	    putc(*p,stderr); 
    fprintf(stderr,"]\n"); 
    va_end(ap);
} 

/* called when udp datagram available on a socket 
 * socket: number of socket */ 
void udp(int sockfd) { 
    /* client data */
    struct sockaddr_in cli_addr;        /* raw client address */
    int cli_len;                        /* length used */
    char cli_dotted[MAXADDR];           /* string ip address */
    unsigned long cli_ulong;            /* packed ip address */
    struct hostent *cli_hostent;        /* host entry */
    /* message parameters */ 
    char message[MAXMESG]; 		/* message to be read */ 
    int mesglen=0; 			/* message length */ 

    flog("udp datagram available on socket %d\n",sockfd); 

    /* get a datagram */
    cli_len = sizeof(cli_addr); /* MUST initialize this */
    mesglen = recvfrom(sockfd, message, MAXMESG, 0,
        (struct sockaddr *) &cli_addr, &cli_len);
    /* get numeric internet address */
    inet_ntop(PF_INET, (void *)&(cli_addr.sin_addr.s_addr),
	cli_dotted, MAXADDR);
    flog("udp connection from %s\n",cli_dotted);

    /* convert numeric internet address to name */
    cli_ulong = cli_addr.sin_addr.s_addr;
    cli_hostent = gethostbyaddr((char *)&cli_ulong, 
	    	sizeof(cli_ulong),PF_INET);
    if (cli_hostent) {
	flog("udp host name is %s\n", cli_hostent->h_name);
    } else {
	flog("no name for udp host\n");
    }
    message[mesglen]='\0'; // moot point; makes it a string if possible
    flog("message is '%s'",message); 
} 

/* get a file from storage;
 * name: name of file (in local machine)
 * content: an array of content (result parameter)
 * size: size of file */ 
int get(char *name, char **content, int *size) { 
    if (get_size(name)>=0) {  
	int blocks; 
	*size=get_size(name); 
	blocks = (*size%BLOCKSIZE==0?*size/BLOCKSIZE:*size/BLOCKSIZE+1); 
	if (blocks>0) { 
	    *content= (char *)malloc(BLOCKSIZE*blocks); 
	    int i;
	    for (i=0; i<blocks; i++) 
		get_fblock(name,i,*content+i*BLOCKSIZE); 
        } else { 
	    *content = (char *)malloc(1*sizeof(char)); 
	    **content=0; 
	} 
	return TRUE; 
    } else { 
	return FALSE; 
    } 
} 

/* put a file into storage. 
 * name: local name of file. 
 * content: a character array of content. 
 * size: size of local file. */ 
int put(char *name, char *content, int size) { 
    int blocks; 
    remember_size(name,size); 
    blocks = (size%BLOCKSIZE==0?size/BLOCKSIZE:size/BLOCKSIZE+1); 
    if (blocks>0) { 
	int i; 
	for (i=0; i<blocks; i++) { 
	    int n = next_cblock(); 
	    if (n>=0) { 
		remember_fblock(name,i,content+i*BLOCKSIZE); 
	    } else { 
		flog("no more cache!\n"); 
		return FALSE; 
	    } 
	} 
    } 
    return TRUE; 
} 

/* delete a file from storage 
 * name: name of file (on one machine) */ 
int del(char *name)  {
    if ((get_size(name))>=0) { 
	forget_size(name); 
	forget_file(name);
	return TRUE; 
    } else { 
	return FALSE; 
    } 
} 


