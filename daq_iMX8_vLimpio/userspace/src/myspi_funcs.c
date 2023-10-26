/*
 * myspi_funcs.c
 *
 *  Created on: Jan 19, 2018
 *      Author: airizar
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <errno.h>

#include "../include/crc16.h"
#include "../include/daq_ceit.h"

#define NUMELS(x) (sizeof(x)/sizeof((x)[0]))

extern uint8_t spi_mode; // CPHA='0, CPOL='1'
extern uint8_t spi_bits;
extern uint32_t spi_speed;	// In bits per second

// It calculates the new crc16 with the newByte. Variable crcValue is the actual or initial value
uint16_t crc16(uint16_t crcValue, unsigned char newByte)
{
	unsigned char i;

	for (i = 0; i < 8; i++) {

		if (((crcValue & 0x8000) >> 8) ^ (newByte & 0x80)){
			crcValue = (crcValue << 1)  ^ CRC16_CCITT;
		}else{
			crcValue = (crcValue << 1);
		}

		newByte <<= 1;
	}

	return crcValue;
}

uint16_t crc16_uint16_false(uint16_t *data, int len) {

	uint16_t crc16_o = 0xFFFF;
	int i;
	unsigned char aux;

	for (i=0;i<len;i++) {
		aux = (unsigned char)((data[i]>>8) & 0xFF);
		crc16_o = crc16(crc16_o,aux);
		// printf("aux=%x\ncrc16_o=%x\n",aux,crc16_o);
		aux = (unsigned char)(data[i] & 0xFF);
		crc16_o = crc16(crc16_o,aux);
		// printf("aux=%x\ncrc16_o=%x\n",aux,crc16_o);
	}

	return crc16_o;

}

int configure_spi(char *device, int *fd) {

	int ret = 0;
	int ret1;

	*fd = open(device, O_RDWR);
	
	if (fd < 0) {
		perror("Can't open SPI device");
		ret = -1;
	}

	/*
	 * spi mode
	 */
	ret1 = ioctl(*fd, SPI_IOC_WR_MODE, &spi_mode);
	if (ret1 == -1) {
		perror("can't set spi mode");
		ret = ret1;
	}

	ret1 = ioctl(*fd, SPI_IOC_RD_MODE, &spi_mode);
	if (ret1 == -1) {
		perror("can't get spi mode");
		ret = ret1;
	}

	/*
	 * bits per word
	 */
	ret1 = ioctl(*fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits);
	if (ret1 == -1) {
		perror("can't set bits per word");
		ret = ret1;
	}

	ret1 = ioctl(*fd, SPI_IOC_RD_BITS_PER_WORD, &spi_bits);
	if (ret1 == -1) {
		perror("can't get bits per word");
		ret = ret1;
	}

	/*
	 * max speed hz
	 */
	ret1 = ioctl(*fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);
	if (ret1 == -1) {
		perror("can't set max speed hz");
		ret = ret1;
	}

	ret1 = ioctl(*fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed);
	if (ret1 == -1) {
		perror("can't get max speed hz");
		ret = ret1;
	}

	printf("	spi mode: 0x%x\n", spi_mode);
	printf("	bits per word: %d\n", spi_bits);
	//printf("max speed: %d Hz (%d KHz)\n", spi_speed, spi_speed/1000);

	return ret;
}

// Funcion utilizada para realizar transmisiones SPI
int transfer16(int fd, uint16_t *tx, uint16_t *rx, uint32_t len)
{
	int ret;
	errno=0;

	struct spi_ioc_transfer tr = {
		.tx_buf = (__u64)tx,
		.rx_buf = (__u64)rx,
		.len = len,
		.delay_usecs = 1,
		.speed_hz = spi_speed,
		.bits_per_word = 16,
	};

	/*
	printf("\nvalor len = %d (tamaño tx y rx en bytes)\n", len);
	printf("spi will interpret %d bytes\n", len*tr.bits_per_word); 	//https://youtu.be/QTd4epkBBek?t=491
	printf("valor en logic = %d\n", len*tr.bits_per_word/32); 		// o len/2
	*/

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret< 0){
	    printf("SPI IOCTL ret(%d), error(%d) %s\n", ret, errno, strerror(errno));
	}

	return ret;

}

