// this is a UDP file service client based upon 
// a simple file completion strategy

/*=============================
  William Richard (wrichard)
  Comp112 a2
  
  My code is pretty simple.
  It makes a bit array to keep track of which blocks it has recieved.
  It asks for more blocks either if there is a timeout (i.e. it has
  exhausted the buffer) or if it has recieved 75% of the blocks
  it last requested.
  I found that this was about the optimal number while I was on the
  server alone.
  It does NOT impliment the extra credit.
  =============================*/ 

#include "ft_recv.h"


#define MAXOUT 256 		/* maximum number of output chars for flog */ 
static void flog(const char *fmt, ...) {
    va_list ap;
    char *p; 
    char buf[MAXOUT]; 
    va_start(ap, fmt);
    fprintf(stderr,"[ft_recv: "); 
    vsnprintf(buf,MAXOUT,fmt,ap); 
    for (p=buf; *p && p-buf<MAXOUT; p++) 
	if ((*p)>=' ') 
	    putc(*p,stderr);
 
    fprintf(stderr,"]\n"); 
    va_end(ap);
} 


int req_is_block_to_store(char* request, int send_sockfd, struct sockaddr_in* sender_addr) {
  struct block locBlk;
  struct block netBlk;
  memset(&locBlk, 0, sizeof(locBlk));
  memset(&netBlk, 0, sizeof(netBlk));

  if(strncmp(request, STORE_BLOCK_MESG, strlen(STORE_BLOCK_MESG)) != 0) {
    return FALSE;
  }

  netBlk = * ((struct block *) &(request[strlen(STORE_BLOCK_MESG)]));

  block_network_to_local(&locBlk, &netBlk);

  int n = next_cblock();
  if(n >=0) {
    if(remember_fblock(locBlk.filename, locBlk.which_block, locBlk.payload)) {
      //ack it
      char* ack = "ACK";
      sendto(send_sockfd, ack, sizeof(ack), 0, (struct sockaddr*) sender_addr, sizeof(sender_addr));
    }
  } else {
    flog("no more cache!\n");
    //remove oldest blocks?
  }

  return TRUE;
}


int numBlocksNeeded = MAXINT;
int numBlocksRecieved = 0;

