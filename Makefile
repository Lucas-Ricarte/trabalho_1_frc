# Makefile - Cliente DNS (Trabalho 01)
# Compila o cliente DNS modularizado.

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99
TARGET  = dns_client

SRCS    = clienteDNS.c dns_encode.c dns_parse.c
OBJS    = $(SRCS:.c=.o)
HEADERS = dns_types.h dns_encode.h dns_parse.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