int send_command_inidac(int fd, uint8_t ndiv_dac, uint16_t ndata_dac, uint16_t control_dac)
{
	int ret;

	//int i = 0;
	uint16_t tx[4];
	uint16_t rx[NUMELS(tx) + 2];
	uint16_t crc16_o, crc16_i;
	uint8_t rcommand;
	uint8_t ack;
	uint32_t len = 2*NUMELS(rx); //2*sizeof(rx)

	// LDAC Frequency is 		--> fDAC = fADC/NDIV_DAC
	// Period of the DAC Signal --> PERIOD_DAC_SIGNAL = NDATA_DAC/fDAC

	//printf("Comando INIDAC\n");

	tx[0] = (THE_INIDAC_COMMAND << 8) + ndiv_dac;
	tx[1] = ndata_dac;
	tx[2] = control_dac;
	tx[3] = crc16_uint16_false(tx,NUMELS(tx)-1);

	ret = transfer16(fd, tx, rx, len);

	if (ret==-1) {
		return -1;
	}

	rcommand = (rx[NUMELS(tx)] >> 8) & 0xFF;
	ack = (rx[NUMELS(tx)] & 0x01);
	crc16_i = rx[NUMELS(tx)+1];

	crc16_o = crc16_uint16_false(rx+NUMELS(tx),1);

	if (rcommand != (THE_INIDAC_COMMAND ^ 0x80)) {
//		 printf("ERROR: RCOMMAND is incorrect => 0x%02x\n",rcommand);
//		 for (i=0;i<len/2;i++)
//		 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -2;
	} else if (crc16_i != crc16_o) {
		// printf("ERROR: Received CRC16 is incorrect => 0x%04x\n",crc16_i);
		// for (i=0;i<len/2;i++)
		// 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -3;
	} else {
		return ack;
	}
}

int send_command_iniadc(int fd, uint8_t enchan_adc, uint16_t convsttime_adc, uint8_t sclktime_adc, uint8_t os_adc, uint16_t ndata_adc,
						uint16_t nblock_adc, uint8_t powermode_adc, uint8_t resetmode_adc, uint8_t range_adc, uint8_t parn_adc) {

	int ret;

	uint16_t tx[6];
	uint16_t rx[NUMELS(tx)+2]; // The receiver buffer size if the size of the transmission + 2 words
	uint16_t crc16_o, crc16_i;
	uint8_t rcommand;
	uint8_t ack;
	uint32_t len = 2*NUMELS(rx); // 2*sizeof(rx)
	uint16_t i = 0;

	// Sampling Frequency of ADC		--> fADC = 120e6/200/convsttime_adc
	// Frequency of SCLK Signal of ADC	--> fSCLK => 120e6/2/sclktime_adc
	// NCHAN_ADC is the number of active ADC Channels (number of '1' in enchan_adc)
	// NDATA_ADC must be a multiple of NBLOCK*NCHAN_ADC

	//printf("Comando INIADC\n");

	tx[0] = (THE_INIADC_COMMAND << 8) + enchan_adc;	// First word to be transmitted
	tx[1] = (convsttime_adc << 4) + (powermode_adc << 2) + resetmode_adc; // Second word to be transmitted
	tx[2] = (sclktime_adc << 11) + (os_adc << 8) + (range_adc << 7) + (parn_adc << 6); // Third word to be transmitted
	tx[3] = (ndata_adc << 3); // Fourth word to be transmitted
	tx[4] = (nblock_adc << 3); // Fifth word to be transmitted
	tx[5] = crc16_uint16_false(tx,NUMELS(tx)-1); // Transmit the CRC of the 5 previous words

	/*
	printf("Comando THE_INIADC_COMMAND enviado\n");
	for (i=0;i<6;i++)
		printf("tx[%d]=0x%04x\n",i,tx[i]);
	*/

	ret = transfer16(fd, tx, rx, len);

	if (ret==-1) {
		return -1;
	}

	// The receiver words to check are the last two of vector rx[]
	rcommand = (rx[NUMELS(tx)] >> 8) & 0xFF;	// Response Command
	ack = (rx[NUMELS(tx)] & 0x01);				// Acknowledge
	crc16_i = rx[NUMELS(tx)+1];					// CRC received

	// Calculates the CRC of the words received (except the CRC)
	crc16_o = crc16_uint16_false(rx+NUMELS(tx),1);

	/*
	printf("Respuesta THE_INIADC_COMMAND recibida\n");
		for (i=0;i<(NUMELS(tx)+2);i++)
			printf("rx[%d]=0x%04x\n",i,rx[i]);
	*/

	if (rcommand != (THE_INIADC_COMMAND ^ 0x80)) { // Incorrect Response Command
		 printf("ERROR: RCOMMAND is incorrect => 0x%02x\n",rcommand);
		 for (i=0;i<len/2;i++)
			printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -2;
	} else if (crc16_i != crc16_o) {	// CRC received is different than the calculated
		// printf("ERROR: Received CRC16 is incorrect => 0x%04x\n",crc16_i);
		// for (i=0;i<len/2;i++)
		// 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -3;
	} else {
		return ack;
	}
}

