
#ifndef THREADMAIN_H_
#define THREADMAIN_H_

#include <pthread.h>

// Estructura para pasar argumentos a los threads
struct tARGS{
	int argc;
	char **argv;
};

// Mutex para sincronizacion de threads
extern pthread_mutex_t data_adc_lock;
extern pthread_mutex_t adc_reading_lock;
extern pthread_mutex_t connection_lock;  
extern pthread_mutex_t M_lock;
extern pthread_mutex_t nblock_adc_lock;
extern pthread_mutex_t enchan_adc_lock;
extern pthread_mutex_t param_lock;
extern pthread_mutex_t read_adc_buffer_lock;    

#endif /* THREADMAIN_H_ */
