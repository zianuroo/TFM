/*
 * crc16.h
 *
 *  Created on: May 6, 2022
 *      Author: airizar
 */
#define CRC16_DNP	0x3D65		// DNP, IEC 870, M-BUS, wM-BUS, ...
#define CRC16_CCITT	0x1021		// X.25, V.41, HDLC FCS, Bluetooth, ...

//Other polynoms not tested
#define CRC16_IBM	0x8005		// ModBus, USB, Bisync, CRC-16, CRC-16-ANSI, ...
#define CRC16_T10_DIF	0x8BB7		// SCSI DIF
#define CRC16_DECT	0x0589		// Cordeless Telephones
#define CRC16_ARINC	0xA02B		// ACARS Aplications

#include <stdint.h>

uint16_t crc16(uint16_t crcValue, unsigned char newByte);
uint16_t crc16_uint16_false(uint16_t *data, int len);
