CFLAGS=-Wall -ggdb3 -Werror -Wno-unused -std=c11
CC=gcc
TARGETS=$(patsubst %.c,%,$(wildcard *.c))

all: $(TARGETS)

clean:
	-rm -rf $(TARGETS)

.PHONY: clean all
