#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "nose.h"

#define BROADCAST_EVERY 5

//if DEAD_THRESHOLD is less than BROADCAST_EVERY, we will not find ourselves
#define DEAD_THRESHOLD 4
#define NUM_BROADCAST_ADDRS 1

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

int port; //port is going to be used by both sending and recieving sides
//these will be used just by the sending side,
//but since the sending method will respond to a signal,
//they must be stored outside of the method so we don't have to remake them
//for every send.
int send_sockfd;
struct sockaddr_in bcast_addrs[NUM_BROADCAST_ADDRS];
struct timespec timeBetweenBroadcasts;


/* logging of server actions */ 
#define MAXOUT 256 		/* maximum number of output chars for flog */ 
static void flog(const char *fmt, ...) {
  return;
    va_list ap;
    char *p; 
    char buf[MAXOUT]; 
    va_start(ap, fmt);
    fprintf(stderr,"[nose: "); 
    vsnprintf(buf,MAXOUT,fmt,ap); 
    for (p=buf; *p && p-buf<MAXOUT; p++) 
	if ((*p)>=' ') 
	    putc(*p,stderr); 
    fprintf(stderr,"]\n"); 
    va_end(ap);
}


// read the primary IP address for an ECE/CS host 
// this is always the address bound to interface eth0
// this is used to avoid listening (by default) on 
// maintenance interfaces. 
// taken from halligan.c
/* int get_primary_addr(struct in_addr *a) {  */
/*     struct ifaddrs * ifAddrStruct=NULL; */
/*     struct ifaddrs * ifa=NULL; */
/*     if (!getifaddrs(&ifAddrStruct)) // get linked list of interface specs */
/* 	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) { */
/* 	    if (ifa ->ifa_addr->sa_family==AF_INET) {  // is an IP4 Address */
/* 		if (strcmp(ifa->ifa_name,"eth0")==0) { // is for interface eth0 */
/* 		    void *tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr) */
/* 			->sin_addr; */
/* 		    memcpy(a, tmpAddrPtr, sizeof(struct in_addr));  */
/* 		    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct); */
/* 		    return 0; // found  */
/* 		}  */
/* 	    }  */
/* 	} */
/*     if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct); */
/*     return -1; // not found */
/* } */ 

hostRecord* getHostRecordAt(int i) {
  if(i < 0 || i >= MAX_STORED_HOSTS) {
    perror("Index out of range");
    return NULL;
  }
  //make sure it isn't out of date
  time_t currentTime = time(NULL);
  if(difftime(currentTime, records[i].lastSeenAt) > DEAD_THRESHOLD) {
    return NULL;
  }

  return & records[i];
}

hostRecord* chooseRandomHost() {
  int randNum;
  hostRecord* curChoice;
  do {
    randNum = rand() % MAX_STORED_HOSTS;
  } while((curChoice = getHostRecordAt(randNum)) == NULL);
  return curChoice;
}


//this method handles our SIGALRM signals
//this is how we'll send out broadcast ping every 5 seconds
//and also print the known hosts every 5 seconds
void checkHostsAliveSignalHandler(int sig) {
  if(sig == SIGALRM) {
    //send out our broadcast packet
    //make the message
    char send_line [MAXMESG];
    send_line[0] = '\0';
    strncat(send_line, ALIVE_MESG, strlen(ALIVE_MESG));
    int i;
    char hostLine[MAXMESG];
    for(i = 0; i < MAX_STORED_HOSTS; ++i) {
      hostRecord* curHost = getHostRecordAt(i);
      if(curHost == NULL) {
	continue;
      }
      snprintf(hostLine, MAXMESG, "%s %ld\n", curHost->hostAddr, curHost->lastSeenAt);
      strncat(send_line, hostLine, strlen(hostLine));
    }

    flog("Sending message '%s' on socket %d\n", send_line, send_sockfd);

    //set the alarm for the next broadcast
    alarm(BROADCAST_EVERY);
    
    //send the packet - all of the necessary variables have been initialized already
    for(i = 0; i < NUM_BROADCAST_ADDRS; ++i) {
      flog("Sending broadcast to %s\n", inet_ntop(PF_INET, (void*) &(bcast_addrs[i].sin_addr.s_addr), hostLine, INET_ADDRSTRLEN));
      sendto(send_sockfd, (void*) send_line, strlen(send_line), 0, (struct sockaddr *)&(bcast_addrs[i]), (socklen_t) sizeof(bcast_addrs[i]));
      nanosleep(&timeBetweenBroadcasts, NULL);
    }

  }
}

