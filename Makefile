CONFIG_MODULE_SIG=n

obj-m += monice.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

ccflags-y += -Wno-error=implicit-function-declaration

