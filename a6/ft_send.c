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
  
  netCmd = * ((struct command *) request);
  
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
