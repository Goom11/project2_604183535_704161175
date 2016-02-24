
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
    memset(&hints, 0, sizeof hints);
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
            perror("sender: socket");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("sender: bind");
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

    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;
    int numbytes;
    char buf[MAXBUFLEN];

    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        fprintf(stderr, "Error: failed to receive filename\n");
        exit(1);
    }

    buf[numbytes] = '\0';
    printf("The requested filename is: %s\n", buf);
}


