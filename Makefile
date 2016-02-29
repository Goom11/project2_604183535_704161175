all:
	gcc -o sender sender.c protocol.c
	gcc -o receiver receiver.c protocol.c