int  send_command_iniqdec(int fd, uint8_t enchan_qdec, uint16_t ndata_qdec, uint16_t ndiv_qdec)
{

	int ret;

	uint16_t tx[4];
	uint16_t rx[NUMELS(tx)+2];
	uint16_t crc16_o, crc16_i;
	uint8_t rcommand;
	uint8_t ack;
	uint32_t len = 2*NUMELS(rx); // 2*sizeof(rx)
	//int i = 0;

	// Sampling Frequency of QDEC    --> fQDEC = fADC/NDIV_QDEC
	// NCHAN_QDEC is the number of active QDEC channels (number of '1' in enchan_qdec)
	// NDATA_QDEC must be a multiple of NCHAN_QDEC

	//printf("Comando INIQDEC\n");

	tx[0] = (THE_INIQDEC_COMMAND << 8) + enchan_qdec;
	tx[1] = ndata_qdec;
	tx[2] = ndiv_qdec;
	tx[3] = crc16_uint16_false(tx,NUMELS(tx)-1);

	ret = transfer16(fd, tx, rx, len);

	if (ret==-1) {
		return -1;
	}

	rcommand = (rx[NUMELS(tx)] >> 8) & 0xFF;
	ack = (rx[NUMELS(tx)] & 0x01);
	crc16_i = rx[NUMELS(tx)+1];

	crc16_o = crc16_uint16_false(rx+NUMELS(tx),1);

	if (rcommand != (THE_INIQDEC_COMMAND ^ 0x80)) {
		// printf("ERROR: RCOMMAND is incorrect => 0x%02x\n",rcommand);
		// for (i=0;i<len/2;i++)
		// 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -2;
	} else if (crc16_i != crc16_o) {
		// printf("ERROR: Received CRC16 is incorrect => 0x%04x\n",crc16_i);
		// for (i=0;i<len/2;i++)
		// 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -3;
	} else {
		return ack;
	}
}

int send_command_startclk(int fd) {

	int ret;

	uint16_t tx[2];
	uint16_t rx[NUMELS(tx)+2];
	uint16_t crc16_o, crc16_i;
	uint8_t rcommand;
	uint8_t ack;
	uint32_t len = 2*NUMELS(rx); // 2*sizeof(rx)
	//int i = 0;


	tx[0] = (THE_STARTCLK_COMMAND << 8);
	tx[1] = crc16_uint16_false(tx,NUMELS(tx)-1);

	//printf("Comando STARTCLK\n");

	ret = transfer16(fd, tx, rx, len);

	if (ret==-1) {
		return -1;
	}

	rcommand = (rx[NUMELS(tx)] >> 8) & 0xFF;
	ack = (rx[NUMELS(tx)] & 0x01);
	crc16_i = rx[NUMELS(tx)+1];

	crc16_o = crc16_uint16_false(rx+NUMELS(tx),1);

	if (rcommand != (THE_STARTCLK_COMMAND ^ 0x80)) {
		// printf("ERROR: RCOMMAND is incorrect => 0x%02x\n",rcommand);
		// for (i=0;i<len/2;i++)
		// 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -2;
	} else if (crc16_i != crc16_o) {
		// printf("ERROR: Received CRC16 is incorrect => 0x%04x\n",crc16_i);
		// for (i=0;i<len/2;i++)
		// 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -3;
	} else {
		return ack;
	}
}

