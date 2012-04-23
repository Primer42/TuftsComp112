#include <stdio.h> 
#include "halligan.h"

int main (int argc, const char * argv[]) {
    struct in_addr me; 
    char addressBuffer[INET_ADDRSTRLEN]; 
    get_primary_addr(&me); 
    inet_ntop(AF_INET, &me, addressBuffer, INET_ADDRSTRLEN);
    printf("The primary IP Address for this host is %s\n", 
	addressBuffer); 
}

