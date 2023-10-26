
#include <stdlib.h>


extern void dataSave(uint8_t enchan_adc_client, uint8_t nchan_adc_client, int16_t M_client, uint16_t ndata_adc_client);
extern uint8_t number_chan(uint8_t enchan);
extern uint8_t chan_no(uint8_t enchan);
extern void printValues(int received_block, uint16_t ndata_adc_client);
extern void printDemoValues(uint8_t nchan_adc_client, int received_block, uint16_t ndata_adc_client);