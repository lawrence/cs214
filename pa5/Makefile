CC = gcc
CFLAGS = -g -pedantic -Wall -std=gnu99 -O3
LIBS = -lpthread
COMPILE = $(CC) $(CFLAGS)
PROGRAMS = client server

all: $(PROGRAMS)

server: server.c server.h
	$(COMPILE) -o $@ $< $(LIBS)
client: client.c client.h
	$(COMPILE) -o $@ $< $(LIBS)

clean:
	rm -rfv *.o $(PROGRAMS)
