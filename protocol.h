
#ifndef protocol
#define protocol

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAXDATALEN 512
#define HEADERSIZE 20

int min(int a, int b);
int max(int a, int b);

typedef struct MyPacket {
    int seq;
    int ack;
    int fin;
    int len;
    int crc;
    char data[MAXDATALEN];
    size_t numbytes;
    int numbytesValid;
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

int sendPacket(connection conn, protocolPacket packet, double pl, double pc);

protocolPacket receivePacket(connection conn);

#endif
