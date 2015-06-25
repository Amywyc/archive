#!/bin/bash

TAR = smartControl datanode
CFLAGES = -WALL -g
LIB = -lpthread

all:$(TAR)

smartControl: smartControl.o
	gcc -o $@ $^ $(CFLAGES) $(LIB)

datanode: datanode.o
	gcc -o $@ $^ $(CFLAGES) $(LIB)

.PHONY :clean
clean:
	rm -rf *.o $(TAR)
