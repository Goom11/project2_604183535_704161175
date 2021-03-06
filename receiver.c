
#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct MyPacketBuffer {
    protocolPacket *buf;
    size_t packetLen;
    size_t bufLen;
} packetBuffer;

packetBuffer allocateNewPacketBuffer() {
    return (packetBuffer) { .buf = malloc(sizeof(protocolPacket)), .packetLen = 0, .bufLen = 1 };
}

void deletePacketBuffer(packetBuffer pb) {
    free(pb.buf);
}

packetBuffer addPacketToPacketBuffer(protocolPacket packet, packetBuffer pb) {
    size_t position = packet.seq / MAXDATALEN;
    if (position >= pb.bufLen) {
        pb.bufLen = pb.bufLen * 2;
        pb.buf = realloc(pb.buf, pb.bufLen * sizeof(protocolPacket));
        memset(pb.buf + pb.bufLen / 2, 0, pb.bufLen / 2);
    }

    pb.packetLen = max(pb.packetLen, position + 1);
    memcpy(pb.buf + position, &packet, sizeof(packet));
    return pb;
}

size_t getPacketBufferDataLen(packetBuffer pb) {
    return (pb.packetLen - 1) * MAXDATALEN + pb.buf[pb.packetLen - 1].len;
}

void exportPacketBufferToFile(packetBuffer pb, char *filename) {
    size_t dataLen = getPacketBufferDataLen(pb);
    char *data = malloc(dataLen);
    size_t i;
    for(i = 0; i < pb.packetLen; i++) {
        memcpy(data + i * MAXDATALEN, pb.buf[i].data, pb.buf[i].len);
    }

    FILE *fp = fopen(filename, "w+");
    if (fp == NULL) {
        fprintf(stderr, "Error: file cannot be created!\n");
        exit(1);
    }
    fwrite(data, sizeof(char), dataLen, fp);
    free(data);
    fclose(fp);
}

int allPacketsInPacketBufferHaveBeenAcked(packetBuffer pb) {
    size_t i;
    for (i = 0; i < pb.packetLen; i++) {
        if (pb.buf[i].ack != 1) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[])
{

    if (argc != 6)
    {
        fprintf(stderr,"usage: receiver <hostname> <portnumber> <filename> <Pl> <Pc>\n");
        exit(1);
    }

    char *hostname = argv[1];
    char *port = argv[2];
    char *filename = argv[3];
    double pl = atof(argv[4]);
    double pc = atof(argv[5]);

    if (pl <0 || pc <0 || pl >1 || pc>1){
        fprintf(stderr, "Pl and Pc must both be between 0 and 1\n");
        exit(1);
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo *servinfo;
    int rv;

    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    struct addrinfo *p;
    int sockfd;

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("sender: socket");
            continue;
        }
        break;
    }

    if (p == NULL){
        fprintf(stderr, "Error: sender failed to bind socket");
        return 2;
    }

    fprintf(stderr, "Bound to socket, about to send requested filename\n");

    int numbytes;
    connection conn = createConnection(sockfd, p->ai_addr, &(p->ai_addrlen));

    if ((numbytes = sendto(conn.sockfd, filename, strlen(filename), 0, conn.addr, *(conn.addrLen))) == -1) {
        perror("Error: failed to send filename\n");
        exit(1);
    }

    freeaddrinfo(servinfo);

    fprintf(stderr, "Sent requested filename, waiting for file\n");


    // for receive call, create zeroed packet
    protocolPacket packet;
    memset(&packet, 0, sizeof(packet));
    packet.fin = 0;
    // size_t packetSize = sizeof(packet);

    int finHasBeenReceived = 0;

    packetBuffer pb = allocateNewPacketBuffer();

    fprintf(stderr, "about to recieve file\n");
    while(allPacketsInPacketBufferHaveBeenAcked(pb) == 0 || finHasBeenReceived == 0) {
        packet = receivePacket(conn);
        fprintf(stderr, "receieved packet with seq: %d\n", packet.seq);

        if(packet.numbytesValid == 0) {
            fprintf(stderr, "Error: failed to receive file or packet\n");
            exit(1);
        } else if(packet.seq == 0 && packet.fin == 1 && packet.len == 0) {
            fprintf(stderr, "Error: file not found\n");
            exit(1);
        } else if (packet.crc == 1) {
            fprintf(stderr,"Error: packet is corrupt\n");
            packet.fin = 0; //to make sure that it does not end on a corrupted packet
        } else {
            packet.ack = 1;
            packet.crc = 0;
            numbytes = sendPacket(conn, packet, pl, pc);
            packet.ack = 0;

            if (numbytes == -1){
                fprintf(stderr, "ACK #%d lost\n", packet.seq);
                packet.fin = 0;
            } else if (numbytes == -2) {
                fprintf(stderr, "ACK #%d corrupted\n", packet.seq);
                packet.fin = 0;
                packet.ack = 0;
                pb = addPacketToPacketBuffer(packet, pb);
            } else {
                packet.ack = 1;
                fprintf(stderr, "Sent ACK for packet with seq: %d\n", packet.seq);
                pb = addPacketToPacketBuffer(packet, pb);
                if (packet.fin == 1){
                    finHasBeenReceived = 1;
                    fprintf(stderr, "FIN received and FINACK sent\n");
                }
            }
        }
    }

    char newName[MAXDATALEN];
    strcpy(newName, "new_");
    strcat(newName, filename);

    exportPacketBufferToFile(pb, newName);
    deletePacketBuffer(pb);

    return 0;
}



