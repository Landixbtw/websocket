#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <wait.h>
#include <signal.h>
#include <poll.h>

#include "../include/utils.h"

// https://beej.us/guide/bgnet/source/examples/server.c
// https://beej.us/guide/bgnet/html/index-wide.html#getaddrinfoprepare-to-launch

#define BACKLOG 10
#define PORT "3490"
#define MAXDATASIZE 1024

void *get_in_addr(struct sockaddr *sa);
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size);
void del_from_pfds(struct pollfd *pfds[], int i, int *fd_count);

int main(void)
{
    // We start linstening on sockfd (sockfd), and all new connection go on new_fd
    int status, sockfd, new_fd, fd_size, fd_count = 0;
    int const yes=1;

    // TODO: ???
    struct sockaddr_storage client_addr;

    // Client address socket length
    socklen_t addr_size;
    // servinfo will point to the result of getaddrinfo
    struct addrinfo hints, *servinfo, *p;

    // IPv6 Address string len
    char s[INET6_ADDRSTRLEN];

    // sigaction, "action to be taken when a signal arrives"
    struct sigaction sa;
    // This will make sure that the hints struct is empty before using it
    memset(&hints, 0, sizeof(hints));

    /*
    *       int               ai_flags      Input flags.
    *       int               ai_family     Address family of socket.
    *       int               ai_socktype   Socket type.
    *       int               ai_protocol   Protocol of socket.
    *       socklen_t         ai_addrlen    Length of socket address.
    *       struct sockaddr  *ai_addr       Socket address of socket.
    *       char             *ai_canonname  Canonical name of service location.
    *       struct addrinfo  *ai_next       Pointer to next in list.
    */

    // We dont care if its IPv4 or IPv6 | Set to AF_INET (IPv4) or IF_INET6 (IPv6)
    hints.ai_family = AF_UNSPEC;
    // https://pubs.opengroup.org/onlinepubs/9799919799/
    // SO_ACCEPTCONN socket is accepting connections

    // The TCP stream sockets
    hints.ai_socktype = SOCK_STREAM;

    /*
     * Assign address of my local host to the socket structure, we can put a specific
     * address into the first param for getaddrinfo
     */

    hints.ai_flags = AI_PASSIVE;

    //specifies the protocol for the returned socket addresses, 0 indicates that the socket addresses with any protocol can be returned
    hints.ai_protocol = 0;

    // We are just getting the local ip address of the server so that you can connect to it from the network
    char *ip_grep = "ip a | grep -v 'docker0' | grep -Eo 'inet (addr:)?([0-9]*\\.){3}[0-9]*' | grep -Eo '([0-9]*\\.){3}[0-9]*' | grep -v '127.0.0.1' | head -n 1";
    const int SYSTEM_IP = system(ip_grep);

    /*
     * for getaddrinfo() function
     * @param name: website link or IP e.g www.google.com / 8.8.8.8
     * @param service: http/https or port number
     * @param req: points to ```struct addrinfo``` which we already filled out
     * https://stackoverflow.com/questions/23401147/what-is-the-difference-between-struct-addrinfo-and-struct-sockaddr
    */

    if ((status = getaddrinfo((char *) SYSTEM_IP, PORT, &hints, &servinfo)) != 0)
    {
      fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
      exit(EXIT_FAILURE);
    }

    printf("Server started\n");

    /*
     * servinfo now points to a linked list of atleast 1 struct addrinfo
     * we can now work with it till we free it with freeaddrinfo(servinfo)
     */

    valid_ll_servinfo(servinfo);

    // we loop through all the results and bind to the first one we can 
    for (p = servinfo; p != NULL; p = p->ai_next) 
    {
        /*
         * This creates the socket.
         * @param domain: specifies the communications domain (IPv4) (IPv6)
         * @param type: specifies the socket type
         * @param protocol: specifies the socket protocol
         */
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
        {
            perror("server: socket()");
            continue;
        }


        /*
         * This will allow reusing a port, that might "Already be in use",
         * @param fd: is the s (socket) we just assigned
         * @param level: This specifies the protocol level at which the option resides (???) To retrieve the option specify as SOL_SOCKET
         * (By using SOL_SOCKET, you are telling the operating system that the option you want to set (like SO_REUSEADDR) applies to the socket itself, 
         * rather than to the TCP protocol specifically)
         * @param optname: This report wether the rules used in validating addresses in bind() should allow the reuse of local addresses, if supported by the protocol
         * @param optval: This is the option (bool) you want to set for optname
         */
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt()");
            /* 
             * We only exit if setsockopt fails because this is a cricital error
             * this means that, SO_REUSEADDR could not be set, which can 
             * lead to errors if the port was recently used and is still in a TIME_WAIT state
             * not having SO_REUSEADDR could prevent the socket from bindind successfully.
            */
            exit(EXIT_FAILURE);
        }


        /*
         * @param fd: is the socket file descriptor that is returned by socket().
         * @param *addr: is the pointer to a struct sockaddr, that contains the 
         * information about the address. (Port, IP, ...)
         * @param len: is the length of bytes of that address,
         */

        if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
        {
            close(sockfd);
            // we print server: bind() so that we know the error happened server side.
            perror("server: bind()");
            continue;
        }
        break;
    }

    // Since servinfo is a linked list we need to free it at the end.
    freeaddrinfo(servinfo);

    if (p == NULL) 
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(EXIT_FAILURE);
    }

    /*
     * This will listen to the connection (s)
     * @param fd: This is the socket we set earlier (s)
     * @param n: This is how many connection requests will be queued before further requests are refused
     */
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    /*
     * -- reaping all dead processes --
     * reaping means cleaning up terminated child processes, this is kinda like a automatic cleaning crew for dead child processes
     * sigemptyset() initializes a so called signal mask to empty, the mask determines which signal should be blocked while the handler is running.
     * SA_RESTART tells the system to auto restart system calls that were interupted by the signal handler.
     * sigaction() install the handler, 
     * @param sig: SIGCHLD is the signal that is sent when a child process is being terminated. 
     * @param act: This is the configure signal action structure
     * @param oact: NULL means that we dont care about the previous signal handler
     */

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction()");
        exit(EXIT_FAILURE);
    }

    printf("server: waiting for connections...\n");

    /*
     * this is for polling
     * we start with 5 connections we can realloc if more are necessary
     */

    fd_count = 1;
    fd_size = 5;

    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);
    
    if(pfds == NULL)
    {
        perror("server: ");
    }

    // NOTE: listener -> sockfd
    pfds[0].fd = sockfd;
    pfds[0].events = POLLIN; // Tell me when you are ready to read

    // NOTE: Right now we try get it working for one client. Expand to n later

    while(1)
    {
        int poll_event = poll(pfds, fd_count, -1);
        if (poll_event == -1)
        {
            perror("server: poll()"); 
            exit(EXIT_FAILURE);
        }

        if (poll_event == 0) perror("server: poll()"); // poll time out

        for (int i = 0; i < fd_count; i++)
        {
            if (pfds[i].revents & POLLIN)
            {
                if (pfds[i].fd == sockfd)
                {
                    addr_size = sizeof(client_addr);
                    new_fd = accept(sockfd, (struct sockaddr *) &client_addr, &addr_size);

                    if (new_fd == -1)
                    {
                        perror("server: accept()");
                    } else
                    {
                        add_to_pfds(&pfds, new_fd, &fd_count, &fd_size);

                        char *msg = "Welcome on the [Websocket]";
                        int len = strlen(msg);
                        if (send(new_fd, msg, len , 0) == -1)
                        {
                            perror("server: send");
                            break;
                        }
                    }
                } else
                {
                    char buf[MAXDATASIZE];

                    int nbytes = recv(pfds[i].fd , buf, MAXDATASIZE, 0);

                    if (nbytes <= 0)
                    {
                        if (nbytes == 0)
                        {
                            printf("Socket %d hung up.\n", pfds[i].fd);
                        } else
                        {
                            perror("server: recv");
                        }
                        close(pfds[i].fd);
                        del_from_pfds(&pfds, i, &fd_count);
                    } else
                    {
                        buf[nbytes] = '\0';
                        printf("Message from Socket: %d \n %s\n", pfds[i].fd, buf);
                    }
                }
            }
        }
    }
    return 0;
}

