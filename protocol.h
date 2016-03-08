
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

typedef struct MyConnection {
    int sockfd;
    struct sockaddr *addr;
    socklen_t *addrLen;
} connection;

connection createConnection(int sockfd, struct sockaddr *addr, socklen_t *addrLen);

void printPacket(protocolPacket packet);

int sendPacket(connection conn, protocolPacket packet);

protocolPacket receivePacket(connection conn);

#endif
