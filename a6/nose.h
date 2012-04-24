#ifndef _NOSE_H
#define _NOSE_H

#define MAX_STORED_HOSTS 10

//make a structure to store records of the hosts we find
typedef struct _host_record{
  char hostAddr[INET_ADDRSTRLEN];
  time_t lastSeenAt;
} hostRecord;


void addOrUpdateHost(char* newHostAddr);
void checkHostsAliveSignalHandler(int sig);
hostRecord* getHostRecordAt(int i);


#endif
