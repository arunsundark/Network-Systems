

build: server.o client.o
	gcc server.c -o server.o -lcrypto -lssl
	gcc client.c -o client.o -lcrypto -lssl


clean: 
	rm -rf *.o
