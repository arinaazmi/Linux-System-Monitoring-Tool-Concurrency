# Makefile for system monitoring tool 

CC=gcc

## The default target to compile the whole program
all: main

## Rule to create the executable
main: main.c
	$(CC) -o $@ $^


## Clean the project by removing all the .o files
.PHONY: clean
clean:
	rm -f main

.PHONY: help
help:
	@echo "Usage: make [all|clean|help]"
	@echo "all:			Compile the whole program"
	@echo "clean:		Remove all the created files"
	@echo "help:		Display this help message"
