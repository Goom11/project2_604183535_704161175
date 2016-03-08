all:
	gcc -o sender -g sender.c protocol.c
	gcc -o receiver -g receiver.c protocol.c
