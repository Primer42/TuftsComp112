/*=============================
   help for Assignment 2: torrents 
  =============================*/ 

#ifndef V2
#include "help.h" 
#else /* V2 */
#include "help-v2.h" 
#endif /* V2 */

#define TRUE  1
#define FALSE 0

/*=============================
   bit manipulation 
   network portable 64-bit ints
   There is no standard for these!
  =============================*/ 

// perform sanity checks on a bit array 
int bits_check (struct bits *b) { 
    if (b->nbits<=0) { 
	fprintf(stderr, "bits_check: nbits=%d invalid\n", b->nbits); 
	return FALSE; 
    } 
    if (b->array==0) { 
	fprintf(stderr, "bits_check: array invalid\n"); 
	return FALSE; 
    } 
    return TRUE; 
}

#ifdef V2
uint64_network_t hton64(uint64_t foo) { 
    uint64_network_t bar; 
    uint32_t mask = ((((uint64_t)1)<<(sizeof(uint32_t)*8))-1); 
    bar.low = foo & mask; 
    bar.high = (foo>>(sizeof(uint32_t)*8)) & mask; 
    return bar; 
} 

uint64_t ntoh64(uint64_network_t bar) { 
    return bar.low | (((uint64_t)bar.high)<<(sizeof(uint32_t)*8)); 
} 
#endif /* V2 */

/*=============================
   bit manipulation 
  =============================*/ 

