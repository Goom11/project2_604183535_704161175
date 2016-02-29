
#ifndef protocol
#define protocol

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAXDATALEN 512
#define HEADERSIZE 20

int sendAsPacket(int sockfd, char *buf, size_t len, struct sockaddr *destAddr, socklen_t addrLen, int seq, int ack, int fin);

int receiveAsPacket (int sockfd, char *buf, size_t len, struct sockaddr *srcAddr, socklen_t addrLen, int *seq, int *ack, int *fin);

typedef struct MyPacket {
    int seq;
    int ack;
    int fin;
    int len;
    char data[MAXDATALEN];
} protocolPacket;

#endif
