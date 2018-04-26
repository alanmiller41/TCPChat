#
# Makefile for the C client server programs
#


all: client server

client: client.c
	gcc -o client client.c

server: server.c
	gcc -o  server server.c

clean:
	 rm -f client server 
