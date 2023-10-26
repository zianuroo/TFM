/*
 * mygpio_funcs.h
 *
 *  Created on: Jan 19, 2018
 *      Author: airizar
 */

#ifndef MYGPIO_FUNCS_H_
#define MYGPIO_FUNCS_H_

int conf_gpio(int gpio, int dir);
int conf_gpio_interrupt(int gpio, char *edge);
int gpio_fd_open(unsigned int gpio);
int unconf_gpio(int gpio);
int close_gpio(int fd);
int write_gpio(int gpio, int val);
int read_gpio(int gpio, int *val);


#endif /* MYGPIO_FUNCS_H_ */



