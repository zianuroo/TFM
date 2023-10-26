/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

// Librerias generales
#include <math.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>

// Librerias propias
#include "../include/daq_ceit.h"
#include "../include/myspi.h"
#include "../include/myspi_funcs.h"
#include "../include/mygpio_funcs.h"
#include "../include/main.h"
#include "../include/server.h"
#include "../include/dataBank.h"

// Declaracion de macros y constantes
#define init_module(module_image, len, param_values) syscall(__NR_init_module, module_image, len, param_values)
#define finit_module(fd, param_values, flags) syscall(__NR_finit_module, fd, param_values, flags)
#define delete_module(name, flags) syscall(__NR_delete_module, name, flags)
#define REG_CURRENT_TASK _IOW('a','a',int32_t*)
#define SIGMB 44

// Variables compartidas con el server
int16_t **data_adc; // Tamaño == 2 bytes
int16_t M = 3; // Number of READADC Commands
uint16_t nblock_adc;
uint8_t enchan_adc = 0xFF;
uint8_t param_read = 0; 
float fadc = 25e3;
int newnice = -15;

// Declaracion de funciones
uint8_t number_chan(uint8_t enchan);
uint8_t chan_no(uint8_t enchan);

// Variables globales
uint8_t spi_mode = SPI_MODE_2; 	// CPHA='0, CPOL='1'
uint8_t spi_bits = 16;
uint32_t spi_speed = 11000000;	// In bits per second, default 11000000
uint16_t spi_delay = 1;			// In microseconds
uint16_t cnt_bloques_perdidos = 0;
int check = 0;
int interrupt = 0;
int adc_reading = 0;
int read_adc_buffer = 0;
// Parametro para definir si el programa funcionara de manera continua (1) o con una cantidad de bloques previamente especificada (0)
int demo = 1;

// Esta funcion se lanzara cada vez que se reciba una señal desde el modulo de kernel
void sig_event_handler(int n, siginfo_t *info, __attribute__ ((unused)) void *unused)
{
	if ((n == SIGMB) /*&& (interrupt == 0)*/) {
		check = info->si_int;
		interrupt = 1;
		cnt_bloques_perdidos++;
	}
}

