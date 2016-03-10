
#include "protocol.h"

void printPacket(protocolPacket packet) {
    if (packet.numbytesValid == 0) {
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
    packet.crc = 0;         //by default, packet is not corrupt
    memcpy(packet.data, data, dataLen);
    packet.numbytesValid = 1;
    return packet;
}

connection createConnection(int sockfd, struct sockaddr *addr, socklen_t *addrLen) {
    return (connection) { .sockfd = sockfd, .addr = addr, .addrLen = addrLen };
}

int sendPacket(connection conn, protocolPacket packet, double pl, double pc) {

    int lossRate = (int)(pl*100);
    int corruptionRate = (int)(pc*100);
    int lost = ((rand()%100+1)<=lossRate);
    int corrupt = ((rand()%100+1)<=corruptionRate);

    if (lost){
        return -1;
    }

    if (corrupt){
        packet.crc = 1;
        sendto(
            conn.sockfd,
            (char *)&packet,
            sizeof(packet),
            0,
            (struct sockaddr *)conn.addr,
            *(conn.addrLen));
        return -2;
    }

    return sendto(
            conn.sockfd,
            (char *)&packet,
            sizeof(packet),
            0,
            (struct sockaddr *)conn.addr,
            *(conn.addrLen));
}

protocolPacket receivePacket(connection conn){
    protocolPacket packet;
    int numbytes = recvfrom(conn.sockfd, (char *)&packet, sizeof(packet), 0, conn.addr, conn.addrLen);
    if (numbytes == -1) {
        packet.numbytesValid = 0;
    }
    return packet;
}
