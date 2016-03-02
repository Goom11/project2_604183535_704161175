
#include "protocol.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr,"usage: sender <portnumber>\n");
        // fprintf(stderr,"usage: sender <portnumber> <CWnd> <Pl> <Pc>\n");
        exit(1);
    }

    char *port = argv[1];

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *servinfo;
    int rv;

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    struct addrinfo *p;
    int sockfd;

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            fprintf(stderr, "sender: socket");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            fprintf(stderr, "sender: bind");
            continue;
        }
        break;
    }

    if (p == NULL){
        fprintf(stderr, "Error: sender failed to bind socket");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("Bound to socket, now waiting for requested filename\n");

    struct sockaddr_storage theirAddr;
    socklen_t addrLen = sizeof(theirAddr);
    int numbytes;
    char buf[MAXDATALEN];

    if ((numbytes = recvfrom(sockfd, buf, MAXDATALEN-1, 0, (struct sockaddr *)&theirAddr, &addrLen)) == -1) {
        fprintf(stderr, "Error: failed to receive filename\n");
        exit(1);
    }

    buf[numbytes] = '\0';
    printf("The requested filename is: %s\n", buf);

    FILE *fp = fopen(buf, "r");
    if (fp == NULL) {
        int rv = sendPacket(sockfd, (struct sockaddr *)&theirAddr, addrLen, createFileNotFoundPacket());
        fprintf(stderr, "Error: file not found\n");
        exit(1);
    }

}


