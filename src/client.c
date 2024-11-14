//
// Created by ole on 25.10.24.
//
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
#define MAXDATASIZE 100
#define PROGRAM_NAME "./websock"

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

    // TODO: Put the client in a loop so that it does not exit when the message is sent.

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
    int fd_count = 0;
    int fd_size = 5;

    /*
     * struct pollfd { 
     * int fd;         // the socket descriptor 
     * short events;   // bitmap of events we're interested in 
     * short revents;  // when poll() returns, bitmap of events that occurred 
     * };
     */
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    pfds[0].fd = 0; // stdin
    pfds[0].events = POLLIN; // tell me when ready to read


    fd_count = 1;

    for (;;)
    {

        // https://sysadminsage.com/recv-failure-connection-reset-by-peer/
        // https://stackoverflow.com/questions/40621899/c-socket-recv-connection-reset-by-peer

        // we receive the message the server sent
        if ((numbytes = recv(s_fd, buf, MAXDATASIZE, 0)) == -1)
        {
            perror("client: recv");
            break;
        }

        if (numbytes == 0) printf("client: server has disconnected\n");

        // Null terminate the string
        buf[numbytes] = '\0';

        printf("client: received '%s' \n", buf);

        // timeout -1 means wait forever
        int poll_event = poll(pfds, fd_count, -1);

        // right now it should never time out because timeout is -1
        if (poll_event == 0) printf("Poll timed out\n");
        if (poll_event == -1) perror("client: poll()");
        else {
            /*
             *  .revents contains the event that occurred in this case POLLIN
             *  with the bitwise and (&) we are setting it to the correct int
             *  if the same bit is in both
             *
             *  (These bits are not representative only an example)
             *  revents:  00001001    (POLLIN bit is 1)
             *  POLLIN:   00000001
             *  result:   00000001    (non-zero, so polling_happened is true)
             */
            int polling_happened = pfds[0].revents & POLLIN;

            /*
             * on success a positive number is returned, 0 indicates timeout, -1 error
             * apparently non-zero numbers mean true, zero is false
             */
            if (polling_happened)
            {
                printf("File descriptor %d is ready to read\n", pfds[0].fd);
            } else {
                printf("Unexpected event occured: %d \n", pfds[0].revents);
                break;
            }
        }
    }

    close(s_fd);

    return 0;
}

