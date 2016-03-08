
#include "protocol.h"

int min(int a, int b) {
    return (a < b) ? a : b;
}

int numberOfPacketsLeftInFile(size_t sourceLen, int currentPosition) {
    // round up
    return (sourceLen - currentPosition + (MAXDATALEN - 1)) / MAXDATALEN;
}

int verifyAndLoadFile(char buf[MAXDATALEN], char **source) {
    FILE *fp = fopen(buf, "r");
    int sourceLen = -1;

    if (fp == NULL) {
        fprintf(stderr, "Error: file not found\n");
        return -1;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }

    long fileSize = ftell(fp);

    // file size error
    if (fileSize == -1){
        fprintf(stderr, "Error: file size error\n");
        fclose(fp);
        return -1;
    }

    source = malloc(sizeof(char) * fileSize + 1);

    // file size error
    if (fseek(fp, 0L, SEEK_SET) != 0){
        fprintf(stderr, "Error: file size error\n");
        free(source);
        fclose(fp);
        return -1;
    }

    sourceLen = fread(source, sizeof(char), fileSize, fp);

    // file reading error
    if(sourceLen == 0){
        fprintf(stderr, "Error: file reading error\n");
        free(source);
        fclose(fp);
        return -1;
    }

    source[sourceLen] = '\0';

    fclose(fp);
    return sourceLen;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr,"usage: sender <portnumber> <CWnd>\n");
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

    struct sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);
    connection conn = createConnection(sockfd, (struct sockaddr *)&addr, &addrLen);
    // senderConnection conn = createSenderConnection(sockfd);

    int numbytes;
    char buf[MAXDATALEN];

    if ((numbytes = recvfrom(conn.sockfd, buf, MAXDATALEN-1, 0, (struct sockaddr *)conn.addr, conn.addrLen)) == -1) {
        fprintf(stderr, "Error: failed to receive filename\n");
        exit(1);
    }

    buf[numbytes] = '\0';
    printf("The requested filename is: %s\n", buf);


    char *source = NULL;
    int loadFileRV = verifyAndLoadFile(buf, &source);

    if (loadFileRV == -1) {
        sendPacket(conn, createFileNotFoundPacket());
        exit(1);
    }
    size_t sourceLen = loadFileRV;

    int finished = 0;
    int currentPosition = 0;
    int cwnd = atoi(argv[2]);

    protocolPacket packet;

    memset(&packet, 0, sizeof(packet));
    packet.seq = 0;
    packet.ack = 0;
    packet.fin = 0;
    packet.len = 0;

    while (finished != 1) {
        // sendMultiplePackets
        int numberOfPacketsToSend = min(cwnd, numberOfPacketsLeftInFile(sourceLen, currentPosition));
        int pi;
        for (pi = 0; pi < numberOfPacketsToSend; pi++) {
            int startingPosition = currentPosition + pi*MAXDATALEN;
            protocolPacket packet = createPacket(
                    startingPosition,
                    0,
                    (startingPosition + MAXDATALEN >= sourceLen),
                    source+startingPosition,
                    min(MAXDATALEN, sourceLen - startingPosition));
            sendPacket(conn, packet);
        }
    }

}


