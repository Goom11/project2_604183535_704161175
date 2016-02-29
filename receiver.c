
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

    if ((numbytes = sendto(sockfd, filename, strlen(filename), 0, p->ai_addr, p->ai_addrlen)) == -1) {
        perror("Error: failed to send filename\n");
        exit(1);
    }

    freeaddrinfo(servinfo);

    printf("Sent requested filename, waiting for file\n");


    // for receive call
    int seq = 0;
    int ack = 0;
    int fin = 0;
    int crc = 0;

    //empty file to copy data to
    char newName = [MAXLEN];
    strcpy(newName, "new_");
    strcat(newName, argv[3]);

    FILE *fp = fopen(newName, "w+");

    if (fp = null){
        printf("Error: new file cannot be created.");
        exit(1);
    }
}