void * mainRead(void* args)
{
	struct tARGS *targs = (struct tARGS*)args;
	int argc =  targs->argc;
	char** argv = targs->argv;
	int fd_spi;
	int ret;
	char *device = "/dev/spidev0.2";
	//float fdac;
	float fsclk_adc;
	uint8_t ndiv_dac = 3;
	uint16_t ndata_dac = 24;
	uint16_t control_dac = 733;
	int16_t *data_dac = NULL;
	uint8_t nchan_adc;
	uint16_t ndata_adc;
	uint8_t powermode_adc = POWER_MODE_ADC_NORMAL;
	uint8_t resetmode_adc = RESET_MODE_ADC_FULL;
	uint8_t os_adc = 0;
	uint8_t range_adc = 1;
	uint8_t parn_adc = 1;
	uint16_t convsttime_adc = 10;
	uint8_t sclktime_adc = 10;
	//int16_t *data_adc = NULL;
	uint8_t enchan_qdec = 0x03;
	uint16_t ndata_qdec = 24;
	uint16_t ndiv_qdec = 8;
	uint8_t nchan_qdec;
	// int16_t *data_qdec = NULL;
	int gpio_reset;
	// int gpio_membank; 
	int gpio_memrdy;
	int gpio_hb;
	//int nice; 
	int pid;
	int i, j, k, nerr, n;

	// Hasta que no haya conexion TCP/IP, el programa no empieza
	printf("[SPI]: esperando a conexion remota\n");
	while(1){
		pthread_mutex_lock(&connection_lock);
		if(connection == 1){
			pthread_mutex_unlock(&connection_lock);
			break;
		}
		pthread_mutex_unlock(&connection_lock);
	}
	printf("[SPI]: Conexion establecida, se conocen los parametros\n");

	// Reconfiguracion de prioridad del programa
	pid = getpid();
	//nice = getpriority(PRIO_PROCESS, pid);
	ret = setpriority(PRIO_PROCESS, pid, newnice);
	if (ret==0){
		//printf("Niceness value set from %d to %d\n", nice, newnice);
	}else{
		printf("Error setting new niceness value\n");
	}

	// Se muestran las condiciones de medida
	printf("[Parametros SPI]\n");
	nchan_adc = number_chan(enchan_adc);
	printf("	Number of ADC Channels Enabled: %d\n",nchan_adc);
	if (demo == 1){
		printf("	M indefinido: el sistema funcionara de manera indeterminada\n");
	}
	else{
		printf("	M: %d bloques\n", M);
	}
	printf("	NBLOCK ADC: %d\n",nblock_adc);
	printf("	SPI_SPEED: %d bps\n",spi_speed);
	printf("	fadc: %.0f Hz\n",fadc);
	ndata_adc = nblock_adc * nchan_adc;
	printf("	Tamano ndata: %d\n", ndata_adc);
	
	pthread_mutex_lock(&param_lock);
	param_read=1;
	pthread_mutex_unlock(&param_lock);

	/* allocate rows */
	if((data_adc = malloc(M * sizeof(int16_t *))) == NULL){
		perror("Failed to allocate rows");
		free(data_adc);
		data_adc = NULL;
		exit(1);
	}
	/* allocate cols */
	int c;
	for(c = 0; c < M; c++){
		if((data_adc[c] = (int16_t *) calloc((ndata_adc+4),sizeof(int16_t))) == NULL){
			perror("Failed to allocate cols");
			for (int j=0; j<c; j++) {
				free(data_adc[j]);
				data_adc[j]=NULL;
			}
			exit(2);   /* ideally you should free the cols and rows */
		}
	}

	// data_qdec = (int16_t *) calloc((ndata_qdec+4)*M,sizeof(int16_t));
	nchan_qdec = number_chan(enchan_qdec);

	// Heartbeat signal: GPIO4_IO02
	gpio_hb = 98;
	conf_gpio(gpio_hb,1); // Set to output GPIO
	write_gpio(gpio_hb,1);

	// Reset Signal: GPIO03_IO21 => 2*32 + 21 = 85
	// Set this pin to '1' to reset the FPGA
	gpio_reset = 85;
	conf_gpio(gpio_reset,1); // Set to output GPIO
	write_gpio(gpio_reset,1);
	usleep(10000);
	write_gpio(gpio_reset,0);
	usleep(100000);

	// MemRdy Signal: GPIO1_IO15
	gpio_memrdy = 15;
	conf_gpio(gpio_memrdy,0); // Set to input GPIO

	// Configuracion de SPI
	ret = configure_spi(device, &fd_spi);
	if (ret<0) {
		printf("Error in configure_spi()\n\n\n");
	} else {
		// printf("configure_spi() OK!!\n\n\n");
	}

	convsttime_adc = (uint16_t)(round(125e6/200/fadc));
	fsclk_adc = 5e6;
	sclktime_adc = (uint8_t)(120e6/2/fsclk_adc);
	
	// SE PREPARA EL SISTEMA PARA LEER EL MODULO DE KERNEL
	int fd_mbkmodule;
	// Insmod kernel module
	fd_mbkmodule = open("kernelspace/mbdriver/mbkmodule.ko", O_RDONLY);
	if (finit_module(fd_mbkmodule, "", 0) != 0) {
		printf("Error instalando \n");
		exit(1);
	}
	close(fd_mbkmodule);

	int32_t number;
	struct sigaction act;
	/* install custom signal handler */
	sigemptyset(&act.sa_mask);
	act.sa_flags = (SA_SIGINFO | SA_RESTART);
	act.sa_sigaction = sig_event_handler;
	sigaction(SIGMB, &act, NULL);

	// Open chardev
	fd_mbkmodule = open("/dev/mbdriver", O_RDWR);
	if(fd_mbkmodule < 0) {
			printf("Cannot open device file...\n");
			exit(1);
	}

	/* register this task with kernel for signal */
	if (ioctl(fd_mbkmodule, REG_CURRENT_TASK,(int32_t*) &number)) {
		printf("Failed\n");
		close(fd_mbkmodule);
		exit(1);
	}

	// Comando para inicalizar el ADC
	pthread_mutex_lock(&nblock_adc_lock);
	ret = send_command_iniadc(fd_spi, enchan_adc, convsttime_adc, sclktime_adc, os_adc, ndata_adc,
			nblock_adc, powermode_adc, resetmode_adc, range_adc, parn_adc);
	if (ret==1)
		{}// printf("	INIADC Command successfully applied\n");
	else if (ret==-1)
		printf("	Error in INIADC transfer16()\n");
	else if (ret==-2)
		printf("	Error in INIADC RCOMMAND\n");
	else if (ret==-3)
		printf("	Error in INIADC CRC16 received\n");
	else
		printf("	Error in INIADC received Ack (%d)\n",ret);
	pthread_mutex_unlock(&nblock_adc_lock);

	// Comando para inicalizar el QDEC
	ret = send_command_iniqdec(fd_spi, enchan_qdec, ndata_qdec, ndiv_qdec);
	if (ret==1)
		{}// printf("	INIQDEC Command successfully applied\n");
	else if (ret==-1)
		printf("	Error in INIQDECtransfer16()\n");
	else if (ret==-2)
		printf("	Error in INIQDEC RCOMMAND\n");
	else if (ret==-3)
		printf("	Error in INIQDECCRC16 received\n");
	else
		printf("	Error in INIQDEC received Ack (%d)\n",ret);

	// Comando para inicalizar el DAC
	ret = send_command_inidac(fd_spi, ndiv_dac, ndata_dac, control_dac);
	if (ret==1)
		{}// printf("	INIDAC Command successfully applied\n");
	else if (ret==-1)
		printf("	Error in INIDAC transfer16()\n");
	else if (ret==-2)
		printf("	Error in INIDAC RCOMMAND\n");
	else if (ret==-3)
		printf("	Error in INIDAC CRC16 received\n");
	else
		printf("	Error in INIDAC received Ack (%d)\n",ret);

	// Comando para escribir señal de salida en el DAC
	data_dac = (int16_t *) calloc(ndata_dac+4,sizeof(int16_t));
	srand(time(0));
	for(i=0;i<ndata_dac;i++)
		data_dac[i+1] = (int16_t)(rand() >> 16);
	ret = send_command_wrmdac(fd_spi, data_dac, ndata_dac);
	if (ret==1)
		{}// printf("	WRMDAC Command successfully applied\n");
	else if (ret==-1)
		printf("	Error in WRMDAC transfer16()\n");
	else if (ret==-2)
		printf("	Error in WRMDAC RCOMMAND\n");
	else if (ret==-3)
		printf("	Error in WRMDAC CRC16 received\n");
	else
		printf("	Error in WRMDAC received Ack (%d)\n",ret);

	// Comando para inicalizar los CLK para ADC y DAC 
	ret = send_command_startclk(fd_spi);
	if (ret==1)
		{}// printf("	STARTCLK Command successfully applied\n");
	else if (ret==-1)
		printf("	Error in STARTCLK transfer16()\n");
	else if (ret==-2)
		printf("	Error in STARTCLK RCOMMAND\n");
	else if (ret==-3)
		printf("	Error in STARTCLK CRC16 received\n");
	else
		printf("	Error in STARTCLK received Ack (%d)\n",ret);

	// Comando para iniciar el DAC
	ret = send_command_startdac(fd_spi);
	if (ret==1)
		{}// printf("	STARTDAC Command successfully applied\n");
	else if (ret==-1)
		printf("	Error in STARTDAC transfer16()\n");
	else if (ret==-2)
		printf("	Error in STARTDAC RCOMMAND\n");
	else if (ret==-3)
		printf("	Error in STARTDAC CRC16 received\n");
	else
		printf("	Error in STARTDAC received Ack (%d)\n",ret);

	// Comando para iniciar el QDEC
	ret = send_command_startqdec(fd_spi);
	if (ret==1)
		{}// printf("	STARTQDEC Command successfully applied\n");
	else if (ret==-1)
		printf("	Error in STARTQDEC transfer16()\n");
	else if (ret==-2)
		printf("	Error in STARTQDEC RCOMMAND\n");
	else if (ret==-3)
		printf("	Error in STARTQDEC CRC16 received\n");
	else
		printf("	Error in STARTQDEC received Ack (%d)\n",ret);

	// Bucle de lectura de datos
	i=0;
	if (demo == 0){
		while (i<M) {
			if (interrupt == 1){
				interrupt = 0;
				cnt_bloques_perdidos--;
				i++;
				// No es necesario el mutex porque el server leera el data_adc unicamente cuando haya datos listos
				ret = send_command_readadc(fd_spi, data_adc[i-1], ndata_adc, check);
				if (ret==1)
					{ }//printf("	READADC Command successfully applied\n");
				else if (ret==-1)
					printf("	Error in READADC transfer16()\n");
				else if (ret==-2)
					printf("	Error in READADC RCOMMAND\n");
				else if (ret==-3)
					printf("	Error in READADC CRC16 received\n");
				else
					printf("	Error in READADC received Ack (%d)\n",ret);
				
				// Se avisa que los datos estan listos
				pthread_mutex_lock(&adc_reading_lock);
				adc_reading++;
				pthread_mutex_unlock(&adc_reading_lock);

				//		ret = send_command_readqdec(fd_spi, data_qdec+(ndata_qdec+4)*i, ndata_qdec, membank);
				//		if (ret==1)
				//			printf("	READQDEC Command successfully applied\n");
				//		else if (ret==-1)
				//			printf("	Error in READQDEC transfer16()\n");
				//		else if (ret==-2)
				//			printf("	Error in READQDEC RCOMMAND\n");
				//		else if (ret==-3)
				//			printf("	Error in READQDEC CRC16 received\n");
				//		else
				//			printf("	Error in READQDEC received Ack (%d)\n",ret);

			}
		}
	}
	else{
		int write_adc_buffer = 0;
		while (1) {
			if (interrupt == 1){
				cnt_bloques_perdidos--;
				interrupt = 0;
				i++;
				// No es necesario el mutex porque el server leera el data_adc unicamente cuando haya datos listos
				ret = send_command_readadc(fd_spi, data_adc[write_adc_buffer], ndata_adc, check);
				if (ret==1)
					{ }//printf("	READADC Command successfully applied\n");
				else if (ret==-1)
					printf("	Error in READADC transfer16()\n");
				else if (ret==-2)
					printf("	Error in READADC RCOMMAND\n");
				else if (ret==-3)
					printf("	Error in READADC CRC16 received\n");
				else
					printf("	Error in READADC received Ack (%d)\n",ret);
				
				pthread_mutex_lock(&read_adc_buffer_lock);
				if (write_adc_buffer == 0){
					write_adc_buffer = 1;
					read_adc_buffer = 0;
				}
				else{
					write_adc_buffer = 0;
					read_adc_buffer = 1;
				}
				pthread_mutex_unlock(&read_adc_buffer_lock);

				// Se avisa que los datos estan listos
				pthread_mutex_lock(&adc_reading_lock);
				adc_reading++;
				pthread_mutex_unlock(&adc_reading_lock);
			}
		}
	}

	// Comando para parar el DAC
	ret = send_command_stopdac(fd_spi);
	if (ret==1)
		{}// printf("	STOPDAC Command successfully applied\n");
	else if (ret==-1)
		printf("	Error in STOPDAC transfer16()\n");
	else if (ret==-2)
		printf("	Error in STOPDAC RCOMMAND\n");
	else if (ret==-3)
		printf("	Error in STOPDAC CRC16 received\n");
	else
		printf("	Error in STOPDAC received Ack (%d)\n",ret);

	// Comando para parar el QDEC
	ret = send_command_stopqdec(fd_spi);
	if (ret==1)
		{}// printf("	STOPQDEC Command successfully applied\n");
	else if (ret==-1)
		printf("	Error in STOPQDEC transfer16()\n");
	else if (ret==-2)
		printf("	Error in STOPQDEC RCOMMAND\n");
	else if (ret==-3)
		printf("	Error in STOPQDEC CRC16 received\n");
	else
		printf("	Error in STOPQDEC received Ack (%d)\n",ret);

	// Comando para parar el CLK
	ret = send_command_stopclk(fd_spi);
	if (ret==1)
		{}// printf("	STOPCLK Command successfully applied\n");
	else if (ret==-1)
		printf("	Error in STOPCLK transfer16()\n");
	else if (ret==-2)
		printf("	Error in STOPCLK RCOMMAND\n");
	else if (ret==-3)
		printf("	Error in STOPCLK RC16 received\n");
	else
		printf("	Error in STOPCLK received Ack (%d)\n",ret);
	
	// Se desisntala el modulo de kernel
	close(fd_mbkmodule);
	if (delete_module("mbkmodule", O_NONBLOCK) != 0) {
		printf("Error borrando\n");
		return NULL;
	}

	// Se cuenta la cantidad de errores en los datos recibidos
	nerr=0;
	n = 0;
	pthread_mutex_lock(&data_adc_lock);
	for (j=0;j<nchan_adc;j++) {
		n += chan_no(enchan_adc>>n);
		for (i=0;i<M;i++) {
			for (k=0;k<ndata_adc/nchan_adc;k++) {
				// printf("Chan%d[%d] = %d\n",n,i*ndata_adc/nchan_adc+k,*(data_adc+(ndata_adc+4)*i+(j+3)+nchan_adc*k));
				if (adc_data[(i*ndata_adc/nchan_adc+k)%201][n-1] != data_adc[i][(j+3)+nchan_adc*k]) { //[j*(ndata_adc/nchan_adc)+k]
					// printf("Error in Chan%d[%d]\n",n,i*ndata_adc/nchan_adc+k);
					nerr++;
				}
			}
		}
	}
	pthread_mutex_unlock(&data_adc_lock);
	/*
	for (j=0;j<nchan_qdec;j++)
		for (i=0;i<M;i++)
			for (k=0;k<ndata_qdec/nchan_qdec;k++) {
				// printf("Chan%d[%d] = %d\n",j+1,i*ndata_qdec/nchan_qdec+k,*(data_qdec+(ndata_qdec+4)*i+(j+3)+nchan_qdec*k));
			}
	*/

	if (nerr==0) {
		printf("\t\t\tTest enchan_adc = %02x Passed!!\n",enchan_adc);
	} else {
		printf("\t\t\tTest enchan_adc = %02x Not Passed!!\n",enchan_adc);
		printf("\t\t\tCantidad de errores = %d\n",nerr);
	}
	printf("\t\t\tCantidad de bloques perdidos = %d\n",cnt_bloques_perdidos);
	
	// Hasta que no se cierre conexion, el programa no acaba
	while(1){
		pthread_mutex_lock(&connection_lock);
		if(connection == 0){
			pthread_mutex_unlock(&connection_lock);
			break;
		}
		pthread_mutex_unlock(&connection_lock);
	}

	// Se liberan las memorias asignadas dinamicamente
	free(data_dac);
	data_dac = NULL;
	
	// Ya se ha cerrado la conexion, no se necesita un mutex
	/* free cols */
	for (int i=0; i<M; i++) {
		free(data_adc[i]);
		data_adc[i]=NULL;
	}
	/* free rows */
	free(data_adc);
	data_adc = NULL;

	// Close SPI
	close(fd_spi);

	// Close GPIOs
	unconf_gpio(gpio_reset);
	unconf_gpio(gpio_memrdy);
	unconf_gpio(gpio_hb);

	return 0;
}


uint8_t number_chan(uint8_t enchan) {

	uint8_t n = 0;

	while (enchan!=0) {
		n += (enchan & 0x01);
		enchan >>= 1;
	}

	return n;

}

uint8_t chan_no(uint8_t enchan) {
	int n=1;
	while ((enchan & 0x01)==0) {
		n++;
		enchan >>= 1;
	}
	return n;
}