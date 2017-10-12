obj-m = jp.o

KBUILD_FLAGS += -w

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules $(KBUILD_FLAGS)
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
