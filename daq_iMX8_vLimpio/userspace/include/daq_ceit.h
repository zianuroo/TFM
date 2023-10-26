/*
 * daq_ceit.h
 *
 *  Created on: May 19, 2022
 *      Author: airizar
 */

#ifndef DAQ_CEIT_H_
#define DAQ_CEIT_H_

#define THE_INIDAC_COMMAND 		0x01
#define THE_INIADC_COMMAND 		0x02
#define THE_READADC_COMMAND		0x03
#define THE_STOPCLK_COMMAND		0x04
#define THE_STOPDAC_COMMAND		0x05
#define THE_STARTDAC_COMMAND	0x06
#define THE_STARTQDEC_COMMAND	0x07
#define THE_STOPQDEC_COMMAND	0x08
#define THE_STARTCLK_COMMAND	0x09
#define THE_WRMDAC_COMMAND		0x0A
#define THE_READQDEC_COMMAND	0x0C
#define THE_STARTALL_COMMAND	0x0D
#define THE_INIQDEC_COMMAND		0x0E
#define THE_STOPALL_COMMAND		0x0F

#define POWER_MODE_ADC_NORMAL	0x0
#define POWER_MODE_ADC_STBY		0x1
#define POWER_MODE_ADC_SHDOWN	0x3

#define RESET_MODE_ADC_NONE		0x0
#define RESET_MODE_ADC_PARTIAL	0x1
#define RESET_MODE_ADC_FULL		0x3

#define ZERO_SCALE_DAC			0x0
#define MID_SCALE_DAC			0x1
#define	FULL_SCALE_DAC			0x3
#define	OVR_DISABLE_DAC			0x0
#define	OVR_ENABLE_DAC			0x1
#define TWOSC_DAC				0x1
#define BINARY_DAC				0x0
#define BIP10_DAC				0x0
#define SE10_DAC				0x1
#define BIP5_DAC				0x2
#define SE5_DAC					0x3
#define BIP3_DAC				0x5
#define SE16_DAC				0x6
#define SE20_DAC				0x7
#define BIP2p5_DAC				0x4
#define ETS_DAC					0x1
#define IRO_DAC					0x1



#endif /* DAQ_CEIT_H_ */
