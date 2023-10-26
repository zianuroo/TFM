KERNEL_SOURCES := /lib/modules/$(shell uname -r)/build

obj-m += mbkmodule.o

all:
	make -s -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -s -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
