
#include "protocol.h"

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

int sendAsPacket(int sockfd, char *data, size_t dataLen, struct sockaddr *destAddr, socklen_t addrLen, int seq, int ack, int fin) {
    protocolPacket packet = createPacket(seq, ack, fin, data, dataLen);
    size_t packetSize = sizeof(packet);
    char *tempBuf = malloc(packetSize);
    memset(tempBuf, 0, packetSize);
    memcpy(tempBuf, (char *)&packet, packetSize);
    int numbytes = sendto(sockfd, tempBuf, packetSize, 0, destAddr, addrLen);
    free(tempBuf);
    return numbytes;
}







