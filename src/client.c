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
#include <netinet/in.h>

#include <arpa/inet.h>

#include "../include/main.h"

// https://beej.us/guide/bgnet/html/index-wide.html#getaddrinfoprepare-to-launch
// https://beej.us/guide/bgnet/source/examples/client.c

#define PORT "3490"
// max number of bytes we can get at once
#define MAXDATASIZE 100
#define PROGRAM_NAME "websock"

void *get_in_addr(struct sockaddr *sa);

int main(int argc, char *argv[]) {

    int status, s_fd, numbytes;
    struct addrinfo hints, *servinfo, *p;
    // buffer for recv()
    char buf[MAXDATASIZE];
    // IPv6 Address string length
    char s[INET6_ADDRSTRLEN];


    if (argc != 2)
    {
        show_usage(PROGRAM_NAME);
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    printf("trying to connect to: %s\n", argv[1]);
    if ((status = getaddrinfo(argv[1], PORT , &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1; 
    }

    // we loop through all the results and bind to the first one we can 
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        /* this creates the client socket */
        if ((s_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("client: socket()");
            continue;
        }
        // then we connect to the socket, we just created 
        if (connect(s_fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(s_fd);
            perror("client: connect()");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    // inet_ntop converts the IP address from binary to a readable string
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);

    // we dont need the linked list anymore so we free it
    freeaddrinfo(servinfo);

    // we receive the message the server sent
    if ((numbytes = recv(s_fd, buf, MAXDATASIZE, 0)) == -1)
    {
        perror("client: recv");
        exit(EXIT_FAILURE);
    }

    // Null terminate the string
    buf[numbytes] = '\0';


    printf("client: received '%s' \n", buf);
    close(s_fd);

    return 0;
}


    
/*
 * If the the sockaddr data in the sockaddr points to AF_INET we return the IPv4 address
 * If not we return the IPv6 address
 */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_data == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
