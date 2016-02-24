
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

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
    for(p = servinfo; p != NUL; p = p->ai.next) {
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

    printf("Waiting for requested filename...\n");

}


