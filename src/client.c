//
// Created by ole on 25.10.24.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

// https://beej.us/guide/bgnet/html/index-wide.html#getaddrinfoprepare-to-launch
// https://beej.us/guide/bgnet/source/examples/client.c


int client(void) {
    int status = 0;

    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    if ((status = getaddrinfo("localhost", "8080", &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Connecting to server ...\n");

    int s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (s == -1) {
        perror("socket()");
        close(s);
        freeaddrinfo(servinfo);
        exit(EXIT_FAILURE);
    }

    int c = connect(s, servinfo->ai_addr, servinfo->ai_addrlen);
    if (c == -1) {
        perror("connect()");
        close(s);
        freeaddrinfo(servinfo);
        exit(EXIT_FAILURE);
    }

    char *msg = "Hello World, Beej was here\n";
    int bytes_send, len;
    len = strlen(msg);

    bytes_send = send(s, msg, len, 0);
    if (bytes_send == -1) {
        perror("send()");
        close(s);
        freeaddrinfo(servinfo);
        exit(EXIT_FAILURE);
    }
    return 0;
}

