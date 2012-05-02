#ifndef FT_RECV_H
#define FT_RECV_H


#include "help.h" 
#include "nose.h"
#include "storage.h"

#include <stdarg.h>
#include <sys/stat.h>
#include <math.h>

#define TRUE  1
#define FALSE 0

#define REQUEST_PERCENTAGE .75

int req_is_block_to_store(char* request, int send_sockfd, struct sockaddr_in* sender_addr);

void request_file(char* filename, int port, int sockfd, int numBlocks, char** contents);


#endif
