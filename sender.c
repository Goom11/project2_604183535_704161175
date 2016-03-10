
#include "protocol.h"

int min(int a, int b) {
    return (a < b) ? a : b;
}

int numberOfPacketsLeftInFile(size_t sourceLen, size_t currentPosition) {
    // round up
    return (sourceLen - currentPosition + (MAXDATALEN - 1)) / MAXDATALEN;
}

int getPacketIndex(protocolPacket packet, size_t currentPosition) {
    return (packet.seq - currentPosition) / MAXDATALEN;
}

int numberOfConsecutivelyReceivedPackets(int *window, int cwnd) {
    int num = 0;
    int i;
    for(i = 0; i < cwnd; i++) {
        if (window[i] == 1) {
            num++;
        } else {
            break;
        }
    }
    return num;
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

    *source = malloc(sizeof(char) * fileSize + 1);

    // file size error
    if (fseek(fp, 0L, SEEK_SET) != 0){
        fprintf(stderr, "Error: file size error\n");
        free(*source);
        fclose(fp);
        return -1;
    }

    sourceLen = fread(*source, sizeof(char), fileSize, fp);

    // file reading error
    if(sourceLen == 0){
        fprintf(stderr, "Error: file reading error\n");
        free(*source);
        fclose(fp);
        return -1;
    }

    (*source)[sourceLen] = '\0';

    fclose(fp);
    return sourceLen;
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr,"usage: sender <portnumber> <CWnd> <Pl> <Pc>\n");
        exit(1);
    }

    char *port = argv[1];
    double pl = atof(argv[3]);
    double pc = atof(argv[4]);

    if (pl < 0 || pc < 0 || pl > 1 || pc > 1){
        fprintf(stderr,"Pl and Pc must both be between 0 and 1\n");
        exit(1);
    }

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
            fprintf(stderr, "sender: socket\n");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            fprintf(stderr, "sender: bind\n");
            continue;
        }
        break;
    }

    if (p == NULL){
        fprintf(stderr, "Error: sender failed to bind socket");
        return 2;
    }

    freeaddrinfo(servinfo);

    fprintf(stderr, "Bound to socket, now waiting for requested filename\n");

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
    fprintf(stderr, "The requested filename is: %s\n", buf);


    char *source = NULL;
    int loadFileRV = verifyAndLoadFile(buf, &source);

    if (loadFileRV == -1) {
        sendPacket(conn, createFileNotFoundPacket());
        exit(1);
    }

    size_t sourceLen = loadFileRV;

    size_t currentPosition = 0;
    int cwnd = atoi(argv[2]);

    fprintf(stderr, "beginning sending file\n");
    // we are finished if the currentPosition is past the end of the file
    while (currentPosition < sourceLen) {
        // send the window of packets
        int numberOfPacketsToSend = min(cwnd, numberOfPacketsLeftInFile(sourceLen, currentPosition));
        int pi;
        for (pi = 0; pi < numberOfPacketsToSend; pi++) {
            size_t startingPosition = currentPosition + pi*MAXDATALEN;
            protocolPacket packet = createPacket(
                    startingPosition,
                    0,
                    (startingPosition + MAXDATALEN >= sourceLen),
                    source+startingPosition,
                    min(MAXDATALEN, sourceLen - startingPosition));
            fprintf(stderr, "sending: packet seq: %d\n", packet.seq);
            sendPacket(conn, packet);
        }

        int received;

        // idk what's really going on here but apparently this is how select works...
        fd_set set;
        FD_ZERO(&set);
        FD_SET(conn.sockfd, &set);

        // setup window details
        size_t windowSize = cwnd * sizeof(int);
        int *window = (int *)malloc(windowSize);
        memset(window, 0, windowSize);

        // wait up to 3 seconds
        struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };

        // while we receive packets, process them
        while ((received = select(conn.sockfd+1, &set, NULL, NULL, &timeout)) >= 1) {
            protocolPacket packet = receivePacket(conn);
            int pi = getPacketIndex(packet, currentPosition);
            window[pi] = 1;
            fprintf(stderr, "receiving ack for packet seq: %d\n", packet.seq);
        }

        // update window details
        int windowShift = numberOfConsecutivelyReceivedPackets(window, cwnd);
        currentPosition += MAXDATALEN * windowShift;
    }
    fprintf(stderr, "done sending file, got final FINACK\n");

    free(source);
    close(conn.sockfd);

    return 0;
}


