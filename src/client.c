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

// Setup client address information
// getaddrinfo

// create a new socket for the client

// connect to the server via connect()
// send() or receive (recv())
// close the socket


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

    int se = send(s, "Hello World!\n", 12, 0);
    if (se == -1) {
        perror("send()");
        close(s);
        freeaddrinfo(servinfo);
        exit(EXIT_FAILURE);
    }
    return 0;
}

