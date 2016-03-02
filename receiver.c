
#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAXLEN 512

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

    if ((numbytes = sendto(sockfd, filename, strlen(filename), 0, p->ai_addr, p->ai_addrlen)) == -1) {
        perror("Error: failed to send filename\n");
        exit(1);
    }

    freeaddrinfo(servinfo);

    printf("Sent requested filename, waiting for file\n");


    // for receive call, create zeroed packet
    protocolPacket packet;
    memset(&packet, 0, sizeof(packet));
    packet.fin = 0;
    size_t packetSize = sizeof(packet);

    //empty file to copy data to
    char newName[MAXLEN];
    strcpy(newName, "new_");
    strcat(newName, argv[3]);

    FILE *fp = fopen(newName, "w+");

    if (fp == NULL){
        printf("Error: new file cannot be created.");
        exit(1);
    }

    while(packet.fin != 1) {
        packet = receivePacket(sockfd, p->ai_addr, &(p->ai_addrlen));
        printPacket(packet);

        if(packet.numbytes == -1) {
            fprintf(stderr, "Error: failed to receive file or packet\n");
            exit(1);
        } else if(packet.seq == 0 && packet.fin == 1) {
            fprintf(stderr, "Error: file not found\n");
            exit(1);
        }
    }
}

