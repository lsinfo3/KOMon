MODULE_NAME 	:= udp_probe
SRC_PATH	:= src
SRC 		+= $(SRC_PATH)/main.o $(SRC_PATH)/circBuf.o $(SRC_PATH)/newhash.o
FLAGS		+= -g -DDEBUG

obj-m := $(MODULE_NAME).o
udp_probe-objs := $(SRC)

ccflags-y 	+= ${FLAGS}
CC 		+= ${FLAGS}

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
debug:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules EXTRA_CFLAGS="$(FLAGS)"
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
