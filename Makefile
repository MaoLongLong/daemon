all: daemon

daemon: daemon.c
	$(CC) -o daemon daemon.c -Wall -W -pedantic -std=c99

clean:
	rm daemon
