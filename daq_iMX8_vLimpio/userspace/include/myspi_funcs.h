/*
 * myspi_funcs.h
 *
 *  Created on: Jan 19, 2018
 *      Author: airizar
 */

#ifndef MYSPI_FUNCS_H_
#define MYSPI_FUNCS_H_

#include <stdint.h>

int transfer16(int fd, uint16_t *tx, uint16_t *rx, uint32_t len);
int configure_spi(char *device, int *fd);
int send_command_inidac(int fd, uint8_t ndiv_dac, uint16_t ndata_dac, uint16_t control_dac);
int send_command_iniadc(int fd, uint8_t enchan_adc, uint16_t convsttime_adc, uint8_t sclktime_adc, uint8_t os_adc, uint16_t ndata_adc,
						uint16_t nblock_adc, uint8_t powermode_adc, uint8_t resetmode_adc, uint8_t range_adc, uint8_t parn_adc);
int send_command_iniqdec(int fd, uint8_t enchan_qdec, uint16_t ndata_qdec, uint16_t ndiv_qdec);
int send_command_readadc(int fd, int16_t *rx, uint16_t ndata_adc, uint8_t membank);
int send_command_readqdec(int fd, int16_t *rx, uint16_t ndata_qdec, uint8_t membank);
int send_command_startclk(int fd);
int send_command_startdac(int fd);
int send_command_startqdec(int fd);
int send_command_stopclk(int fd);
int send_command_stopdac(int fd);
int send_command_stopqdec(int fd);
int send_command_wrmdac(int fd, int16_t *data_dac, uint16_t ndata_dac);

#endif /* MYSPI_FUNCS_H_ */