int send_command_startdac(int fd) {

	int ret;

	uint16_t tx[2];
	uint16_t rx[NUMELS(tx)+2];
	uint16_t crc16_o, crc16_i;
	uint8_t rcommand;
	uint8_t ack;
	uint32_t len = 2*NUMELS(rx); // 2*sizeof(rx)
	//int i = 0;


	tx[0] = (THE_STARTDAC_COMMAND << 8);
	tx[1] = crc16_uint16_false(tx,NUMELS(tx)-1);

	//printf("Comando STARTDAC\n");

	ret = transfer16(fd, tx, rx, len);

	if (ret==-1) {
		return -1;
	}

	rcommand = (rx[NUMELS(tx)] >> 8) & 0xFF;
	ack = (rx[NUMELS(tx)] & 0x01);
	crc16_i = rx[NUMELS(tx)+1];

	crc16_o = crc16_uint16_false(rx+NUMELS(tx),1);

	if (rcommand != (THE_STARTDAC_COMMAND ^ 0x80)) {
		// printf("ERROR: RCOMMAND is incorrect => 0x%02x\n",rcommand);
		// for (i=0;i<len/2;i++)
		// 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -2;
	} else if (crc16_i != crc16_o) {
		// printf("ERROR: Received CRC16 is incorrect => 0x%04x\n",crc16_i);
		// for (i=0;i<len/2;i++)
		// 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -3;
	} else {
		return ack;
	}
}

int send_command_startqdec(int fd) {

	int ret;

	uint16_t tx[2];
	uint16_t rx[NUMELS(tx)+2];
	uint16_t crc16_o, crc16_i;
	uint8_t rcommand;
	uint8_t ack;
	uint32_t len = 2*NUMELS(rx); // 2*sizeof(rx)
	//int i = 0;


	tx[0] = (THE_STARTQDEC_COMMAND << 8);
	tx[1] = crc16_uint16_false(tx,NUMELS(tx)-1);

	//printf("Comando STARTQDEC\n");

	ret = transfer16(fd, tx, rx, len);

	if (ret==-1) {
		return -1;
	}

	rcommand = (rx[NUMELS(tx)] >> 8) & 0xFF;
	ack = (rx[NUMELS(tx)] & 0x01);
	crc16_i = rx[NUMELS(tx)+1];

	crc16_o = crc16_uint16_false(rx+NUMELS(tx),1);

	if (rcommand != (THE_STARTQDEC_COMMAND ^ 0x80)) {
		// printf("ERROR: RCOMMAND is incorrect => 0x%02x\n",rcommand);
		// for (i=0;i<len/2;i++)
		// 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -2;
	} else if (crc16_i != crc16_o) {
		// printf("ERROR: Received CRC16 is incorrect => 0x%04x\n",crc16_i);
		// for (i=0;i<len/2;i++)
		// 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -3;
	} else {
		return ack;
	}
}

int send_command_stopclk(int fd) {

	int ret;

	uint16_t tx[2];
	uint16_t rx[NUMELS(tx)+2];
	uint16_t crc16_o, crc16_i;
	uint8_t rcommand;
	uint8_t ack;
	uint32_t len = 2*NUMELS(rx); // 2*sizeof(rx)
	//int i = 0;


	tx[0] = (THE_STOPCLK_COMMAND << 8);
	tx[1] = crc16_uint16_false(tx,NUMELS(tx)-1);

	//printf("Comando STOPCLK\n");

	ret = transfer16(fd, tx, rx, len);

	if (ret==-1) {
		return -1;
	}

	rcommand = (rx[NUMELS(tx)] >> 8) & 0xFF;
	ack = (rx[NUMELS(tx)] & 0x01);
	crc16_i = rx[NUMELS(tx)+1];

	crc16_o = crc16_uint16_false(rx+NUMELS(tx),1);

	if (rcommand != (THE_STOPCLK_COMMAND ^ 0x80)) {
		// printf("ERROR: RCOMMAND is incorrect => 0x%02x\n",rcommand);
		// for (i=0;i<len/2;i++)
		// 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -2;
	} else if (crc16_i != crc16_o) {
		// printf("ERROR: Received CRC16 is incorrect => 0x%04x\n",crc16_i);
		// for (i=0;i<len/2;i++)
		// 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -3;
	} else {
		return ack;
	}
}