#ifndef V2
void bits_alloc(struct bits *b, int nbits) { 
#else /* V2 */
void bits_alloc(struct bits *b, uint64_t nbits) { 
#endif /* V2 */
    bits_check(b); 
    b->nbits = nbits; 
#ifndef V2
    int size = BITSIZE(b); 
    b->array = (long *) malloc(size*sizeof(long)); 
    int i; 
    for (i=0; i<size; i++) b->array[i]=0; 
#else /* V2 */
    uint64_t size = BITSIZE(b); 
    b->array = (uint64_t *) malloc(size*sizeof(uint64_t)); 
    int i;
    for (i=0; i<size; i++) b->array[i]=0;
#endif /* V2 */
} 

void bits_free(struct bits *b) { 
    bits_check(b); 
    free(b->array); 
}

void bits_clearall(struct bits *b) { 
    bits_check(b); 
#ifndef V2
    int size = BITSIZE(b); int i; 
#else /* V2 */
    uint64_t size = BITSIZE(b); uint64_t i; 
#endif /* V2 */
    for (i=0; i<size; i++) b->array[i]=0; 
} 
    
#ifndef V2
void bits_setrange(struct bits *b, int first, int last) { 
    int i; 
#else /* V2 */
void bits_setrange(struct bits *b, uint64_t first, uint64_t last) { 
    uint64_t i; 
#endif /* V2 */
    bits_check(b); 
    if (first<0) first=0; 
    if (first>=b->nbits) first=b->nbits-1; 
    if (last<0) last=0; 
    if (last>=b->nbits) last=b->nbits-1; 
    if (first>last) { 
	fprintf(stderr, "first bit > last bit in bits_setrange\n"); 
	first=last; 
    } 
#ifndef V2
    int firstlong = first/BITSPERLONG; // truncating
    int lastlong = last/BITSPERLONG;   // truncating 
    int firstbit = first - firstlong*BITSPERLONG; 
    int lastbit  = last  - lastlong*BITSPERLONG; 
#else /* V2 */
    uint64_t firstlong = first/BITSPERLONG;
    uint64_t lastlong = last/BITSPERLONG;
    uint64_t firstbit = first - firstlong*BITSPERLONG; 
    uint64_t lastbit  = last  - lastlong*BITSPERLONG; 
#endif /* V2 */
    if (firstlong != lastlong) { 
    /* low word */ 
#ifndef V2
        long lowmask; 
	if (firstbit==0) lowmask = -1L; 
	else lowmask = ((1L<<(BITSPERLONG-firstbit+1))-1)<<firstbit; 
#else /* V2 */
        uint64_t lowmask; 
	if (firstbit==0) lowmask = ~((uint64_t)0); 
	else lowmask = ((((uint64_t)1)<<(BITSPERLONG-firstbit+1))-1)<<firstbit; 
#endif /* V2 */
	b->array[firstlong] |= lowmask; 
    /* middle words */ 
	for (i=firstlong+1; i<=lastlong-1; i++)
#ifndef V2
	    b->array[i]=-1L; /* 0xffffffffffffffff */
#else /* V2 */
	    b->array[i]=~((uint64_t)0); /* 0xffffffffffffffff */
#endif /* V2 */

    /* high word */ 
#ifndef V2
	long highmask; 
        if (lastbit==BITSPERLONG-1) highmask=-1L; 
        else highmask = (1L<<(lastbit+1))-1; 
#else /* V2 */
	uint64_t highmask; 
        if (lastbit==BITSPERLONG-1) highmask=~((uint64_t)0); 
        else highmask = (((uint64_t)1)<<(lastbit+1))-1; 
#endif /* V2 */
	b->array[lastlong] |= highmask; 
    } else { 

    /* inside one word */ 
#ifndef V2
        long midmask; 
	if (firstbit==0 && lastbit==BITSPERLONG-1) midmask=-1L; 
	else midmask = ((1L<<(lastbit-firstbit+1))-1)<<firstbit; 
#else /* V2 */
        uint64_t midmask; 
	if (firstbit==0 && lastbit==BITSPERLONG-1) midmask=~((uint64_t)0); 
	else midmask = ((((uint64_t)1)<<(lastbit-firstbit+1))-1)<<firstbit; 
#endif /* V2 */
	b->array[firstlong] |= midmask; 
    } 
} 

#ifndef V2
void bits_setbit(struct bits *b, int bit) { 
#else /* V2 */
void bits_setbit(struct bits *b, uint64_t bit) { 
#endif /* V2 */
    bits_check(b); 
    if (bit<0 || bit>=b->nbits) { 
	fprintf(stderr, "bits_setbit: invalid bit %d\n", bit); 
	return; 
    } 
#ifndef V2
    int whichbin = bit/BITSPERLONG; 
    int whichbit = bit%BITSPERLONG; 
    b->array[whichbin] |= 1L<<whichbit; 
#else /* V2 */
    uint64_t whichbin = bit/BITSPERLONG; 
    uint64_t whichbit = bit%BITSPERLONG; 
    b->array[whichbin] |= ((uint64_t)1)<<whichbit; 
#endif /* V2 */
} 

#ifndef V2
void bits_clearbit(struct bits *b, int bit) { 
#else /* V2 */
void bits_clearbit(struct bits *b, uint64_t bit) { 
#endif /* V2 */
    bits_check(b); 
    if (bit<0 || bit>=b->nbits) { 
	fprintf(stderr, "bits_clearbit: invalid bit %d\n", bit); 
	return; 
    } 
#ifndef V2
    int whichbin = bit/BITSPERLONG; 
    int whichbit = bit%BITSPERLONG; 
    b->array[whichbin] &= ~(1L<<whichbit); 
#else /* V2 */
    uint64_t whichbin = bit/BITSPERLONG; 
    uint64_t whichbit = bit%BITSPERLONG; 
    b->array[whichbin] &= ~(((uint64_t)1)<<whichbit); 
#endif /* V2 */
} 

#ifndef V2
int bits_testbit(struct bits *b, int bit) { 
#else /* V2 */
int bits_testbit(struct bits *b, uint64_t bit) { 
#endif /* V2 */
    bits_check(b); 
    if (bit<0 || bit>=b->nbits) { 
	fprintf(stderr, "bits_testbit: invalid bit %d\n", bit); 
	return FALSE; 
    } 
#ifndef V2
    int whichbin = bit/BITSPERLONG; 
    int whichbit = bit%BITSPERLONG; 
    return ((b->array[whichbin])&(1L<<whichbit))!=0; 
#else /* V2 */
    uint64_t whichbin = bit/BITSPERLONG; 
    uint64_t whichbit = bit%BITSPERLONG; 
    return ((b->array[whichbin])&(((uint64_t)1)<<whichbit))!=0; 
#endif /* V2 */
} 

int bits_empty(struct bits *b) { 
    bits_check(b); 
#ifndef V2
    int size = BITSIZE(b); int i; 
#else /* V2 */
    uint64_t size = BITSIZE(b); uint64_t i; 
#endif /* V2 */
    for (i=0; i<size; i++) 
	if (b->array[i]) return FALSE; 
    return TRUE; 
    
} 

void bits_printall(struct bits *b) { 
    bits_check(b); 
#ifndef V2
    int size = BITSIZE(b); int i; 
#else /* V2 */
    uint64_t size = BITSIZE(b); uint64_t i; 
#endif /* V2 */
    for (i=0; i<size; i++) { 
	fprintf(stderr, "%d: 0x%lx\n", i, b->array[i]); 
    } 
} 

void bits_printlist(struct bits *b) { 
#ifndef V2
    int i; 
#else /* V2 */
    uint64_t i; 
#endif /* V2 */
    int first_one=TRUE; 
    bits_check(b); 
    for (i=0; i<b->nbits; i++) { 
	if (bits_testbit(b,i)) { 
	    if (first_one) { 
		first_one=FALSE; 
	    } else { 
		fprintf(stderr, ","); 
	    } 
	    fprintf(stderr, "%d", i); 
        } 
    } 
} 

/*=============================
   low-level protocol help 
  =============================*/ 

// translate a local binary structure to network order 
#ifndef V2
void block_local_to_network(struct block *local, struct block *net) { 
    net->which_block = htonl(local->which_block); 
    net->total_blocks = htonl(local->total_blocks); 
    net->paysize = htonl(local->paysize); 
#else /* V2 */
void block_local_to_network(struct block *local, struct block_network *net) { 
    net->which_block = hton64(local->which_block); 
    net->total_blocks = hton64(local->total_blocks); 
    net->paysize = hton64(local->paysize); 
#endif /* V2 */
    memcpy(net->filename, local->filename, MAXNAME); 
    memcpy(net->payload, local->payload,   BLOCKSIZE); 
} 

// translate a network binary structure to local order 
#ifndef V2
void block_network_to_local(struct block *local, struct block *net) { 
    local->which_block = ntohl(net->which_block); 
    local->total_blocks = ntohl(net->total_blocks); 
    local->paysize = ntohl(net->paysize); 
#else /* V2 */
void block_network_to_local(struct block *local, struct block_network *net) { 
    local->which_block = ntoh64(net->which_block); 
    local->total_blocks = ntoh64(net->total_blocks); 
    local->paysize = ntoh64(net->paysize); 
#endif /* V2 */
    memcpy(local->filename, net->filename,  MAXNAME); 
    memcpy(local->payload, net->payload,  BLOCKSIZE); 
} 

// translate a local binary structure to network order 
#ifndef V2
void command_local_to_network(struct command *local, struct command *network) {
#else /* V2 */
void command_local_to_network(struct command *local, struct command_network *network) {
#endif /* V2 */
    int i; 
    memcpy(network->filename, local->filename, MAXNAME); 
#ifndef V2
    network->nranges = htonl(local->nranges); 
#else /* V2 */
    network->nranges = hton64(local->nranges); 
#endif /* V2 */
    for (i=0; i<MAXRANGES; i++) { 
#ifndef V2
	network->ranges[i].first_block = htonl(local->ranges[i].first_block); 
	network->ranges[i].last_block = htonl(local->ranges[i].last_block); 
#else /* V2 */
	network->ranges[i].first_block = hton64(local->ranges[i].first_block); 
	network->ranges[i].last_block = hton64(local->ranges[i].last_block); 
#endif /* V2 */
    } 
} 

// translate a network binary structure to local order 
#ifndef V2
void command_network_to_local(struct command *local, struct command *network) {
#else /* V2 */
void command_network_to_local(struct command *local, struct command_network *network) {
#endif /* V2 */
    int i; 
    memcpy(local->filename, network->filename,  MAXNAME); 
#ifndef V2
    local->nranges = ntohl(network->nranges); 
#else /* V2 */
    local->nranges = ntoh64(network->nranges); 
#endif /* V2 */
    for (i=0; i<MAXRANGES; i++) { 
#ifndef V2
	local->ranges[i].first_block = ntohl(network->ranges[i].first_block); 
	local->ranges[i].last_block = ntohl(network->ranges[i].last_block); 
#else /* V2 */
	local->ranges[i].first_block = ntoh64(network->ranges[i].first_block); 
	local->ranges[i].last_block = ntoh64(network->ranges[i].last_block); 
#endif /* V2 */
    } 
} 

/*=============================
   high-level protocol help 
  =============================*/ 

/* send a command to a server */ 
int send_command(int sockfd, const char *address, int port, 
	  	 const char *filename, int first, int last) { 
#ifndef V2
    struct command loc_cmd, net_cmd; 
#else /* V2 */
    struct command loc_cmd; struct command_network net_cmd; 
#endif /* V2 */
    struct sockaddr_in server_addr; 
/* set up cmd structure */ 
    memset(&loc_cmd, 0, sizeof(loc_cmd)); 
    strcpy(loc_cmd.filename, filename); 
    loc_cmd.nranges=1; 
    loc_cmd.ranges[0].first_block = first; 
    loc_cmd.ranges[0].last_block = last; 
/* translate to network order */ 
    command_local_to_network(&loc_cmd, &net_cmd) ; 
/* set up target address */ 
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = PF_INET;
    inet_pton(PF_INET, address, &server_addr.sin_addr);
    server_addr.sin_port        = htons(port);
/* send the packet: only send instantiated ranges */ 
    int sendsize = COMMAND_SIZE(loc_cmd.nranges);
    int ret = sendto(sockfd, (void *)&net_cmd, sendsize, 0, 
	(struct sockaddr *)&server_addr, sizeof(server_addr)); 
    if (ret<0) perror("send_command"); 
    return ret;
} 

/* receive a block from a server */ 
int recv_block(int sockfd, struct block *b, struct sockaddr_in *resp_addr) { 
    int resp_len; 			/* length used */ 
    int resp_mesglen; 
#ifndef V2
    struct block net_blk; 
#else /* V2 */
    struct block_network net_blk; 
#endif /* V2 */
again: 
    resp_mesglen = 0; 
    resp_len = sizeof(resp_addr); /* MUST initialize this */ 
    resp_mesglen = recvfrom(sockfd, &net_blk, sizeof(net_blk), 0, 
	    (struct sockaddr *) resp_addr, &resp_len);
    /* check for socket error */ 
    if (resp_mesglen<0) { 
	perror("recv_block"); 
	return resp_mesglen; 
    /* check for erronious packets */ 
    } else if (resp_mesglen!=sizeof(net_blk)) { 
	fprintf(stderr, "recv_block: received %d bytes, expected %ld", 
		resp_mesglen, sizeof(struct block)); 
        fprintf(stderr, " -- ignoring bad input\n"); 
	goto again; 
    } 
    /* translate to local byte order */ 
    block_network_to_local(b, &net_blk); 
    return resp_mesglen; 
} 

/* run a "select" on the local socket, with timeout */ 
int select_block(int sockfd, int seconds, int microsec) { 
  /* set up a timeout wait for input */ 
    struct timeval tv;
    fd_set rfds;
  /* Watch sockfd to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
  /* Wait up to one second. */
    tv.tv_sec = seconds; tv.tv_usec = microsec;
    return select(sockfd+1, &rfds, NULL, NULL, &tv); 
} 

/*=============================
  client usage and argument checking 
  =============================*/
void client_usage() { 
    fprintf(stderr,
	"client usage: ./client {server_address} {server_port} {client_port} {filename}\n");
    fprintf(stderr, 
	"  {server_address} is the IP address where the server is running.\n"); 
    fprintf(stderr, 
	"  {server_port} is the port number of the server.\n"); 
    fprintf(stderr, 
	"  {client_port} is the port this client should use.\n"); 
    fprintf(stderr, 
	"  {filename} is the file name to get.\n"); 
} 

int client_arguments_valid(const char *server_dotted, int server_port, 
			   int client_port, const char *filename) { 

    int valid = TRUE; 
    struct in_addr server_addr; 	/* native address */ 
    if (inet_pton(AF_INET, server_dotted, &server_addr)<=0) { 
	fprintf(stderr, "client: server address %s is not valid\n", 
		server_dotted); 
	valid=FALSE; 
	
    } 
    
    if (server_port<9000 || server_port>32767) { 
	fprintf(stderr, "client: server_port %d is not allowed\n", 
		server_port); 
	valid=FALSE; 
    } 

    if (client_port<9000 || client_port>32767) { 
	fprintf(stderr, "client: client_port %d is not allowed\n", 
		client_port); 
	valid=FALSE; 
    } 

    if (index(filename, '/')) { 
	fprintf(stderr, "client: '/' is not allowed in filename %s\n", 
		filename); 
	valid=FALSE; 
    } 
    return valid; 
} 

/*=============================
  routines that only work properly in Halligan hall
  (because they understand Halligan network policies)
  read the primary IP address for an ECE/CS linux host 
  this is always the address bound to interface eth0
  this is used to avoid listening (by default) on 
  secondary interfaces. 
  =============================*/ 
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
