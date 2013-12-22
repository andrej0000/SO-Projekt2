CC = gcc
CCFLAGS = -pthread -Wall

ifeq ($(debug), 1)
	CCFLAGS += -g -DDEBUG
endif

ALL: serwer klient

err.o: err.h err.c
	$(CC) $(CCFLAGS) err.c -c -o err.o

klient.o: err.o serwer.c msg.h
	$(CC) $(CCFLAGS) klient.c -c -o klient.o

klient: err.o msg.h klient.o
	$(CC) $(CCFLAGS) err.o klient.o msg.h -o klient

serwer.o: err.o serwer.c msg.h
	$(CC) $(CCFLAGS) serwer.c -c -o serwer.o

serwer: err.o msg.h serwer.o
	$(CC) $(CCFLAGS) err.o serwer.o msg.h -o serwer

clean:
	rm *.o
