//
// Created by ole on 25.10.24.
//
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>
#include <poll.h>

#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include "../include/utils.h"

// https://beej.us/guide/bgnet/html/index-wide.html#getaddrinfoprepare-to-launch
// https://beej.us/guide/bgnet/source/examples/client.c

#define PORT "3490"
// max number of bytes we can get at once
#define MAXDATASIZE 1024


void *get_in_addr(struct sockaddr *sa);

int main(int argc, char *argv[])
{

    int status, sockfd, numbytes;
    struct addrinfo hints, *servinfo, *p;
    // buffer for recv()
    char buf[MAXDATASIZE];
    // IPv6 Address string length
    char s[INET6_ADDRSTRLEN];


    if (argc != 2)
    {
        printf("Usage: \e[1m%s [server host address]\e[0m\n", argv[0]);
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    printf("trying to connect to: %s\n", argv[1]);
    if ((status = getaddrinfo(argv[1], PORT , &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1; 
    }

    // we loop through all the results and bind to the first one we can 
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        /* this creates the client socket */
        sockfd = 0;
        if ((sockfd= socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("client: socket()");
            continue;
        }
        // then we connect to the socket, we just created 

        // TODO: ip addr might not be init correctly
        printf("hostname: %s\n", p->ai_canonname);
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect()");
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
    printf("client: connecting to %s on port %s\n", s, PORT);

    // we dont need the linked list anymore so we free it
    freeaddrinfo(servinfo);

    /*
     * -- What do we want --
     *  - Maintain persistent connection with the server
     *  - Keep receiving messages
     *
     * -- What do we need --
     *  - How can we keep the client "alive" -> Loop
     *      - Where do we put it
     *  - When the print statement is in the loop it prints '' all the time.
     *      - Print statement should only be triggered if there is a message
     *      - if (message arrived) printf(message)
     *
     * -- recv() blocking --
     *  when recv "blocks" it waits for data to arrive, 
     *  --> Use non blocking?
     *  What happens while its waiting ? What can we do while it waits ?
     *
     *  We cant use scanf() because the program is paused while scanf is waiting for input
     *
     * We want to use poll(), not select() this is apparently the old way
     *
     */


    /*
     * -- this allows for synchronous I/O Multiplexing --
     * @param fds[]: array of information (sockets to monitor)
     * @param nfds: count of elements in the array
     * @param timeout: timeout in milliseconds
     * poll returns number of elements which have had an event occur
     */

    /*
     * this is for polling
     * we start with 5 connections we can realloc if more are necessary
     */
    int fd_count = 2;
    int fd_size = 5;

    /*
     * struct pollfd { 
     * int fd;         // the socket descriptor 
     * short events;   // bitmap of events we're interested in 
     * short revents;  // when poll() returns, bitmap of events that occurred 
     * };
     */
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN; // ready to read something, either user input or server output


    pfds[1].fd = sockfd;
    pfds[1].events = POLLIN;


    while(1)
    {
        // https://sysadminsage.com/recv-failure-connection-reset-by-peer/
        // https://stackoverflow.com/questions/40621899/c-socket-recv-connection-reset-by-peer

        // timeout -1 means wait forever
        int poll_event = poll(pfds, fd_count, -1);

        // right now it should never time out because timeout is -1
        if (poll_event == 0) printf("Poll timed out\n");
        if (poll_event == -1) perror("client: poll()");
        if (poll_event > 0)
        {
            for(int i = 0; i < fd_count; i++)
            {
                if (pfds[i].revents & POLLIN && pfds[i].fd == sockfd)
                {
                    // we receive the message the server sent
                    if ((numbytes = recv(pfds[1].fd, buf, MAXDATASIZE, 0)) == -1)
                    {
                        switch(numbytes)
                        {
                            case(EAGAIN | EWOULDBLOCK): continue;
                            case(ENOTSOCK): fprintf(stderr, "error: this is not a socket: %s", strerror(errno)); break;
                            default: perror("client: recv"); break;
                        }
                    }

                    // Null terminate the string
                    buf[numbytes] = '\0';

                    if (numbytes > 0)
                    {
                        fprintf(stderr, ">> ");
                        printf("client: received '%s' \n", buf);
                    }
                    else if(numbytes == 0)
                    {
                        printf("client: server has disconnected\n");
                        free(pfds);
                        close(sockfd);
                        return 0;
                    }
                    fprintf(stderr, "<< ");
                }
                else if(pfds[i].revents & POLLIN && pfds[i].fd == STDIN_FILENO)
                {
                    fprintf(stderr, "<< ");
                    char *msg = custom_getline();
                    if(msg == NULL)
                    {
                        fprintf(stderr, "msg is NULL\n");
                        exit(1);
                    }
                    int len = strlen(msg+1);
                    // check for EWOULDBLOCK || EAGAIN
                    // save unsent data and set POLLOUT for next poll() call
                    if (len > 0)
                    {
                        if (send(pfds[1].fd, msg, len , 0) == -1)
                        {
                            perror("client: send");
                            break;
                        }
                    }
                }
                else if(pfds[i].revents & POLLHUP)
                {
                    fprintf(stderr, "Hangup occured on device %i", i);
                } else if (pfds[i].revents & POLLERR)
                {
                    // how can we check the error value? Errno is not set
                    fprintf(stderr, "Error occured on device %i", i);
                }
            }
        } 
    }

    free(pfds);
    close(sockfd);

    return 0;
}

