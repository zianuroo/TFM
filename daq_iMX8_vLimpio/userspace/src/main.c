
#include "../include/main.h"
#include "../include/myspi.h"
#include "../include/server.h"

#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>    

pthread_mutex_t data_adc_lock;        
pthread_mutex_t adc_reading_lock;
pthread_mutex_t connection_lock;
pthread_mutex_t nblock_adc_lock;
pthread_mutex_t enchan_adc_lock;
pthread_mutex_t param_lock;
pthread_mutex_t ndata_adc_lock;    
pthread_mutex_t read_adc_buffer_lock;                   

int main(int argc, char *argv[])
{
	
	// Estructura para pasar los argumentos del main a los threads
	struct tARGS *args;
	args = malloc(sizeof(struct tARGS));
	args->argc = argc; 
	args->argv = argv; 
	
	// Variables para creacion de threads
	pthread_t tid1, tid2;
	
	// Se inicializan los mutex
	if (pthread_mutex_init(&data_adc_lock, NULL) != 0) {
		printf("\n mutex init has failed\n");
		exit(0);
	}
	if (pthread_mutex_init(&adc_reading_lock, NULL) != 0) {
		printf("\n mutex init has failed\n");
		exit(0);
	}
	if (pthread_mutex_init(&connection_lock, NULL) != 0) {
		printf("\n mutex init has failed\n");
		exit(0);
	}
	if (pthread_mutex_init(&nblock_adc_lock, NULL) != 0) {
		printf("\n mutex init has failed\n");
		exit(0);
	}
	if (pthread_mutex_init(&enchan_adc_lock, NULL) != 0) { //
		printf("\n mutex init has failed\n");
		exit(0);
	}
	if (pthread_mutex_init(&param_lock, NULL) != 0) {
		printf("\n mutex init has failed\n");
		exit(0);
	}
	if (pthread_mutex_init(&read_adc_buffer_lock, NULL) != 0) {
		printf("\n mutex init has failed\n");
		exit(0);
	}

	// SE CREAN THREADS
	// Se crea un thread para la lectura de membank
	if (pthread_create(&tid1, NULL, mainRead, (void*) args) != 0) {
		perror("pthread_create() error");
		exit(1);
	}
	// Se crea un thread para la comunicacion mediante Ethernet/Wi-Fi
	if (pthread_create(&tid2, NULL, server, (void*) args) != 0) {
		perror("pthread_create() error");
		exit(1);
	}

	// SE ESPERA A QUE ACABEN LOS THREADS
	if (pthread_join(tid1, NULL) != 0) {
		perror("pthread_join() error");
		exit(2);
	}
	if (pthread_join(tid2, NULL) != 0) {
		perror("pthread_join() error");
		exit(2);
	}
	
	// Se destruyen los mutex
	pthread_mutex_destroy(&data_adc_lock);
	pthread_mutex_destroy(&adc_reading_lock);
	pthread_mutex_destroy(&connection_lock);
	pthread_mutex_destroy(&nblock_adc_lock);
	pthread_mutex_destroy(&enchan_adc_lock);
	pthread_mutex_destroy(&param_lock);
	pthread_mutex_destroy(&read_adc_buffer_lock);
	
	printf("Ejecucion terminada\n");
}