void addOrUpdateHost(char* newHostAddr, time_t seenAt) {
  //find the first host that is either out of date or matches our connected host
  flog("Got host %s seen at %ld\n", newHostAddr, seenAt);
  int matchingIndex = -1;
  int outOfDateIndex = -1;
  int i;
  time_t currentTime = time(NULL);
  for(i = 0; i < MAX_STORED_HOSTS || (matchingIndex > 0 && outOfDateIndex > 0); ++i) {
    if(outOfDateIndex < 0 && difftime(currentTime, records[i].lastSeenAt) > DEAD_THRESHOLD) {
      outOfDateIndex = i;
      //there may be an out of date before our match, so we must keep looking
    }
    if(matchingIndex < 0 && strncmp(records[i].hostAddr, newHostAddr, INET_ADDRSTRLEN) == 0) {
      matchingIndex = i;
      //if we have found a match, we don't need to find an out of date
      break;
    }
  }
  
  //update the lastSeenAt if we have a match
  if(matchingIndex >= 0) {
    //if we found a match, update that record if it's newer
    if(difftime(seenAt, records[matchingIndex].lastSeenAt) > 0)
      records[matchingIndex].lastSeenAt = seenAt;
  } else if(outOfDateIndex >= 0) {
    //if we didn't find a match, but we found an out of date record
    //put our new record at that location
    strncpy(records[outOfDateIndex].hostAddr, newHostAddr, INET_ADDRSTRLEN);
    records[outOfDateIndex].lastSeenAt = seenAt;
  } else {
    perror("Out of space for records. Please increase MAX_STORED_HOSTS.");
    exit(1);
  }

  flog("Done adding host %s", newHostAddr);
}

void addOrUpdateHostNow(char* newHostAddr) {
  addOrUpdateHost(newHostAddr, time(NULL));
}

int req_is_alive(char* request) {
  if(strncmp(request, ALIVE_MESG, strlen(ALIVE_MESG)) == 0) {
    //add the hosts from the message to our records
    char newHost[INET_ADDRSTRLEN];
    time_t remoteSeenAt;
    
    char* nextLine = request;
    while((nextLine = strchr(nextLine, '\n')) != NULL) {
      //get past the newline
      nextLine++;
      if(*nextLine == '\0')
	break;
      //read in the host and when it was last seen
      sscanf(nextLine, "%s %ld", newHost, &remoteSeenAt);
      addOrUpdateHost(newHost, remoteSeenAt);
    }
    
    return 1;
  }
  return 0;
}

void initDiscovery(int inputPort, int udp_sock) {
  port = inputPort;
  send_sockfd = udp_sock;

  //set up the broadcast addresses
  memset(&bcast_addrs, 0, sizeof(struct sockaddr_in) * NUM_BROADCAST_ADDRS);
  
  bcast_addrs[0].sin_family=PF_INET;
  //bcast_addrs[1].sin_family=PF_INET;
  //bcast_addrs[2].sin_family=PF_INET;
  //bcast_addrs[3].sin_family=PF_INET;

  //inet_aton("130.64.23.255", &bcast_addrs[0].sin_addr);
  //inet_aton("10.4.1.255", &bcast_addrs[1].sin_addr);
  //inet_aton("10.5.1.255", &bcast_addrs[2].sin_addr);
  inet_aton("255.255.255.255", &bcast_addrs[0].sin_addr);


  bcast_addrs[0].sin_port = htons(port);
  //bcast_addrs[1].sin_port = htons(port);
  //bcast_addrs[2].sin_port = htons(port);
  //bcast_addrs[3].sin_port = htons(port);
  
  //set up the records array
  records = (hostRecord*) malloc(sizeof(hostRecord) * MAX_STORED_HOSTS);
  memset(records, 0, sizeof(hostRecord) * MAX_STORED_HOSTS);

  timeBetweenBroadcasts.tv_sec = 1;
  timeBetweenBroadcasts.tv_nsec = 0;

  //set up signal handling method
  signal(SIGALRM, checkHostsAliveSignalHandler);
  //start the SIGALRM
  raise(SIGALRM);
}
