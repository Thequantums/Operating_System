default:
	gcc -Wall -Wextra -lz lab1b-client.c -o lab1b-client
	gcc -Wall -Wextra -lz lab1b-server.c -o lab1b-server

dist:
	tar -czvf lab1b-405147924.tar.gz lab1b-client.c lab1b-server.c Makefile README

clean:
	rm -f lab1b-server lab1b-client *.tar.gz *.txt *.o