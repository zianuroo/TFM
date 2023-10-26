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
#include "../include/myspi.h"
#include "../include/main.h"

#define MAX 128
#define PORT 8080

int server_TCP()
{
	int sockfd, connfd;
	unsigned int len;
	struct sockaddr_in servaddr, cli;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
		printf("socket bind failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully binded..\n");

	// Now server is ready to listen and verification
	if ((listen(sockfd, 5)) != 0) {
		printf("Listen failed...\n");
		exit(0);
	}
	else
		printf("Server listening..\n");
	len = sizeof(cli);

	// Accept the data packet from client and verification
	connfd = accept(sockfd, (SA*)&cli, (socklen_t*)&len);
	if (connfd < 0) {
		printf("server accept failed...\n");
		exit(0);
	}
	else
		printf("server accept the client...\n");

	// Function for chatting between client and server
	char buff[MAX];
	int numblock = 0;
	char info[MAX];
	uint8_t ncanales;
	int16_t M_server;
	uint16_t nblock_adc_server;
	uint8_t enchan_adc_server;
	uint16_t ndata_adc_server;
	int string_max_size = 256;
	char * string_from_array = NULL;
	int datawrite = 0;
	ssize_t bytes_written;

	string_from_array  = (char *) calloc(string_max_size, sizeof(char));
	if(NULL == string_from_array){
		printf("Memory allocation failed. Exiting...");
		exit(0);
	}
	memset(string_from_array, 0, string_max_size);
	
	// infinite loop for chat
	for (;;) {
		// LECTURA DE CMD_CLIENT
		// Al principio el servidor se queda esperando hasta que el cliente se comunica con el
		bzero(buff, MAX);
		read(connfd, buff, sizeof(buff));		
		//printf("Received command: %s\n", buff);

		//Comando RDDATA --> Se responde con SNDATA
		if (strncmp(CMD_CLIENT[0], buff, 6) == 0) {
			// Se leen los parametros de medida
			printf("Received command: %s\n", buff);
			char delim[] = " ";
			char *ptr = strtok(buff, delim);
			int infonum = 0;
			while(ptr != NULL){
				char aux_str[80];
				sprintf(aux_str,"%s", ptr);
				ptr = strtok(NULL, delim);
				if(infonum == 1){
					enchan_adc = atoi(aux_str);
				}
				else if(infonum == 2){
					M = atoi(aux_str);
					if(M>0){
						demo = 0;
					}
					else{
						demo = 1;
						M = 2; // Dos bloques para usarlos en modo pingpong
					}
				}
				else if(infonum == 3){
					nblock_adc = atoi(aux_str);
				}
				else if(infonum == 4){
					spi_speed = atoi(aux_str);
				}
				else if(infonum == 5){
					fadc = atof(aux_str);
				}
				else if(infonum == 6){
					newnice = atoi(aux_str);
				}
				infonum++;
			}			
			// Cantidad de bloques de datos
			if (demo == 1){
				M_server = -1; // Para confirmar al cliente de que la transmision va a ser continua
			}
			else{
				M_server = M;
			}
			nblock_adc_server = nblock_adc;
			enchan_adc_server = enchan_adc;
			
			char enchan_adc_info[MAX];
			char nblock_adc_info[MAX];
			char M_server_info[MAX];
			sprintf(enchan_adc_info, "%d", enchan_adc_server);
			sprintf(nblock_adc_info, "%d", nblock_adc_server);
			sprintf(M_server_info, "%d", M_server);
			
			sprintf(info," ");
			strcat(info,enchan_adc_info);
			strcat(info," ");
			strcat(info,nblock_adc_info);
			strcat(info," ");
			strcat(info,M_server_info);

			ncanales = number_chan(enchan_adc_server);
			ndata_adc_server = nblock_adc_server * ncanales;

			pthread_mutex_lock(&connection_lock);
			connection = 1;
			pthread_mutex_unlock(&connection_lock);

			// Se espera a la configuracion del SPI
			while(1){
				pthread_mutex_lock(&param_lock);
				if(param_read == 1){
					pthread_mutex_unlock(&param_lock);
					break;
				}
				pthread_mutex_unlock(&param_lock);
			}
			bzero(buff, MAX);
			strncpy(buff, CMD_SERVER[0], sizeof(buff));
			strcat(buff,info);
		}
		
		//Comando exit o END --> Se responde con exit (mas adelante se cierra ejecucion)
		else if ((strncmp(CMD_CLIENT[1], buff, 4) == 0) || (strncmp(CMD_CLIENT[3], buff, 3) == 0)) {
			bzero(buff, MAX);
			strncpy(buff, CMD_SERVER[1], sizeof(buff));
			//printf("Sent answer : %s\n", buff);
		}
		
		// Comando START --> Se envia la informacion
		else if (strncmp(CMD_CLIENT[2], buff, 5) == 0) {
			// Si ya se han mandado "M_server" bloques, cuando se reciba START, se avisa que ya ha acabado la lectura de ADC mediante el END
			if(numblock == M_server){
				pthread_mutex_lock(&connection_lock);
				connection = 0;
				pthread_mutex_unlock(&connection_lock);
				block_sent = 0;
				bzero(buff, MAX);
				strncpy(buff, CMD_SERVER[3], sizeof(buff));
				//printf("Sent answer : %s\n", buff);
			}
			else{
				// Si es la primera iteracion o se acaba de mandar un bloque, se debe de esperar a que nuevos datos esten listos.
				if(block_sent == 1){
					block_sent = 0;
					// Se espera a que los datos esten listos
					while(1){
						pthread_mutex_lock(&adc_reading_lock);
						if(adc_reading > 0){
							adc_reading--;
							pthread_mutex_unlock(&adc_reading_lock);
							break;
						}
						pthread_mutex_unlock(&adc_reading_lock);
						usleep(100);
					}
				}
				// Se transmiten los datos.
				bzero(buff, MAX);
				if (demo == 0){
					// No se necesita el mutex ya que los datos seran leidos una vez el myspi ya ha acabado de escribir un bloque.
					bytes_written = write(connfd, &data_adc[numblock][0], sizeof(uint16_t)*(ndata_adc_server + 4));
				}
				else{
					pthread_mutex_lock(&read_adc_buffer_lock);
					bytes_written = write(connfd, &data_adc[read_adc_buffer][0], sizeof(uint16_t)*(ndata_adc_server + 4));
					pthread_mutex_unlock(&read_adc_buffer_lock);
					printf("bloque %d enviado \n", read_adc_buffer);
				}
	
				if (bytes_written == -1) {
					printf("write error \n"); // handle error
				}
				if ((unsigned)bytes_written < sizeof(uint16_t)*(ndata_adc_server + 4)) {
					printf("not all the data was written, you may need to write the remaining data again\n");
				}
				numblock++;
				block_sent = 1;
				datawrite = 1;
			}
		}
		
		// Comando mal escrito --> No deberia ocurrir ya que los comandos se transmiten de forma automatica
		else{
			printf("Invalid command\n");
			bzero(buff, MAX);
			strncpy(buff, CMD_SERVER[2], sizeof(buff));
			printf("Sent answer : %s\n", buff);
		}

		// ENVIO DE COMANDO
		if(datawrite == 0){
			write(connfd, buff, sizeof(buff));
		}
		datawrite = 0;
		
		// Si hemos mandado el comando exit --> Se cierra conexion (esto ocurre cuando se ha recibido un exit desde el cliente)
		if (strncmp("exit", buff, 4) == 0) {
			printf("Server Exit...\n");
			break;
		}
	}

	// After chatting close the socket
	close(sockfd);
	return 1;
}