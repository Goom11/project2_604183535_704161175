
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

typedef struct MySenderConnection {
    int sockfd;
    struct sockaddr_storage theirAddr;
    socklen_t addrLen;
} senderConnection;

senderConnection createSenderConnection(int sockfd);

typedef struct MyReceiverConnection {
    int sockfd;
    struct sockaddr *srcAddr;
    socklen_t *addrLen;
} receiverConnection;

receiverConnection createReceiverConnection(int sockfd, struct sockaddr *srcAddr, socklen_t *addrLen);

void printPacket(protocolPacket packet);

int sendPacket(senderConnection conn, protocolPacket packet);

protocolPacket receivePacket(receiverConnection conn);

#endif