int send_command_stopdac(int fd) {

	int ret;

	uint16_t tx[2];
	uint16_t rx[NUMELS(tx)+2];
	uint16_t crc16_o, crc16_i;
	uint8_t rcommand;
	uint8_t ack;
	uint32_t len = 2*NUMELS(rx); // 2*sizeof(rx)
	//int i = 0;


	tx[0] = (THE_STOPDAC_COMMAND << 8);
	tx[1] = crc16_uint16_false(tx,NUMELS(tx)-1);

	//printf("Comando STOPDAC\n");

	ret = transfer16(fd, tx, rx, len);

	if (ret==-1) {
		return -1;
	}

	rcommand = (rx[NUMELS(tx)] >> 8) & 0xFF;
	ack = (rx[NUMELS(tx)] & 0x01);
	crc16_i = rx[NUMELS(tx)+1];

	crc16_o = crc16_uint16_false(rx+NUMELS(tx),1);

	if (rcommand != (THE_STOPDAC_COMMAND ^ 0x80)) {
		// printf("ERROR: RCOMMAND is incorrect => 0x%02x\n",rcommand);
		// for (i=0;i<len/2;i++)
		//	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -2;
	} else if (crc16_i != crc16_o) {
		// printf("ERROR: Received CRC16 is incorrect => 0x%04x\n",crc16_i);
		// for (i=0;i<len/2;i++)
		// 	 printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -3;
	} else {
		return ack;
	}
}

int send_command_stopqdec(int fd) {

	int ret;

	uint16_t tx[2];
	uint16_t rx[NUMELS(tx)+2];
	uint16_t crc16_o, crc16_i;
	uint8_t rcommand;
	uint8_t ack;
	uint32_t len = 2*NUMELS(rx); // 2*sizeof(rx)
	//int i = 0;


	tx[0] = (THE_STOPQDEC_COMMAND << 8);
	tx[1] = crc16_uint16_false(tx,NUMELS(tx)-1);

	//printf("Comando STOPQDEC\n");

	ret = transfer16(fd, tx, rx, len);

	if (ret==-1) {
		return -1;
	}

	rcommand = (rx[NUMELS(tx)] >> 8) & 0xFF;
	ack = (rx[NUMELS(tx)] & 0x01);
	crc16_i = rx[NUMELS(tx)+1];

	crc16_o = crc16_uint16_false(rx+NUMELS(tx),1);

	if (rcommand != (THE_STOPQDEC_COMMAND ^ 0x80)) {
		// printf("ERROR: RCOMMAND is incorrect => 0x%02x\n",rcommand);
		// for (i=0;i<len/2;i++)
		//	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -2;
	} else if (crc16_i != crc16_o) {
		// printf("ERROR: Received CRC16 is incorrect => 0x%04x\n",crc16_i);
		// for (i=0;i<len/2;i++)
		// 	 printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -3;
	} else {
		return ack;
	}
}

