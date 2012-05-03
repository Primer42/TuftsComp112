#ifndef __HELP_H__
#define __HELP_H__

/*=============================
  helper routines for Assignment 2: torrents 
  =============================*/ 

#include <stdlib.h> 
#include <stdint.h> 
#include <unistd.h> 
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <values.h> 
#include <fcntl.h> 

#include "storage.h"

#ifdef V2
/*=============================
   network portable 64-bit ints
  =============================*/ 

typedef struct { 
    uint32_t high; 
    uint32_t low; 
} uint64_network_t; 
extern uint64_network_t hton64(uint64_t foo); 
extern uint64_t ntoh64(uint64_network_t bar); 
#endif /* V2 */

/*=============================
  protocol definitions 
  =============================*/ 

#define MAXRANGES    12		/* ranges in one command */ 

#define ALIVE_MESG "a"
#define STORE_BLOCK_MESG "b"
#define REQUEST_RANGE_MESG "r"
#define DELETE_BLOCK_MESG "d"

// a range of blocks to send 
struct range { 
#ifndef V2
    uint32_t first_block; 
    uint32_t last_block; 
#else /* V2 */
    uint64_t first_block; 
    uint64_t last_block; 
#endif /* V2 */
} ; 

#ifdef V2
// network-portable version
struct range_network { 
    uint64_network_t first_block; 
    uint64_network_t last_block; 
} ; 
#endif /* V2 */

// a response from the server: a piece of a file 
struct block { 
    char filename[MAXNAME]; 
#ifndef V2
    uint32_t which_block; 
    uint32_t total_blocks; 
    uint32_t paysize; 
#else /* V2 */
    uint64_t which_block; 
    uint64_t total_blocks; 
    uint64_t paysize; 
#endif /* V2 */
    char payload[BLOCKSIZE]; 
} ; 

#ifdef V2
// network-portable version
struct block_network { 
    char filename[MAXNAME]; 
    uint64_network_t which_block; 
    uint64_network_t total_blocks; 
    uint64_network_t paysize; 
    char payload[BLOCKSIZE]; 
} ; 
#endif /* V2 */

// command to the server : which blocks of what file to get
struct command { 
  char type[MAXNAME];
  char filename[MAXNAME]; 
#ifndef V2
  uint32_t nranges; 
#else /* V2 */
  uint64_t nranges; 
#endif /* V2 */
  struct range ranges[MAXRANGES]; 
}; 

#ifdef V2
// portable network version
struct command_network { 
    char filename[MAXNAME]; 
    uint64_network_t nranges; 
    struct range_network ranges[MAXRANGES]; 
} ; 
#endif /* V2 */

// this is used in size calculations i
// to send partial structs
#ifndef V2
struct command_prefix { 
#else /* V2 */
struct command_network_prefix { 
#endif /* V2 */
    char filename[MAXNAME]; 
#ifndef V2
    uint32_t nranges; 
    struct range ranges[1]; 
#else /* V2 */
    uint64_t nranges; 
    struct range_network ranges[1]; 
#endif /* V2 */
} ; 

// compute the size of a command to send or receive
#ifndef V2
#define COMMAND_SIZE(RANGES) \
    (sizeof(struct command_prefix) + ((RANGES-1)*sizeof(struct range)))
#else /* V2 */
#define COMMAND_SIZE(RANGES) \
    (sizeof(struct command_network_prefix) \
	+ ((RANGES-1)*sizeof(struct range_network)))
#endif /* V2 */

/*=============================
   low-level protocol help 
  =============================*/ 

// network order => local order 
extern void block_network_to_local(struct block *local, 
#ifndef V2
	struct block *net); 
#else /* V2 */
	struct block_network *net); 
#endif /* V2 */
extern void command_network_to_local(struct command *local, 
#ifndef V2
	struct command *network); 
#else /* V2 */
	struct command_network *network); 
#endif /* V2 */

// local order => network order 
extern void block_local_to_network(struct block *local, 
#ifndef V2
	struct block *net); 
#else /* V2 */
	struct block_network *net); 
#endif /* V2 */
extern void command_local_to_network(struct command *local, 
#ifndef V2
	struct command *network); 
#else /* V2 */
	struct command_network *network); 
#endif /* V2 */

/*=============================
   high-level protocol help 
  =============================*/ 

/* send a command to a server */ 
extern int send_command(int sockfd, const char *address, int port, 
	const char *filename, int first, int last) ; 

/* receive a block from a server */ 
extern int recv_block(int sockfd, struct block *b, 
        struct sockaddr_in *resp_addr); 

/* run a "select" on input from servers, with timeout */ 
extern int select_block(int sockfd, int seconds, int microsec); 

/*=============================
   bit manipulation routines 
  =============================*/ 

#ifndef V2
#define BITSPERLONG (8*sizeof(long))
#else /* V2 */
#define BITSPERLONG (8*sizeof(uint64_t))
#endif /* V2 */

#define BITSIZE(BITS) \
	(((BITS)->nbits)/BITSPERLONG + ((((BITS)->nbits)%BITSPERLONG)!=0))

struct bits { 
#ifndef V2
    long *array; 
    int nbits; 
#else /* V2 */
    uint64_t *array; 
    uint64_t nbits; 
#endif /* V2 */
} ; 

#ifndef V2
extern void bits_alloc(struct bits *b, int nbits); 
#else /* V2 */
extern void bits_alloc(struct bits *b, uint64_t nbits); 
#endif /* V2 */

extern void bits_free(struct bits *b); 
extern void bits_clearall(struct bits *b); 

#ifndef V2
extern void bits_setrange(struct bits *b, int first, int last); 
extern void bits_setbit(struct bits *b, int bit); 
extern void bits_clearbit(struct bits *b, int bit); 
extern int bits_testbit(struct bits *b, int bit); 
#else /* V2 */
extern void bits_setrange(struct bits *b, uint64_t first, uint64_t last); 
extern void bits_setbit(struct bits *b, uint64_t bit); 
extern void bits_clearbit(struct bits *b, uint64_t bit); 
extern int bits_testbit(struct bits *b, uint64_t bit); 
#endif /* V2 */

extern int bits_empty(struct bits *b); 
extern void bits_printall(struct bits *b); 
extern void bits_printlist(struct bits *b); 

/*=============================
  client-specific routines 
  =============================*/ 
extern void client_usage(); 
extern int client_arguments_valid(
	const char *server_dotted, int server_port, 
       int client_port, const char *filename); 

/*=============================
   halligan-specific routines
  =============================*/ 

/* read the primary IP address for an ECE/CS host */ 
extern int get_primary_addr(struct in_addr *a); 

#endif /* __HELP_H__ */ 
