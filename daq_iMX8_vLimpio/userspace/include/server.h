#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // read(), write(), close()
#include <stdint.h>

#define PORT 8080
#define SA struct sockaddr

extern char *CMD_CLIENT[];
extern char *CMD_SERVER[];

extern void* server(void* args);

extern int connection;
extern int block_sent;

