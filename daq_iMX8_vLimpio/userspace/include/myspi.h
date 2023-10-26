/*
 * myspi.h
 *
 *  Created on: Jun 15,    2022
 *      Author: airizar
 */

#ifndef MYSPI_H_
#define MYSPI_H_

#include <stdint.h>

extern void* mainRead(void* args);
extern int16_t adc_data[201][8];
extern int adc_reading;

extern int16_t **data_adc;
extern int16_t M; // Number of READADC Commands
extern uint16_t nblock_adc;
extern uint8_t enchan_adc;
extern uint8_t param_read;
extern float fadc;
extern int newnice;
extern uint32_t spi_speed;
extern uint8_t number_chan(uint8_t enchan);
extern int demo;
extern int read_adc_buffer;


#endif /* MYSPI_H_ */
