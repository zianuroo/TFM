#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // read(), write(), close()

#include "../include/server.h"
#include "../include/server_TCP.h"
#include "../include/server_UDP.h"
#include "../include/myspi.h"
#include "../include/main.h"

#define MAX 128
#define PORT 8080
#define SA struct sockaddr

char *CMD_CLIENT[] = {"RDDATA", "exit", "START", "END"};
char *CMD_SERVER[] = {"SNDATA", "exit", "ERR", "END"};

int connection = 0;
int block_sent = 1;

uint8_t number_chan(uint8_t enchan);

void *server(void* args){

	struct tARGS *targs = (struct tARGS*)args;
	int argc =  targs->argc;
	char** argv = targs->argv;
	int protocolo = 0;
	
	// Se comprueba si se ha escrito el comando -UDP
	for (int i=1;i<argc;i++) {
		if (strcmp(argv[i],"-UDP")==0) {
			protocolo = 1;
		}
	}
	
	// Si no se ha ecrito, se lanza server_TCP. Si se ha escrito, se lanza server_UDP
	if (protocolo == 0){
		server_TCP();
	}
	else{
		server_UDP();
	}
	
	return NULL;

}