void add_to_pfds(struct pollfd *pfds[], int new_fd, int *fd_count, int *fd_size)
{
    // we want to dynamically add more entries if needed
    // we can do that with realloc
    //
    // first we want to check if we have enough space
    // if (not enough room in the array)
    //      make array bigger
    //      realloc the size
    //
    //  
    // we need to increase the fd_count because we added a new one
    if (*fd_count == *fd_size) {
        *fd_size *= 2;
    
        struct pollfd *temp_pfds = realloc(*pfds, sizeof(struct pollfd) * (*fd_size));
    
        if (temp_pfds == NULL) {
            perror("realloc failed");
            *fd_size /= 2; // Revert size change
            return; 
        }
        *pfds = temp_pfds;
    }

    /*
     *  because of pfds declaration we need to dereference the pointer.
     *  (*pfds) gets the pointer, and *fd_count is I 
     */

    (*pfds)[*fd_count].fd = new_fd;
    (*pfds)[*fd_count].events = POLLIN | POLLOUT;

    (*fd_count)++;
}


void del_from_pfds(struct pollfd *pfds[], int i, int *fd_count)
{
    // we want to copy the last element over the one we want to remove and shrink the list by one at the end
    /*
     * Before (fd_count = 4):
     * pfds[0]: fd1 
     * pfds[1]: fd2  <- want to delete this (i = 1) 
     * pfds[2]: fd3 
     * pfds[3]: fd4 
     *
     * Step 1: pfds[1] = pfds[3] 
     * pfds[0]: fd1 
     * pfds[1]: fd4  <- copied from end 
     * pfds[2]: fd3 
     * pfds[3]: fd4  <- not needed anymore 
     * Step 2: fd_count-- 
     * now fd_count = 3, effectively making the last element inaccessible
     *
     * we can use I like this because this function will later be used in a for loop
    */

    pfds[i] = pfds[*fd_count - 1];

    (*fd_count)--;
}
