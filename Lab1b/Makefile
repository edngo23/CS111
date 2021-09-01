#NAME: Ethan Ngo
#EMAIL: ethan.ngo2019@gmail.com
#ID: 205416130

default:
	gcc -Wall -Wextra -g -o lab1b-client -lz lab1b-client.c
	gcc -Wall -Wextra -g -o lab1b-server -lz lab1b-server.c

client:
	gcc -Wall -Wextra -g -o lab1b-client -lz lab1b-client.c

server:
	gcc -Wall -Wextra -g -o lab1b-server -lz lab1b-server.c

clean:
	rm -f lab1b-client lab1b-server *.tar.gz

dist:
	tar -czvf lab1b-205416130.tar.gz lab1b-client.c lab1b-server.c Makefile README