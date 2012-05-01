#include "ft_send.h"

#include "storage.h"
#include "help.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>


#define TRUE 1
#define FALSE 0


void req_is_range(char* request, int sockfd struct sockaddr_in requester_addr) {
  if(strncmp(request, "requestRange", 12)) {
    struct command locCmd;
    memset(&locCmd, 0, sizeof(locCmd));

    //get the local command
    command_network_to_local(&locCmd, &request);
    
    //start getting blocks and sending them back
    struct block loc_blk;
    struct block net_blk;
    struct range curRange;

    memset(&loc_blk, , sizeof(loc_blk));

    strncpy(loc_blk.filename, locCmd.filename, MAXNAME);

    int requester_addr_len = sizeof(requester_addr);

    int rangeNum, curBlockNum;
    for(rangeNum = 0; rangeNum < locCmd.nranges; ++rangeNum) {
      curRange = locCmd.ranges[rangeNum];
      for(curBlockNum = curRange.first_block; curBlockNum <= curRange.last_bock, curBlockNum++) {
	if(get_fblock(locCmd.filename, curBlockNum, loc_blk.payload)) {
	  loc_blk.paysize = strlen(loc_blk.payload);
	  loc_blk.which_block = curBlockNum;
	  loc_blk.total_blocks = 0;
	  block_local_to_network(&loc_blk, &net_blk);
	  
	  sendto(sockfd, &net_blk, sizeof(net_blk), 0, requester_addr, requester_addr_len);
	}
      }
    }
  }

}
