
#include "protocol.h"

void printPacket(protocolPacket packet) {
    if (packet.numbytes < 0) {
        printf("Invalid packet, cannot print\n");
        return;
    }
    printf("Packet:\n");
    printf("seq: %d\n", packet.seq);
    printf("ack: %d\n", packet.ack);
    printf("fin: %d\n", packet.fin);
    printf("len: %d\n", packet.len);
    printf("data: %*.*s\n", packet.len, packet.len, packet.data);
    printf("numbytes: %zd\n", packet.numbytes);
    return;
}

protocolPacket createFileNotFoundPacket() {
    return createPacket(0, 0, 1, NULL, 0);
}

protocolPacket createPacket(int seq, int ack, int fin, char *data, size_t dataLen) {
    protocolPacket packet;
    memset(&packet, 0, sizeof(packet));
    packet.seq = seq;
    packet.ack = ack;
    packet.fin = fin;
    packet.len = dataLen;
    memcpy(packet.data, data, dataLen);
    return packet;
}

connection createConnection(int sockfd, struct sockaddr *addr, socklen_t *addrLen) {
    return (connection) { .sockfd = sockfd, .addr = addr, .addrLen = addrLen };
}

receiverConnection createReceiverConnection(int sockfd, struct sockaddr *srcAddr, socklen_t *addrLen) {
    return (receiverConnection) { .sockfd = sockfd, .srcAddr = srcAddr, .addrLen = addrLen };
}

int sendPacket(connection conn, protocolPacket packet) {
    size_t packetSize = sizeof(packet);
    char *tempBuf = malloc(packetSize);
    memset(tempBuf, 0, packetSize);
    memcpy(tempBuf, (char *)&packet, packetSize);
    int numbytes = sendto(conn.sockfd, tempBuf, packetSize, 0, (struct sockaddr *)conn.addr, *(conn.addrLen));
    free(tempBuf);
    return numbytes;
}

protocolPacket receivePacket(receiverConnection conn){
    protocolPacket packet;
    size_t packetSize = sizeof(packet);
    char *tempBuf = malloc(packetSize);
    memset(tempBuf, 0, packetSize);
    int numbytes = recvfrom(conn.sockfd, tempBuf, packetSize, 0, conn.srcAddr, conn.addrLen);
    memcpy((char *)&packet, tempBuf, packetSize);
    if (numbytes == -1) {
        packet.numbytes = -1;
    }
    free(tempBuf);
    return packet;
}
