all:
	gcc -o sender -g -Wall -Wextra sender.c protocol.c
	gcc -o receiver -g -Wall -Wextra receiver.c protocol.c