//This sends the command that is passed to it
//deals with all the network order stuff so the function that makes 
//the command doesn't have to
void sendPremadeCommand(int sockfd, int port, struct command loc_cmd) {
  fprintf(stderr, "sendPremadeCommand with %d ranges\n", loc_cmd.nranges);
  int i;
  for(i = 0; i < MAX_STORED_HOSTS; ++i) {
    hostRecord* curHost = getHostRecordAt(i);
    if(curHost == NULL){
      continue;
    }
    const char* address = curHost->hostAddr;
    struct command net_cmd;
    struct sockaddr_in server_addr;
    //translate the command into network order
    command_local_to_network(&loc_cmd, &net_cmd);
    //set up target address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = PF_INET;
    inet_pton(PF_INET, address, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);
    //send the packet
    char mesg[MAXMESG];
    mesg[0] = '\0';
    strncat(mesg, REQUEST_RANGE_MESG, strlen(REQUEST_RANGE_MESG));
    strncat(mesg, (char*) &net_cmd, COMMAND_SIZE(loc_cmd.nranges));

    int ret = sendto(sockfd, (void*) mesg, strlen(mesg), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (ret<0) perror ("sendPremadeCommand");
  }
}

/* This method tests if we should make another request.
   i.e. if we have recieved 75% of the packets we last asked for.
*/
int makeRequest() {
  int limit = numBlocksNeeded * REQUEST_PERCENTAGE + 1;
  return numBlocksRecieved == limit;
}

/* This function requests the missing blocks
   in as few 'command' packets as possible.
   It looks through the bit array, looking for ranges of missing blocks
   Once it has created 12 ranges, it sends a command.
   At the end, if it has any ranges left over, it sends them.
*/
void requestMissingBlocks(int sockfd, int port, const char* filename, struct bits *blocksNeeded) {
  struct command cmd;
  //set up cmd structur
  memset(&cmd, 0, sizeof(cmd));
  strcpy(cmd.filename, filename);
  int constructingRange = FALSE;
  cmd.nranges = 0;
  strncpy(cmd.type, "requestRange", 12);
  int block;

  numBlocksRecieved = 0;
  numBlocksNeeded = 0;

  for(block = 0; block < blocksNeeded->nbits; ++block) {
    if(constructingRange) {
      //in the middle of constructing a range - look for the end of the range
      if(!bits_testbit(blocksNeeded, block)) {
	//found the end of the range
	cmd.ranges[cmd.nranges].last_block = block - 1;
	numBlocksNeeded += cmd.ranges[cmd.nranges].last_block + 1 - cmd.ranges[cmd.nranges].first_block;
	cmd.nranges += 1;
	//check to see if we have too many ranges and need to send the command
	if(cmd.nranges == MAXRANGES) {
	  sendPremadeCommand(sockfd, port, cmd);
	  cmd.nranges = 0;
	}
	constructingRange = FALSE;
      }
    } else {
      //not currently constructing a range - look for the start of a range
      if(bits_testbit(blocksNeeded, block)) {
	//found the start of a range
	cmd.ranges[cmd.nranges].first_block = block;
	constructingRange = TRUE;
      }
    }
  }
  //if we're currently making a range, finish it
  if(constructingRange) {
    cmd.ranges[cmd.nranges].last_block = blocksNeeded->nbits;
    cmd.nranges += 1;
  }
  //see if we have any ranges to send
  if(cmd.nranges) {
    sendPremadeCommand(sockfd, port, cmd);
  }
}

void request_file(char* filename, int port, int sockfd, int numBlocks, char** contents) {
  //start by requesting the file from all known hosts
  int i;
  for(i = 0; i < MAX_STORED_HOSTS; ++i) {
    hostRecord* curHost = getHostRecordAt(i);
    if(curHost != NULL) {
      send_command(sockfd, curHost->hostAddr, port, filename, 0, numBlocks);
    }
  }

  //make a bit array to keep track of which blocks we have reiceved
  struct bits blocksNeeded;
  bits_alloc(&blocksNeeded, numBlocks);
  bits_clearall(&blocksNeeded);
  bits_setrange(&blocksNeeded, 0, numBlocks-1);

  numBlocksNeeded = numBlocks;
  numBlocksRecieved = 0;

  int done = FALSE;

  //recieve the file
  //finish when every bit in the bit array has been set
  //i.e. all blocks have been marked as received

  while(!done) {
    int retval;
  again:
    if(done) {
      break;
    }

    if((retval = select_block(sockfd, 1, 0)) == 0) {
      //timeout
      requestMissingBlocks(sockfd, port, filename, &blocksNeeded);
    } else if(retval < 0) {
      perror("select_block");
    } else if(makeRequest()) {
      requestMissingBlocks(sockfd, port, filename, &blocksNeeded);
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

      //save the block
      strncpy(*contents+one_block.which_block*BLOCKSIZE, one_block.payload, BLOCKSIZE);

      //see if the block has been received already
      if (bits_testbit(&blocksNeeded, one_block.which_block)) {
	//increment our counter if it's a new block
	++numBlocksRecieved;
      }
      
      //finally, mark the block as recieved
      bits_clearbit(&blocksNeeded, one_block.which_block);
      
    }
    //see if we're done
    done = bits_empty(&blocksNeeded);
  }
}



/* int main_old(int argc, char **argv) */
/* { */

/* /\* local client data *\/  */
/*     int sockfd;		   		/\* file descriptor for endpoint *\/  */
/*     struct sockaddr_in client_sockaddr;	/\* address/port pair *\/  */
/*     struct in_addr client_addr; 	/\* network-format address *\/  */
/*     char client_dotted[INET_ADDRSTRLEN];/\* human-readable address *\/  */
/*     int client_port; 			/\* local port *\/  */

/* /\* remote server data *\/  */
/*     char *server_dotted; 		/\* human-readable address *\/  */
/*     int server_port; 			/\* remote port *\/  */

/* /\* the request *\/  */
/*     char *filename; 			/\* filename to request *\/  */

/* /\* read arguments *\/  */
/*     if (argc != 5) {  */
/* 	fprintf(stderr, "client: wrong number of arguments\n");  */
/* 	client_usage();  */
/*         exit(1);  */
/*     }  */

/*     server_dotted = argv[1];  */
/*     server_port = atoi(argv[2]);  */
/*     client_port = atoi(argv[3]);  */
/*     filename = argv[4];  */

/*     if (!client_arguments_valid( */
/* 	server_dotted, server_port,  */
/* 	client_port, filename)) {  */
/* 	    client_usage();  */
/* 	    exit(1);  */
/*     }  */

/* /\* get the primary IP address of this host *\/  */
/*     get_primary_addr(&client_addr);  */
/*     inet_ntop(AF_INET, &client_addr, client_dotted, INET_ADDRSTRLEN); */

/* /\* construct an endpoint address with primary address and desired port *\/  */
/*     memset(&client_sockaddr, 0, sizeof(client_sockaddr)); */
/*     client_sockaddr.sin_family      = PF_INET; */
/*     memcpy(&client_sockaddr.sin_addr,&client_addr,sizeof(struct in_addr));  */
/*     client_sockaddr.sin_port        = htons(client_port); */

/* /\* make a socket*\/  */
/*     sockfd = socket(PF_INET, SOCK_DGRAM, 0); */
/*     if (sockfd<0) {  */
/* 	perror("can't open socket");  */
/* 	exit(1);  */
/*     }  */

/* /\* bind it to an appropriate local address and port *\/  */
/*     if (bind(sockfd, (struct sockaddr *) &client_sockaddr,  */
/* 	     sizeof(client_sockaddr))<0) {  */
/* 	perror("can't bind local address");  */
/* 	exit(1);  */
/*     }  */
/*     fprintf(stderr, "client: Receiving on %s, port %d\n",  */
/* 	client_dotted, client_port);  */

/* /\* send a command *\/  */
/*     send_command(sockfd, server_dotted, server_port, filename, 0, MAXINT);  */

/*     fprintf(stderr, "client: requesting %s blocks %d-%d\n",  */
/* 	filename, 0, MAXINT);  */

/*     //make a bit array to keep track of which blocks we have reiceved */
/*     struct bits blocksNeeded; */
/*     int bitArrayAllocd = FALSE; */

/*     //open the file we're asking for */
/*     char clientFilePath[MAXNAME]; */
/*     strncat(clientFilePath, "./", 2); */
/*     strncat(clientFilePath, filename, strlen(filename)); */
/*     int outFd; */
/*     if((outFd = open(clientFilePath, O_RDWR | O_CREAT, 0666)) < 0) { */
/*       perror("Unable to create client file"); */
/*       exit(2); */
/*     } */

/*     int done = FALSE; */
/* /\* receive the whole document and make naive assumptions *\/  */
/*     //finish when the bit array has been set and it is empty */
/*     //i.e all blocks of the file have been marked as recieved */
/*     while (!done) { */
/*       int retval;  */
/*     again:  */
/*       if(done) { */
/* 	break; */
/*       } */
      
/*       if((retval = select_block(sockfd, 1, 0))==0) {  */
/*         /\* timeout *\/  */
/* 	fprintf(stderr, "Timeout\n"); */
/* 	//check if we've started getting blocks already */
/* 	if(!bitArrayAllocd) { */
/* 	  //we've never gotten a responce */
/* 	  //bad news */
/* 	  fprintf(stderr, "Timeout without recieving any blocks\n"); */
/* 	  exit(1); */
/* 	} else { */
/* 	  requestMissingBlocks(sockfd, server_dotted, server_port, filename, &blocksNeeded); */
/* 	} */
	
/*       } else if (retval<0) {  */
/* 	/\* error *\/  */
/* 	perror("select");  */
/* 	fprintf(stderr, "client: receive error\n");  */
/*       } else if (makeRequest()) { */
/* 	requestMissingBlocks(sockfd, server_dotted, server_port, filename, &blocksNeeded); */
/*       } else {  */
/* 	/\* input is waiting, read it *\/  */
/* 	struct sockaddr_in resp_sockaddr; 	/\* address/port pair *\/  */
/* 	int resp_len; 			/\* length used *\/  */
/* 	char resp_dotted[INET_ADDRSTRLEN]; 	/\* human-readable address *\/  */
/* 	int resp_port; 			/\* port *\/  */
/* 	int resp_mesglen; 			/\* length of message *\/  */
/* 	struct block one_block;  */
	
/* 	/\* use helper routine to receive a block *\/  */
/* 	recv_block(sockfd, &one_block, &resp_sockaddr); */
	
/* 	/\* get human-readable internet address *\/ */
/* 	inet_ntop(AF_INET, (void *)&(resp_sockaddr.sin_addr.s_addr),   */
/* 		  resp_dotted, INET_ADDRSTRLEN); */
/* 	resp_port = ntohs(resp_sockaddr.sin_port);  */
	
/* 	fprintf(stderr, "client: %s:%d sent %s block %d (range 0-%d)\n", */
/* 		resp_dotted, resp_port, one_block.filename,  */
/* 		one_block.which_block, one_block.total_blocks); */
	
/* 	/\* check block data for errors *\/ */
/* 	if (strcmp(filename, one_block.filename)!=0) {  */
/* 	  fprintf(stderr,  */
/* 		  "client: received block with incorrect filename %s\n",  */
/* 		  one_block.filename);  */
/* 	  goto again;  */
/* 	} */
	
/* 	//first, make the bit array if it has not already been made */
/* 	if(!bitArrayAllocd) { */
/* 	  bits_alloc(&blocksNeeded, one_block.total_blocks); */
/* 	  bits_clearall(&blocksNeeded); */
/* 	  bits_setrange(&blocksNeeded, 0, one_block.total_blocks-1);	   */
/* 	  bitArrayAllocd = TRUE; */
/* 	  numBlocksNeeded = one_block.total_blocks; */
/* 	  numBlocksRecieved = 0; */
/* 	} */
	
/* 	//next, write out the block */
/* 	lseek(outFd, one_block.which_block*BLOCKSIZE, SEEK_SET); */
/* 	write(outFd, one_block.payload, one_block.paysize); */

/* 	//see if the block has been recieved already */
/* 	if (bits_testbit(&blocksNeeded, one_block.which_block)) { */
/* 	  //increment our counter if it's a new block */
/* 	  ++numBlocksRecieved; */
/* 	} */

/* 	//finally, mark the block as recieved */
/* 	bits_clearbit(&blocksNeeded, one_block.which_block); */
/*       } */
/*       //see if we're done */
/*       done = bits_empty(&blocksNeeded); */
/*     } */
/*     close(outFd); */
/* } */

