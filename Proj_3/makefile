# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Targets
TARGETS = client server servicemap

all: $(TARGETS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c

server: server.c
	$(CC) $(CFLAGS) -o server server.c

servicemap: servicemap.c
	$(CC) $(CFLAGS) -o servicemap servicemap.c

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
