#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h> // bzero()
#include <unistd.h> // read(), write(), close()
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "../include/client.h"
#include "../include/client_funcs.h"
#include "../include/dataBank.h"

// Funcion para guardar en txt-s los datos recibidos
void dataSave(uint8_t enchan_adc_client, uint8_t nchan_adc_client, int16_t M_client, uint16_t ndata_adc_client){
	// Transmision de datos acabada se reconstruye la señal para que sea procesable a la hora de guardarlo en un txt
	char *readingsDir = 	"./ADCreadings";
	char aux[100];
	int n = 0;
	// Create the directory
	if (mkdir(readingsDir, 0700) == -1) {
		if (errno != EEXIST) {
			perror("Error creating directory");
		}
	}
	
	// Se guardan los datos por canales
	//printf("Proceeding to save the results...\n");
	for (int j=0;j<nchan_adc_client;j++) {
		n += chan_no(enchan_adc_client>>n);
		bzero(aux, 100);
		sprintf(aux, "./ADCreadings/AI%dlec.txt", n);		
		FILE *fp = fopen(aux, "w");
		if (fp == NULL){
			printf("Error opening the file %s", aux);
		}
		else{
			for (int i=0;i<M_client;i++) {
				for (int k=0;k<ndata_adc_client/nchan_adc_client;k++) {
					fprintf(fp, "%d\n", data_adc_client[i][(j+3)+nchan_adc_client*k]);
				}
			}
		}
		fclose(fp);
		//printf("	AI%d Succesfully saved!\n",n);
	}
	
	// Se crea un log con la informacion sobre los datos reibidos: si han sido correctos o no
	int nerr=0;
	n = 0;
	bzero(aux, 100);
	sprintf(aux, "./ADCreadings/log.txt");		
	FILE *fp = fopen(aux, "w");
	for (int j=0;j<nchan_adc_client;j++) {
		n += chan_no(enchan_adc_client>>n);
		fprintf(fp,"For channel %d\n", n);
		for (int i=0;i<M_client;i++) {
			for (int k=0;k<ndata_adc_client/nchan_adc_client;k++) {
				// printf("Chan%d[%d] = %d\n",n,i*ndata_adc_client/nchan_adc_client+k,*(data_adc+(ndata_adc_client+4)*i+(j+3)+nchan_adc_client*k));
				if (adc_data[(i*ndata_adc_client/nchan_adc_client+k)%201][n-1] != data_adc_client[i][(j+3)+nchan_adc_client*k]) { //[j*(ndata_adc_client/nchan_adc_client)+k]
					fprintf(fp,"Error in [%d] --> Expected: %d // Received %d\n", 
					i*ndata_adc_client/nchan_adc_client+k, 
					adc_data[(i*ndata_adc_client/nchan_adc_client+k)%201][n-1], 
					data_adc_client[i][(j+3)+nchan_adc_client*k]);
					
					nerr++;
				}
			}
		}
		if(nerr == 0){
			fprintf(fp,"	No errors!\n");
		}
		fprintf(fp,"\n\n");
		nerr = 0;
	}
	printf("	Log and data succesfully saved!\n");
	

}

// Funcion que sirve para imprimir los valores que se van recibiendo por TCP o UDP
void printValues(int received_block, uint16_t ndata_adc_client){

			printf("	Fin bloque de datos %d\n", received_block);
			//Impresion de valores recibidos
			for(int i=0; i<(ndata_adc_client + 4); i++){
				if ((i == 0) || (i == 1) || (i == 2)){
					printf("	CMD ID: 0x%08x\n", data_adc_client[received_block][i]);
				}
				else if( i == ((ndata_adc_client + 4)-1)){
					printf("	CRC16: 0x%08x\n", data_adc_client[received_block][i]);
				}
				else{
					//printf("	%d \n", data_adc_client[received_block][i]);
				}				
			}
}

// Funcion que sirve para imprimir los valores que se van recibiendo por TCP o UDP (en modo demo)
void printDemoValues(uint8_t nchan_adc_client, int received_block, uint16_t ndata_adc_client){
	for (int k=0;k<ndata_adc_client/nchan_adc_client;k++) {
		printf("	| ");
		for (int j=0;j<nchan_adc_client;j++) {
			printf("%6d | ", data_adc_client[received_block][(j+3)+nchan_adc_client*k]);
		}
		printf("\n");
	}
}

// Funcion que calcula el numero de canales activos
uint8_t number_chan(uint8_t enchan) {

	uint8_t n = 0;

	while (enchan!=0) {
		n += (enchan & 0x01);
		enchan >>= 1;
	}

	return n;
}

// Funcion que permite calcula cuales son los canales activos
uint8_t chan_no(uint8_t enchan) {
	int n=1;
	while ((enchan & 0x01)==0) {
		n++;
		enchan >>= 1;
	}
	return n;
}

