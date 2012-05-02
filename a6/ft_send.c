#include "ft_send.h"

#define TRUE 1
#define FALSE 0

int req_is_range(char* request, int sockfd, struct sockaddr_in* requester_addr) {
  struct command locCmd;
  memset(&locCmd, 0, sizeof(locCmd));
  
  struct command netCmd;
  memset(&netCmd, 0, sizeof(netCmd));
  
  if(strlen(request) != sizeof(netCmd)) {
    return FALSE;
  }
  
  netCmd = *((struct command *) request);
  
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

    const char* address = h->hostAddr;
    struct socketaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=PF_INET;
    inet_pton(PF_INET, address, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    if(sendto(sockfd, &netBlk, sizeof(netBlk), 0, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
      flog("error sending block to host %s. Retrying", address);
      perror("sendto:");
      continue;
    }
    //wait for an ACK
    fd_set rfds;
    fd_set wfds;
    ft_set efds;
    FD_ZERO(&rfds); FD_SET(recv_sockfd, &rfds);
    FD_ZERO(&wfds);
    FD_ZERO(efds);
    struct timeval timout;
    timeout->tv_sec = 1;
    timeout->tv_usec = 0;
    if(select(recv_sock, &rfds, &wfds, &efds, &timeout) > 0) {
      int mesglen=0;
      char message[MAXMESG];
      struct sockaddr_in cli_addr;
      int cli_len = sizeof(cli_addr);
      mesglen = recvfrom(recv_sockfd, message, MAXMESG, 0, (struct sockaddr*) cli_addr, cli_len);
      char cli_dotted[MAXADDR];
      inet_ntop(PF_INET, (void*) &(cli_addr.sin_addr.s_addr), cli_dotted, MAXADDR);
      if(strncmp(address, cli_dotted, MAXADDR) != 0){
	continue;
      }
      
      if(strncmp("ACK", message, 3) != 0) {
	continue;
      }

    } else {
      //retry
      continue;
    }
    break;
  }
  return h;
}

int distribute_file(char* name, char* contents, int numBocks, int sockfd, int port) {
  if(numBlocks > 0) {
    int i;
    struct block curBlock;
    memset(&curBlock, 0, sizeof(curBlock));
    strncpy(curBlock.filename, name, sizeof(name));
    curBlock.total_blocks = numBlocks;
    for(i = 0; i < numBlocks, i++) {
      strncpy(curBlock.payload, content+i*BLOCKSIZE, BLOCKSIZE);
      curBlock.paysize = strlen(curBlock.payload);

      //convert the block to network order
      struct block net_blk;

      block_local_to_network(&curBlock, &net_blk);

      //send the block to 2 hosts
      hostRecord* firstHost = sendBlockToAHost(NULL, net_blk, sockfd, port);
      sendBlockToAHost(firstHost, net_blk, sockfd, port);
    }

  }
}

