#include "ft_send.h"

#define TRUE 1
#define FALSE 0

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


int req_is_range(char* request, int sockfd, struct sockaddr_in* requester_addr) {
  struct command locCmd;
  memset(&locCmd, 0, sizeof(locCmd));
  
  struct command netCmd;
  memset(&netCmd, 0, sizeof(netCmd));
  
  if(strncmp(request, REQUEST_RANGE_MESG, strlen(REQUEST_RANGE_MESG)) != 0) {
    return FALSE;
  }
  
  netCmd = * ((struct command *) &(request[strlen(REQUEST_RANGE_MESG)]));
  
  //get the local command
  command_network_to_local(&locCmd, &netCmd);
  
  if(strncmp(locCmd.type, "requestRange", 12) != 0) {
    return FALSE;
  }
  
  //start getting blocks and sending them back
  struct block loc_blk;
  struct block net_blk;
  struct range curRange;
  
  memset(&loc_blk, 0 , sizeof(loc_blk));
  
  strncpy(loc_blk.filename, locCmd.filename, MAXNAME);
  
  int requester_addr_len = sizeof(requester_addr);
  
  int rangeNum, curBlockNum;
  for(rangeNum = 0; rangeNum < locCmd.nranges; ++rangeNum) {
    curRange = locCmd.ranges[rangeNum];
    for(curBlockNum = curRange.first_block; curBlockNum <= curRange.last_block; curBlockNum++) {
      if(get_fblock(locCmd.filename, curBlockNum, loc_blk.payload)) {
	loc_blk.paysize = strlen(loc_blk.payload);
	loc_blk.which_block = curBlockNum;
	loc_blk.total_blocks = 0;
	block_local_to_network(&loc_blk, &net_blk);
	
	sendto(sockfd, &net_blk, sizeof(net_blk), 0, (struct sockaddr*) requester_addr, requester_addr_len);
      }
    }
  }
  return TRUE;
}

hostRecord* sendBlockToAHost(hostRecord* notThisHost, struct block netBlk, int send_sockfd, int recv_sockfd, int port) {
  hostRecord* h;
  while(TRUE) {
    do {
      h = chooseRandomHost();
    } while(h == notThisHost);

    flog("Sending block to %s\n", h->hostAddr);

    const char* address = h->hostAddr;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=PF_INET;
    inet_pton(PF_INET, address, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    char mesg[MAXMESG];
    mesg[0] = '\0';
    strncat(mesg, STORE_BLOCK_MESG, strlen(STORE_BLOCK_MESG));
    strncat(mesg, (char*) &netBlk, sizeof(netBlk));


    if(sendto(send_sockfd, mesg, strlen(mesg), 0, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
      flog("error sending block to host %s. Retrying\n", address);
      perror("sendto:");
      continue;
    }
    flog("Wating for ACK\n");
    //wait for an ACK
    fd_set rfds;
    fd_set wfds;
    fd_set efds;
    FD_ZERO(&rfds); FD_SET(recv_sockfd, &rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if(select(recv_sockfd, &rfds, &wfds, &efds, &timeout) > 0) {
      int mesglen=0;
      char message[MAXMESG];
      struct sockaddr_in cli_addr;
      int cli_len = sizeof(cli_addr);
      mesglen = recvfrom(recv_sockfd, message, MAXMESG, 0, (struct sockaddr*) &cli_addr, &cli_len);
      char cli_dotted[MAXADDR];
      inet_ntop(PF_INET, (void*) &(cli_addr.sin_addr.s_addr), cli_dotted, MAXADDR);
      if(strncmp(address, cli_dotted, MAXADDR) != 0){
	flog("Got response from wrong host\n");
	continue;
      }
      
      if(strncmp("ACK", message, 3) != 0) {
	flog("Response was not ACK\n");
	continue;
      }

    } else {
      //retry
      continue;
    }
    break;
  }
  flog("Done sending block\n");
  return h;
}

int distribute_file(char* name, char* contents, int numBlocks, int send_sockfd, int recv_sockfd, int port) {
  flog("Going to distribute %s with %d blocks", name, numBlocks);
  if(numBlocks > 0) {
    int i;
    struct block curBlock;
    memset(&curBlock, 0, sizeof(curBlock));
    strncpy(curBlock.filename, name, sizeof(name));
    curBlock.total_blocks = numBlocks;
    for(i = 0; i < numBlocks; i++) {
      strncpy(curBlock.payload, contents+i*BLOCKSIZE, BLOCKSIZE);
      curBlock.paysize = strlen(curBlock.payload);

      //convert the block to network order
      struct block net_blk;

      block_local_to_network(&curBlock, &net_blk);

      //send the block to 2 hosts
      hostRecord* firstHost = sendBlockToAHost(NULL, net_blk, send_sockfd, recv_sockfd, port);
      hostRecord* secondHost = sendBlockToAHost(firstHost, net_blk, send_sockfd, recv_sockfd, port);
      flog("Set block %d to hosts %s and %s", i, firstHost->hostAddr, secondHost->hostAddr);
    }

  }
  flog("Done distributing %s", name);
  return TRUE;
}

