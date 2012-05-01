#ifndef _NOSE_H
#define _NOSE_H

#include "help.h"

#define MAX_STORED_HOSTS 10


//make a structure to store records of the hosts we find
typedef struct _host_record{
  char hostAddr[INET_ADDRSTRLEN];
  time_t lastSeenAt;
} hostRecord;


void addOrUpdateHost(char* newHostAddr, time_t seenAt);
void addOrUpdateHostNow(char* newHostAddr);
hostRecord* getHostRecordAt(int i);
void initDiscovery(int inputPort, int udp_sock);
int req_is_alive(char* request);

#endif
