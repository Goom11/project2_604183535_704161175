
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

int receiveAsPacket (int sockfd, char *buf, size_t len, struct sockaddr *srcAddr, socklen_t addrLen, int *seq, int *ack, int *fin){
    char temp[MAXDATALEN + HEADERSIZE];

    bzero(buf, MAXDATALEN + HEADERSIZE);

    int numbytes = recvfrom(sockfd, temp, MAXDATALEN + HEADERSIZE, 0, srcAddr, addrLen);


    //eventually need to add crc
    memcpy(seq, temp, sizeof(int));
    memcpy(ack, temp+4, sizeof(int));
    memcpy(fin, temp+8, sizeof(int));
    memcpy(len, temp+12, sizeof(int));

    memcpy(buf, temp+16, MAXDATALEN);

    return numbytes;
}
