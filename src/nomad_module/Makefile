obj-m += async_promote.o
async_promote-objs := async_promote_main.o buffer_ring.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean