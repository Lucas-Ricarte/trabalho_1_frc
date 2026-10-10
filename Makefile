CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c99 -D_POSIX_C_SOURCE=200809L -Iinclude
TARGET  = dns_client

SRCS    = $(wildcard src/*.c)
OBJS    = $(SRCS:.c=.o)
HEADERS = $(wildcard include/*.h)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
