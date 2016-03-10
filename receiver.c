
#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int max(int a, int b) {
    return (a > b) ? a : b;
}

typedef struct MyBuffer {
    char *buf;
    size_t dataLen;
    size_t bufLen;
} fileBuffer;

fileBuffer allocateNewFileBuffer() {
    return (fileBuffer) { .buf = malloc(MAXDATALEN), .dataLen = 0, .bufLen = MAXDATALEN };
}

void deleteFileBuffer(fileBuffer fb) {
    free(fb.buf);
}

// returns new fileBuffer, not modified old fileBuffer
// also deletes the input fileBuffer so that you don't have to
fileBuffer writeToFileBuffer(char *data, size_t dataLen, size_t position, fileBuffer fb) {
    if (position + dataLen > fb.bufLen) {
        fb.bufLen = fb.bufLen * 2;
        fb.buf = realloc(fb.buf, fb.bufLen);
    }

    fb.dataLen = max(fb.dataLen, position + dataLen);
    memcpy(fb.buf + position, data, dataLen);
    return fb;
}


void exportFileBufferToFile(fileBuffer fb, char *filename) {
    FILE *fp = fopen(filename, "w+");
    if (fp == NULL) {
        fprintf(stderr, "Error: file cannot be created!\n");
        exit(1);
    }
    fwrite(fb.buf, sizeof(char), fb.dataLen, fp);
    fclose(fp);
}

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
        pb.buf = realloc(pb.buf, pb.bufLen);
    }

    pb.packetLen = max(pb.packetLen, position);
    memcpy(pb.buf + position, &packet, sizeof(packet));
    return pb;
}

void exportPacketBufferToFile(packetBuffer fb, char *filename) {
    size_t dataLen = fb.packetLen * MAXDATALEN;
    char *data = malloc(dataLen);
    size_t i;
    for(i = 0; i < fb.packetLen; i++) {
        memcpy(data + i * MAXDATALEN, fb.buf[i].data, fb.buf[i].len);
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

int main(int argc, char *argv[])
{

    if (argc != 4)
    {
        fprintf(stderr,"usage: receiver <hostname> <portnumber> <filename>\n");
        exit(1);
    }

    char *hostname = argv[1];
    char *port = argv[2];
    char *filename = argv[3];

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

    fileBuffer fb = allocateNewFileBuffer();

    fprintf(stderr, "about to recieve file\n");
    while(packet.fin != 1) {
        packet = receivePacket(conn);
        fprintf(stderr, "receieved packet with seq: %d\n", packet.seq);

        if(packet.numbytesValid == 0) {
            fprintf(stderr, "Error: failed to receive file or packet\n");
            exit(1);
        } else if(packet.seq == 0 && packet.fin == 1 && packet.len == 0) {
            fprintf(stderr, "Error: file not found\n");
            exit(1);
        } else {
            packet.ack = 1;
            numbytes = sendPacket(conn, packet,0,0); //NO SIMULATION of packet loss or corruption in sending ACKs for now

            if (numbytes == -1){
                fprintf(stderr, "ACK #%d lost\n", packet.seq);
                packet.fin = 0;
            } else {
                fprintf(stderr, "Sent ACK for packet with seq: %d\n", packet.seq);
                fb = writeToFileBuffer(packet.data, packet.len, packet.seq, fb);
                if (packet.fin == 1){
                    fprintf(stderr, "FIN received and FINACK sent\n");
                }
            }
        }
    }

    char newName[MAXDATALEN];
    strcpy(newName, "new_");
    strcat(newName, filename);

    exportFileBufferToFile(fb, newName);
    deleteFileBuffer(fb);

    return 0;
}



