#ifndef FT_RECV_H
#define FT_RECV_H


#include "help.h" 
#include "nose.h"
#include "storage.h"

#include <sys/stat.h>
#include <math.h>

#define TRUE  1
#define FALSE 0

#define REQUEST_PERCENTAGE .75

void request_file(char* filename, char* host_dotted, int port, int sockfd, int numBlocks, char** contents);


#endif
