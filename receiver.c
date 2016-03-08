
#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    size_t packetSize = sizeof(packet);

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
    char* fileBuf = malloc(MAXDATALEN);

    while(packet.fin != 1) {
        packet = receivePacket(conn);
        printPacket(packet);

        if(packet.numbytes == -1) {
            fprintf(stderr, "Error: failed to receive file or packet\n");
            exit(1);
        } 

        else if(packet.seq == 0 && packet.fin == 1) {
            fprintf(stderr, "Error: file not found\n");
            exit(1);
        }

        // corruption case goes here

        else if (packet.seq < tracker - MAXDATALEN || packet.seq > MAXDATALEN){
            printf("Packet out of order - ignore!\n");
            packet.fin = 0;
        }

        else {
            packet.ack = 1;
            numbytes = sendPacket(conn, packet);

            if (numbytes == -1){
                printf("ACK #%d lost\n", packet.seq);
                packet.fin = 0;
            }
            if (numbytes == -2){
                //corruption case
                packet.fin = 0;
            }

            else{
                printf("Sent ACK #%d\n", packet.seq);
                memcpy(fileBuf+packet.seq, packet.data, packet.len);
                tracker +=MAXDATALEN
                if (packet.fin == 1){
                    printf("FIN received\n");
                    printf("FINACK sent\n");
                }
            }
        }
    }
    fprintf(stderr, "%s", fileBuf);

    fclose(fp);
    free(fileBuf);
}



