#include <stdlib.h>

#define MAX 128
#define ADCDATACOLUMN 8
#define ADCDATAROW 201
#define PORT 8080
#define SA struct sockaddr
#define SERVER_ADDRESS "192.168.137.212"

extern char *CMD_CLIENT[];
extern char *CMD_SERVER[];

extern int16_t **data_adc_client; 