// this is a UDP file service client based upon 
// a simple file completion strategy

/*=============================
  Starting solution for Assignment 2: UDP file transfer
  the intent of this file is to demonstrate how to 
  call the helper routines. It will help you avoid some
  common errors. IT DOES NOT SOLVE THE PROBLEM. 
  =============================*/ 

#include "help.h" 

#include <sys/stat.h>

#define TRUE  1
#define FALSE 0

int main(int argc, char **argv)
{

/* local client data */ 
    int sockfd;		   		/* file descriptor for endpoint */ 
    struct sockaddr_in client_sockaddr;	/* address/port pair */ 
    struct in_addr client_addr; 	/* network-format address */ 
    char client_dotted[INET_ADDRSTRLEN];/* human-readable address */ 
    int client_port; 			/* local port */ 

/* remote server data */ 
    char *server_dotted; 		/* human-readable address */ 
    int server_port; 			/* remote port */ 

/* the request */ 
    char *filename; 			/* filename to request */ 

/* read arguments */ 
    if (argc != 5) { 
	fprintf(stderr, "client: wrong number of arguments\n"); 
	client_usage(); 
        exit(1); 
    } 

    server_dotted = argv[1]; 
    server_port = atoi(argv[2]); 
    client_port = atoi(argv[3]); 
    filename = argv[4]; 

    if (!client_arguments_valid(
	server_dotted, server_port, 
	client_port, filename)) { 
	    client_usage(); 
	    exit(1); 
    } 

/* get the primary IP address of this host */ 
    get_primary_addr(&client_addr); 
    inet_ntop(AF_INET, &client_addr, client_dotted, INET_ADDRSTRLEN);

/* construct an endpoint address with primary address and desired port */ 
    memset(&client_sockaddr, 0, sizeof(client_sockaddr));
    client_sockaddr.sin_family      = PF_INET;
    memcpy(&client_sockaddr.sin_addr,&client_addr,sizeof(struct in_addr)); 
    client_sockaddr.sin_port        = htons(client_port);

/* make a socket*/ 
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd<0) { 
	perror("can't open socket"); 
	exit(1); 
    } 

/* bind it to an appropriate local address and port */ 
    if (bind(sockfd, (struct sockaddr *) &client_sockaddr, 
	     sizeof(client_sockaddr))<0) { 
	perror("can't bind local address"); 
	exit(1); 
    } 
    fprintf(stderr, "client: Receiving on %s, port %d\n", 
	client_dotted, client_port); 

/* send a command */ 
    send_command(sockfd, server_dotted, server_port, filename, 0, MAXINT); 

    fprintf(stderr, "client: requesting %s blocks %d-%d\n", 
	filename, 0, MAXINT); 

    //make a bit array to keep track of which blocks we have reiceved
    struct bits blocksNeeded;
    int bitArrayAllocd = FALSE;

    //open the file we're asking for
    char clientFilePath[FILENAMESIZE];
    strncat(clientFilePath, "./", 2);
    strncat(clientFilePath, filename, strlen(filename));
    int outFd;
    if((outFd = open(clientFilePath, O_RDWR | O_CREAT, 0666)) < 0) {
      perror("Unable to create client file");
      exit(2);
    }

    int done = FALSE;
/* receive the whole document and make naive assumptions */ 
    //finish when the bit array has been set and it is empty
    //i.e all blocks of the file have been marked as recieved
    while (!done) {
      int retval; 
    again: 
      if(done) {
	break;
      }
      
      if ((retval = select_block(sockfd, 1, 0))==0) { 
        /* timeout */ 
	fprintf(stderr, "Timeout\n");
	//check if we've started getting blocks already
	if(!bitArrayAllocd) {
	  //we've never gotten a responce
	  //bad news
	  fprintf(stderr, "Timeout without recieving any blocks");
	  exit(1);
	} else {
	  //send a request for the blocks we need
	  //do this badly for now - would need to rewrite
	  //send_command to do better
	  //baby steps
	  int block;
	  for(block = 0; block < blocksNeeded.nbits; ++block) {
	    if(bits_testbit(&blocksNeeded, block)) {
	      //still need this block - request it
	      send_command(sockfd, server_dotted, server_port, filename, block, block);
	    }
	  }
	}
	
      } else if (retval<0) { 
	/* error */ 
	perror("select"); 
	fprintf(stderr, "client: receive error\n"); 
      } else { 
	/* input is waiting, read it */ 
	struct sockaddr_in resp_sockaddr; 	/* address/port pair */ 
	int resp_len; 			/* length used */ 
	char resp_dotted[INET_ADDRSTRLEN]; 	/* human-readable address */ 
	int resp_port; 			/* port */ 
	int resp_mesglen; 			/* length of message */ 
	struct block one_block; 
	
	/* use helper routine to receive a block */ 
	recv_block(sockfd, &one_block, &resp_sockaddr);
	
	/* get human-readable internet address */
	inet_ntop(AF_INET, (void *)&(resp_sockaddr.sin_addr.s_addr),  
		  resp_dotted, INET_ADDRSTRLEN);
	resp_port = ntohs(resp_sockaddr.sin_port); 
	
	fprintf(stderr, "client: %s:%d sent %s block %d (range 0-%d)\n",
		resp_dotted, resp_port, one_block.filename, 
		one_block.which_block, one_block.total_blocks);
	
	/* check block data for errors */
	if (strcmp(filename, one_block.filename)!=0) { 
	  fprintf(stderr, 
		  "client: received block with incorrect filename %s\n", 
		  one_block.filename); 
	  goto again; 
	}
	
	//first, make the bit array if it has not already been made
	if(!bitArrayAllocd) {
	  bits_alloc(&blocksNeeded, one_block.total_blocks);
	  bits_clearall(&blocksNeeded);
	  bits_setrange(&blocksNeeded, 0, one_block.total_blocks-1);
	  bitArrayAllocd = TRUE;
	}
	
	//next, write out the block
	lseek(outFd, one_block.which_block*PAYLOADSIZE, SEEK_SET);
	write(outFd, one_block.payload, one_block.paysize);
	
	//finally, mark the block as recieved
	bits_clearbit(&blocksNeeded, one_block.which_block);	
      }
      //see if we're done
      done = bits_empty(&blocksNeeded);
    }
    close(outFd);
}

