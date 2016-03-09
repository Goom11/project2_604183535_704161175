
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

fileBuffer copyFileBuffer(fileBuffer fb) {
    char *buf = malloc(fb.bufLen);
    memcpy(buf, fb.buf, fb.bufLen);
    return (fileBuffer) { .buf = buf, .dataLen = fb.dataLen, .bufLen = fb.bufLen };
}

void deleteFileBuffer(fileBuffer fb) {
    free(fb.buf);
}

// returns new fileBuffer, not modified old fileBuffer
// also deletes the input fileBuffer so that you don't have to
fileBuffer writeToFileBuffer(char *data, size_t dataLen, size_t position, fileBuffer fb) {
    fileBuffer newFb = copyFileBuffer(fb);
    deleteFileBuffer(fb);

    if (position + dataLen > newFb.bufLen) {
        newFb.bufLen = newFb.bufLen * 2;
        newFb.buf = realloc(newFb.buf, newFb.bufLen);
    }
    newFb.dataLen = max(newFb.dataLen, position + dataLen);
    memcpy(newFb.buf + position, data, dataLen);
    return newFb;
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

    printf("Bound to socket, about to send requested filename\n");

    int numbytes;
    connection conn = createConnection(sockfd, p->ai_addr, &(p->ai_addrlen));

    if ((numbytes = sendto(conn.sockfd, filename, strlen(filename), 0, conn.addr, *(conn.addrLen))) == -1) {
        perror("Error: failed to send filename\n");
        exit(1);
    }

    freeaddrinfo(servinfo);

    printf("Sent requested filename, waiting for file\n");


    // for receive call, create zeroed packet
    protocolPacket packet;
    memset(&packet, 0, sizeof(packet));
    packet.fin = 0;
    // size_t packetSize = sizeof(packet);

    //empty file to copy data to
    char newName[MAXDATALEN];
    strcpy(newName, "new_");
    strcat(newName, argv[3]);

    FILE *fp = fopen(newName, "w+");

    if (fp == NULL){
        printf("Error: new file cannot be created.");
        exit(1);
    }

    int tracker = 0;
    fileBuffer fb = allocateNewFileBuffer();

    printf("about to recieve file\n");
    while(packet.fin != 1) {
        printf("not finished ofc\n");
        packet = receivePacket(conn);
        printf("receieved packet: \n");
        printPacket(packet);

        if(packet.numbytesValid == 0) {
            fprintf(stderr, "Error: failed to receive file or packet\n");
            exit(1);
        } else if(packet.seq == 0 && packet.fin == 1 && packet.len == 0) {
            fprintf(stderr, "Error: file not found\n");
            exit(1);
        } else {
            packet.ack = 1;
            numbytes = sendPacket(conn, packet);

            if (numbytes == -1){
                printf("ACK #%d lost\n", packet.seq);
                packet.fin = 0;
            } else {
                printf("Sent ACK #%d\n", packet.seq);
                fb = writeToFileBuffer(packet.data, packet.len, packet.seq, fb);
                tracker +=MAXDATALEN;
                if (packet.fin == 1){
                    printf("FIN received\n");
                    printf("FINACK sent\n");
                }
            }
        }
    }
    fprintf(stderr, "%s", fb.buf);

    fclose(fp);
    deleteFileBuffer(fb);

    return 0;
}



