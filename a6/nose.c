#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "nose.h"

#define MAX_MESG 1024
#define BROADCAST_EVERY 5

//if DEAD_THRESHOLD is less than BROADCAST_EVERY, we will not find ourselves
#define DEAD_THRESHOLD 4
#define NUM_BROADCAST_ADDRS 2

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

// read the primary IP address for an ECE/CS host 
// this is always the address bound to interface eth0
// this is used to avoid listening (by default) on 
// maintenance interfaces. 
// taken from halligan.c
int get_primary_addr(struct in_addr *a) { 
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    if (!getifaddrs(&ifAddrStruct)) // get linked list of interface specs
	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
	    if (ifa ->ifa_addr->sa_family==AF_INET) {  // is an IP4 Address
		if (strcmp(ifa->ifa_name,"eth0")==0) { // is for interface eth0
		    void *tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)
			->sin_addr;
		    memcpy(a, tmpAddrPtr, sizeof(struct in_addr)); 
		    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
		    return 0; // found 
		} 
	    } 
	}
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return -1; // not found
} 

hostRecord* getHostRecordAt(int i) {
  if(i < 0 || i > MAX_STORED_HOSTS) {
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

//this method handles our SIGALRM signals
//this is how we'll send out broadcast ping every 5 seconds
//and also print the known hosts every 5 seconds
void checkHostsAliveSignalHandler(int sig) {
  if(sig == SIGALRM) {
    //send out our broadcast packet
    //make the message
    char send_line [MAX_MESG];
    strncat(send_line, "alive\n", 6);
    int i;
    char hostLine[MAX_MESG];
    for(i = 0; i < MAX_STORED_HOSTS; ++i) {
      hostRecord* curHost = getHostRecordAt(i);
      if(curHost == NULL) {
	continue;
      }
      snprintf(hostLine, MAX_MESG, "%s %ld\n", curHost->hostAddr, curHost->lastSeenAt);
      strncat(send_line, hostLine, strlen(hostLine));
    }
    
    //send the packet - all of the necessary variables have been initialized already
    int i;
    for(i = 0; i < NUM_BROADCAST_ADDRS; ++i) {
      sendto(send_sockfd, (void*) send_line, strlen(send_line), 0, (struct sockaddr *)&(bcast_addrs[i]), (socklen_t) sizeof(bcast_addrs[i]));
    }
    
    //set the alarm for the next broadcast
    alarm(BROADCAST_EVERY);

  }
}

void addOrUpdateHost(char* newHostAddr, time_t seenAt) {
      //find the first host that is either out of date or matches our connected host
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
	//if we found a match, update that record
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
}

void addOrUpdateHostNow(char* newHostAddr) {
  addOrUpdateHost(newHostAddr, time(NULL));
}

int req_is_alive(char* request) {
  if(strncmp(request, "alive\n", 6) == 0) {
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
  bcast_addrs[1].sin_family=PH_INET;

  inet_aton("130.64.23.255", bcast_addrs[0].sin_addr);
  inet_aton("10.255.255.255", bcast_addrs[1].sin_addr);


  bcast_addrs[0].sin_port = htons(port);
  bcast_addrs[1].sin_port = htons(port);
  
  //set up the records array
  records = (hostRecord*) malloc(sizeof(hostRecord) * MAX_STORED_HOSTS);
  memset(records, 0, sizeof(hostRecord) * MAX_STORED_HOSTS);

  //set up signal handling method
  signal(SIGALRM, checkHostsAliveSignalHandler);
  //start the SIGALRM
  raise(SIGALRM);
  
}

/* int main(int argc, char** argv) { */

/*   if(argc != 2) { */
/*     fprintf(stderr, "%s usage: %s port\n", argv[0], argv[0]); */
/*     exit(1); */
/*   } */

/*   //read in the port */
/*   port = atoi(argv[1]); */

/*   //make sure it's valid */
/*   if (port<9000 || port>32767) {  */
/*     fprintf(stderr, "%s: port %d is not allowed\n", argv[0], port);  */
/*     exit(1);  */
/*   }  */

/*   //initialize the variables for the send method */
/*   send_sockfd = socket(PF_INET, SOCK_DGRAM, 0); */

/*   if (send_sockfd < 0) { */
/*     perror("socket:"); */
/*   } */

/*   /\* allow broadcasts on the send socket*\/  */
/*   const int broadcast=1; */
/*   if((setsockopt(send_sockfd,SOL_SOCKET,SO_BROADCAST, */
/* 		 &broadcast,sizeof broadcast))) { */
/*     perror("setsockopt - SO_SOCKET "); */
/*     exit(1);  */
/*   }  */

/*   /\* set send target broadcast address *\/  */
/*   memset(&bcast_addr, 0, sizeof(bcast_addr)); */
/*   bcast_addr.sin_family = PF_INET; */
/*   bcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST); */
/*   bcast_addr.sin_port = htons(port); */

/*   //set up the records array */
/*   records = (hostRecord*) malloc(sizeof(hostRecord) * MAX_STORED_HOSTS); */
/*   memset(records, 0, sizeof(hostRecord) * MAX_STORED_HOSTS); */

/*   //set up signal handling method */
/*   signal(SIGALRM, checkHostsAliveSignalHandler); */
/*   //start the SIGALRM */
/*   raise(SIGALRM); */

/*   //start dealing with recieving packets */

/*   /\* receiver data *\/  */
/*   struct sockaddr_in recv_addr; 	/\* server address *\/  */
/*   /\* message data *\/  */
/*   int mesglen; char message[MAX_MESG]; */
/*   /\* sender data *\/  */
/*   struct sockaddr_in send_addr; 	/\* raw sender address *\/  */
/*   socklen_t send_len; 			/\* length used *\/  */
/*   char send_dotted[INET_ADDRSTRLEN]; 	/\* string ip address *\/  */
/*   int recv_sockfd; */

/*   //make the recieving socket */
/*   recv_sockfd = socket(PF_INET, SOCK_DGRAM, 0); */

/*   if (recv_sockfd < 0) { */
/*     perror("socket:"); */
/*   } */

/*   /\* allow broadcasts on the recieving socket*\/  */
/*   if((setsockopt(recv_sockfd,SOL_SOCKET,SO_BROADCAST, */
/* 		 &broadcast,sizeof broadcast))) { */
/*     perror("setsockopt - SO_SOCKET "); */
/*     exit(1);  */
/*   }  */


/*   /\* get the primary IP address of this host *\/  */
/*   struct in_addr primary;  */
/*   get_primary_addr(&primary);  */
/*   char primary_dotted[INET_ADDRSTRLEN];  */
/*   inet_ntop(PF_INET, &primary, primary_dotted, INET_ADDRSTRLEN); */
/*   fprintf(stderr, "%s: Running on %s, port %d\n", argv[0], */
/* 	  primary_dotted, port);  */
  
/*   //initialize the recv_addr */
/*   memset(&recv_addr, 0, sizeof(recv_addr)); */
/*   recv_addr.sin_family      = PF_INET; */
/*   // must use INADDR_ANY to receive broadcasts */
/*   recv_addr.sin_addr.s_addr = htonl(INADDR_ANY); */
/*   recv_addr.sin_port        = htons(port); */
  
/*   /\* bind it to the primary address and selected port on this host *\/  */
/*   if (bind(recv_sockfd, (struct sockaddr *) &recv_addr, sizeof(recv_addr))<0)  */
/*     perror("bind");  */
  
/*   for ( ; ; ) { */
/*     /\* get a datagram *\/  */
/*     send_len = (socklen_t) sizeof(send_addr); /\* MUST initialize this *\/  */
/*     mesglen = recvfrom(recv_sockfd, message, MAX_MESG, 0,  */
/* 		       (struct sockaddr *) &send_addr, &send_len); */
    
/*     if (mesglen>=0) {  */
/*       /\* get numeric internet address *\/ */
/*       inet_ntop(PF_INET, (void *)&(send_addr.sin_addr.s_addr),   */
/* 		send_dotted, INET_ADDRSTRLEN); */
/*       addOrUpdateHost(send_dotted); */
/*     } else {  */
/*       perror("receive failed");  */
/*     }  */
/*   } */
/* } */
