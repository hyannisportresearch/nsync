CC=gcc
CFLAGS=-g -Werror -Wextra -Wall -std=gnu99
DEPS=%.h

%.o:%.c $(DEPS)
	@$(CC) -c -o $@ $< $(CFLAGS)

all: nsync

nsync: nsync_driver.o nsync_centos_parse.o nsync_centos.o nsync_ubuntu_parse.o nsync_ubuntu.o nsync_utils.o
	@$(CC) -o nsync nsync_driver.o nsync_centos_parse.o nsync_centos.o nsync_ubuntu_parse.o nsync_ubuntu.o nsync_utils.o

clean: 
	@rm *.o
	@rm nsync
