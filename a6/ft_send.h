#ifndef _FT_SEND_H
#define _FT_SEND_H


#include "storage.h"
#include "help.h"
#include "nose.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>

#include "operations.h"

int distribute_file(char* name, char* contents, int numBocks, int send_sockfd, int recv_sockfd, int port);
int req_is_range(char* request, int sockfd, struct sockaddr_in* requester_addr);


#endif
