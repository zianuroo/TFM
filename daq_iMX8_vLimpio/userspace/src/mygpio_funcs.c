/*
 * mygpio_funcs.c
 *
 *  Created on: Jan 19, 2018
 *      Author: airizar
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_BUF 64

/*
 * int conf_gpio(int npin, int dir)
 * Inputs
 * 	 dir : direccion ("in" (0), "out" (1))
 * Output
 * 	 fd : descriptor de /sys/class/gpio/gpioX/value
 */
int conf_gpio(int gpio, int dir)
{

	int exportfd, directionfd;
	char buf[50];
	unsigned int length;
	int res;
	//int ret;

	/* The GPIO has to be exported to be able to see it in sysfs */

	exportfd = open("/sys/class/gpio/export", O_WRONLY);
	if (exportfd < 0)
	{
		printf("Cannot open GPIO %d to export it\n",gpio);
		//ret = exportfd;
	}

	sprintf(buf, "%d", gpio);

	length = strlen(buf);
	write(exportfd, buf, length);

	close(exportfd);

	// printf("GPIO %d exported successfully\n",gpio);

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
	directionfd = open(buf, O_WRONLY);
	if (directionfd < 0)
	{
		printf("Cannot open GPIO direction\n");
		//ret = directionfd;
	}

	if (dir==0)
		res = write(directionfd, "in", 2);
	else
		res = write(directionfd, "out", 3);

	if (dir==0)
		if (res!=2)
			printf("Direction value of pin %d not written\n",gpio);
		else {}
			// printf("GPIO %d direction set successfully\n",gpio);
	else
		if (res!=3)
			printf("Direction value of pin %d not written\n",gpio);
		else {}
			// printf("GPIO %d direction set successfully\n",gpio);

	close(directionfd);

	return 0;

}

int conf_gpio_interrupt(int gpio, char *edge) {
	// Se anade gpio_set_edge, el cual nos va a permitir hacer polling sobre el
	// GPIO hasta que los datos esten listos

	int fd;
	char buf[50];
	//int ret = 0;

	sprintf(buf, "/sys/class/gpio/gpio%d/edge", gpio);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		printf("Edge value of pin %d not written\n",gpio);
		//ret = fd;
	}
	write(fd, edge, strlen(edge)+1);
	//printf("Edge value of pin %d written with value %s\n",gpio, edge);
	close(fd);

	return 0;
}
/*
 * int close_gpio(int fd)
 * Cierra el descriptor de /sys/class/gpio/gpioX/value
 * Inputs
 * 	 fd : numero de pin (bank-1)*32+IOn
 * Output
 *   res : Siempre 0
 */
int close_gpio(int fd) {

	close(fd);
	return 0;
}

/*
 * int unconf_gpio_pin(int npin)
 * Hace un unexport del pin en cuestion
 * Inputs
 * 	 npin : descriptor de /sys/class/gpio/gpioX/value
 * Output
 *   res : 0 (Success), Negatiove value (Error)
 */
int unconf_gpio(int gpio)
{

	int unexportfd;
	char buf[50];
	unsigned int length;
	int ret = 0;

	unexportfd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (unexportfd < 0)
	{
		printf("Cannot open GPIO %d to unexport it\n",gpio);
		ret = unexportfd;
	}

	sprintf(buf, "%d", gpio);

	length = strlen(buf);
	write(unexportfd, buf, length);

	close(unexportfd);

	// printf("GPIO %d unexported successfully\n",gpio);

	return ret;

}


/* int write_gpio(int fd, int val)
 * Escribe el valor val en el pin del descriptor fd
 * Inputs:
 * 	 fd: descriptor de /sys/class/gpio/gpioX/value
 *   val : valor del pin (0 o 1)
 * Output:
 *   Sucesss (0), Error (<0)
 */
int write_gpio(int gpio, int val)
{
	char buf[50];
	int fd;

	//int res;

	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
	fd = open(buf, O_WRONLY);
	if (fd<0) {
		perror("gpio/write-value");
		return fd;
	}

	if (val==0)
		write(fd, "0", 2);
	else
		write(fd, "1", 2);

	// if (res != 1) {
	//	printf("Value not written in GPIO%d, res = %d\n",fd,res);
	// 	ret = -1;
	// }
	close(fd);

	return 0;

}

/* int read_gpio(int fd)
 * Lee el valor val en el pin del descriptor fd
 * Inputs:
 * 	 fd: descriptor de /sys/class/gpio/gpioX/value
 * Output:
 *   Valor leido (0, 1, <0 si Error)
 */
int read_gpio(int gpio, int *val)
{

	char cvalue, buf[50];
	int fd;
	//int value;
	//int res;

	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
	fd = open(buf, O_RDONLY);
	if (fd<0) {
		perror("gpio/read-value");
		return fd;
	}

	read(fd, &cvalue, 1);

	if (cvalue=='0')
		*val = 0;
	else
		*val = 1;


	close(fd);

	return 0;

}

int gpio_fd_open(unsigned int gpio)
{
	int fd;
	//int len;
	char buf[MAX_BUF];

	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
	//len = strlen(buf);

	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		perror("gpio/fd_open");
	}
	//printf("fd: %d", fd);
	return fd;
}



