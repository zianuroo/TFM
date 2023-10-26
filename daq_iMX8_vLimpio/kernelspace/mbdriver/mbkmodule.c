/*
 * mbdriver.c
 *
 *  Created on: Jul 1, 2022
 *      Author: airizar
 */

/*
 *
 * Author:   Pierre LE COZ
 *
 * Purposet: Test and demonstrate how to use GPIOs, interrupts and timers in a linux kernel module.
 *
 * Date:     January 2013
 *
 * Informations: This is a linux loadable kernel module.
 * As user space cannot provide access to the GPIO, it is necessary
 * to develop code for the kernel to manage interrupts and GPIO pin values.
 * The cross-compiler produces a .ko file that can be loaded in the kernel using "insmod" command.
 * "rmmod" removes the module from kernel.
 * The module is tested on a ARM developement platform embedding a minimal linux system.
 * A RS-232 to USB cable allows to controle the board from the development computer.
 * The module is build using a cross-toolchaine created with Buildroot.
 *
 */

// EL DRIVER DETECTA CAMBIOS EN GPIO MEMBANK
// El chardev se crea correctamente, ya se puede crear un thread para realizar la lectura de dicho chardev

/*---- Kernel includes ----*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>                 //kmalloc()
#include <linux/uaccess.h>              //copy_to/from_user()
#include <linux/ioctl.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/err.h>
#include <linux/sched/signal.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/hardirq.h>
#include <linux/irqreturn.h>

#ifndef _LINUX_INTERRUPT_H
#define _LINUX_INTERRUPT_H
#endif

#ifndef NULL
#define NULL   ((void *) 0)
#endif

// Define inputs and output pins
#define GPIO_MEMBANK 116
#define SIGMB 44
#define REG_CURRENT_TASK _IOW('a','a',int32_t*)

// Define interrupt request number
#define MEMBANK_IRQ gpio_to_irq(GPIO_MEMBANK)

/* Kernel character device driver /dev/mbdriver. */
static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);

/* Signaling to Application */
static struct task_struct *task = NULL;
static int signum = 0;
int valor = 0;
int i = 0;

/* Called when a process opens accel */
static int device_open(struct inode *inode, struct file *file)
{
	return 0;
}

/* Called when a process closes accel */
static int device_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	printk("REG_CURRENT_TASK %u\n ", cmd);
	if (cmd == REG_CURRENT_TASK) {
		task = get_current();
		signum = SIGMB;
	}
	return 0;
}

// Nombre del device
#define DEVICE_NAME "mbdriver"
static dev_t dev_no = 0;
static struct cdev *mbdriver_cdev = NULL;
static struct class *mbdriver_class = NULL;

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	.unlocked_ioctl = device_ioctl,
	//.write = device_write
};

// Create the interrupt handler
static irqreturn_t my_handler(int irq, void * ident)
{
	struct kernel_siginfo info;
	//Sending signal to app
	memset(&info, 0, sizeof(struct kernel_siginfo));
	info.si_signo = SIGMB;
	info.si_code = SI_QUEUE;
	info.si_int = valor;

	if (task != NULL) {
		//printk(KERN_INFO "Sending signal to app\n");
		if((send_sig_info(SIGMB, &info, task) < 0) && (i == 0)) {
				printk(KERN_INFO "Unable to send signal\n");
				i++;
		}
	}

	if (valor == 1){
		valor = 0;
	}
	else{
		valor = 1;
	}
	return IRQ_HANDLED;
}


// INIT
static int __init membank_init(void){

	int err;
	// SE AÑADE LA CREACION DE UN DEVICE
	/* Get a device number. Get one minor number (0) */
	if ((err = alloc_chrdev_region (&dev_no, 0, 1, DEVICE_NAME)) < 0) {
		printk (KERN_ERR "membank: alloc_chrdev_region() error %d\n", err);
		return err;
	}

	mbdriver_class = class_create (THIS_MODULE, DEVICE_NAME);
	
	mbdriver_cdev = cdev_alloc ();
	mbdriver_cdev->ops = &fops;
	mbdriver_cdev->owner = THIS_MODULE;

	// Add the character device to the kernel
	if ((err = cdev_add (mbdriver_cdev, dev_no, 1)) < 0) {
		printk (KERN_ERR "mbdriver: cdev_add() error %d\n", err);
		return err;
	}
	// Make the device visible for user space in /dev directory
	device_create (mbdriver_class, NULL, dev_no, NULL, DEVICE_NAME );

	// Request GPIO_MEMBANK :
	if ((err = gpio_request(GPIO_MEMBANK, THIS_MODULE->name)) !=0){
		return err;
	}

	// Set GPIO_MEMBANK to input mode :
	if ((err = gpio_direction_input(GPIO_MEMBANK)) !=0){
		gpio_free(GPIO_MEMBANK);
		return err;
	}

	// Request MEMBANK_IRQ:
	if((err = request_irq(gpio_to_irq(GPIO_MEMBANK), my_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, THIS_MODULE->name, THIS_MODULE->name)) != 0){ 
		printk(KERN_INFO "Error %d: could not request irq: %d\n", err, GPIO_MEMBANK);
		gpio_free(GPIO_MEMBANK);
		return err;
	}

	return 0;
}

// Clean up :
static void __exit membank_exit(void)
{
	free_irq(gpio_to_irq(GPIO_MEMBANK), THIS_MODULE->name);
	gpio_free(GPIO_MEMBANK);
	device_destroy (mbdriver_class, dev_no);
	cdev_del (mbdriver_cdev);
	class_destroy (mbdriver_class);
	unregister_chrdev_region (dev_no, 1);
}

module_init(membank_init);
module_exit(membank_exit);

MODULE_IMPORT_NS(vmlinux);
MODULE_AUTHOR("Lander Cia Lasarte");
MODULE_DESCRIPTION("GPIO interrupt module for embedded Linux");
MODULE_LICENSE("GPL");