#ifndef _NOSE_H
#define _NOSE_H

#define MAX_STORED_HOSTS 10

//make a structure to store records of the hosts we find
typedef struct _host_record{
  char hostAddr[INET_ADDRSTRLEN];
  time_t lastSeenAt;
} hostRecord;

/*This is the array of host records.
It is going to initialized to all 0's.
When we find a host, we'll traverse it, looking for one of the following
1) A record matching the host we just heard from
2) An out of date record i.e. a record that is more than DEAD_THRESHOLD seconds old.
If we find a matching record, we'll update the lastSeenAt time.
Otherwise, we'll add the new record at the location of the out of date record.
It is NOT guaranteed that the out of date records will be at the beginning of
the array - in fact, this is likely to not be the case.
Therefore, we must traverse the entire array to make sure we find a matching record.
*/
hostRecord* records;

void addOrUpdateHost(char* newHostAddr);
void checkHostsAliveSignalHandler(int sig);
hostRecord* getHostRecordAt(int i);


#endif
