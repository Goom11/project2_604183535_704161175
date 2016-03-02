
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

typedef struct MyPacket {
    int seq;
    int ack;
    int fin;
    int len;
    char data[MAXDATALEN];
    size_t numbytes;
} protocolPacket;

protocolPacket createPacket(int seq, int ack, int fin, char *data, size_t dataLen);

protocolPacket createFileNotFoundPacket();

void printPacket(protocolPacket packet);

int sendPacket(int sockfd, struct sockaddr *destAddr, socklen_t addrLen, protocolPacket packet);

protocolPacket receivePacket(int sockfd, struct sockaddr *srcAddr, socklen_t *addrLen);

#endif