int send_command_readadc(int fd, int16_t *rx, uint16_t ndata_adc, uint8_t membank) {

	int ret;

	uint16_t tx[2];
	uint16_t crc16_o, crc16_i;
	uint8_t rcommand;
	uint8_t ack;
	uint32_t len = 2*(ndata_adc+NUMELS(tx)+2); // 2*sizeof(rx)

	//printf("\n NUMELS(tx) = %d",NUMELS(tx));
	//int i = 0;

	tx[0] = (THE_READADC_COMMAND << 8) + membank;
	tx[1] = crc16_uint16_false(tx,NUMELS(tx)-1);

	ret = transfer16(fd, tx, rx, len);

	if (ret==-1) {
		return -1;
	}

	rcommand = (rx[NUMELS(tx)] >> 8) & 0xFF;
	ack = (rx[NUMELS(tx)] & 0x01);
	crc16_i = rx[ndata_adc+NUMELS(tx)+1];
	crc16_o = crc16_uint16_false(rx+NUMELS(tx),ndata_adc+1);

	if (rcommand != (THE_READADC_COMMAND ^ 0x80)) {
		// printf("ERROR: RCOMMAND is incorrect => 0x%02x\n",rcommand);
		// for (i=0;i<len/2;i++)
		//	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -2;
	} else if (crc16_i != crc16_o) {
		// printf("ERROR: Received CRC16 is incorrect => 0x%04x\n",crc16_i);
		// for (i=0;i<len/2;i++)
		//	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -3;
	} else {
		return ack;
	}
}

int send_command_readqdec(int fd, int16_t *rx, uint16_t ndata_qdec, uint8_t membank) {

	int ret;

	uint16_t tx[2];
	uint16_t crc16_o, crc16_i;
	uint8_t rcommand;
	uint8_t ack;
	uint32_t len = 2*(ndata_qdec+NUMELS(tx)+2); // 2*sizeof(rx)
	//int i = 0;

	tx[0] = (THE_READQDEC_COMMAND << 8) + membank;
	tx[1] = crc16_uint16_false(tx,NUMELS(tx)-1);

	//printf("Comando READQDEC\n");

	ret = transfer16(fd, tx, rx, len);

	if (ret==-1) {
		return -1;
	}

	rcommand = (rx[2] >> 8) & 0xFF;
	ack = (rx[2] & 0x01);
	crc16_i = rx[ndata_qdec+NUMELS(tx)+1];

	crc16_o = crc16_uint16_false(rx+NUMELS(tx),ndata_qdec+1);

	if (rcommand != (THE_READQDEC_COMMAND ^ 0x80)) {
		// printf("ERROR: RCOMMAND is incorrect => 0x%02x\n",rcommand);
		// for (i=0;i<len/2;i++)
		//	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -2;
	} else if (crc16_i != crc16_o) {
		// printf("ERROR: Received CRC16 is incorrect => 0x%04x\n",crc16_i);
		// for (i=0;i<len/2;i++)
		// 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -3;
	} else {
		return ack;
	}
}

int send_command_wrmdac(int fd, int16_t *data_dac, uint16_t ndata_dac) {

	int ret;

	uint16_t *rx;
	uint16_t crc16_o, crc16_i;
	uint8_t rcommand;
	uint8_t ack;
	uint32_t len = 2*(ndata_dac+4); // 2*sizeof(rx)
	//int i = 0;

	// Mem Allocation for rx
	rx = (uint16_t *) calloc(ndata_dac+4,sizeof(uint16_t));

	data_dac[0] = (THE_WRMDAC_COMMAND << 8);
	data_dac[ndata_dac+1] = crc16_uint16_false(data_dac,ndata_dac+1);

	//printf("Comando WRMDAC\n");

	ret = transfer16(fd, data_dac, rx, len);

	if (ret==-1) {
		return -1;
	}

	rcommand = (rx[ndata_dac+2] >> 8) & 0xFF;
	ack = (rx[ndata_dac+2] & 0x01);
	crc16_i = rx[ndata_dac+3];

	crc16_o = crc16_uint16_false(rx+ndata_dac+2,1);

	free(rx);

	if (rcommand != (THE_WRMDAC_COMMAND ^ 0x80)) {
		// printf("ERROR: RCOMMAND is incorrect => 0x%02x\n",rcommand);
		// for (i=0;i<len/2;i++)f
		// 	printf("rx[%d]=0x%04x\n",i,rx[i])
		return -2;
	} else if (crc16_i != crc16_o) {
		// printf("ERROR: Received CRC16 is incorrect => 0x%04x\n",crc16_i);
		// for (i=0;i<len/2;i++)
		// 	printf("rx[%d]=0x%04x\n",i,rx[i]);
		return -3;
	} else {
		return ack;
	}


}

