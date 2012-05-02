/* called when udp datagram available on a socket 
 * socket: number of socket */ 
void udp(int sock); 

/* get a file from storage;
 * name: name of file (in local machine)
 * content: an array of content (result parameter)
 * size: size of file */ 
int get(char *name, char **content, int *s, int port, int send_sockfd); 

/* put a file into storage. 
 * name: local name of file. 
 * content: a character array of content. 
 * size: size of local file. */ 
int put(char *name, char *content, int size, int port, int send_sockfd); 

/* delete a file from storage 
 * name: name of file (on one machine) */ 
int del(char *name); 
