
#include "protocol.h"

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

    // printf("omg here we are 1\n");
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("sender: socket");
            continue;
        }
        // printf("omg here we are 2\n");
        // int yes = 1;
        // if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            // perror("setsockopt");
            // exit(1);
        // }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            // printf("omg here we are 3\n");
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

    printf("Bound to socket, about to send requested filename\n");

    int numbytes;

    if ((numbytes = sendto(sockfd, filename, strlen(filename), 0, p->ai_addr, p->ai_addrlen)) == -1) {
        perror("Error: failed to send filename\n");
        exit(1);
    }

    freeaddrinfo(servinfo);

    printf("Sent requested filename, waiting for file\n");
}

