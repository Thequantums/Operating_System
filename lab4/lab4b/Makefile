default:
	gcc -Wall -Wextra -lmraa -lm lab4b.c -o lab4b
	
dist:
	 tar -czvf lab4b-405147924.tar.gz lab4b.c Makefile README check.sh

clean: 
	rm -f lab4b *.tar.gz *.txt

check: default
	chmod +x check.sh
	./check.sh
