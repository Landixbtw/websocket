#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

void valid_ll_servinfo(struct addrinfo *linked_list);


int server(void)
{
    int status = 0;
    // servinfo will point to the result of getaddrinfo
    struct addrinfo hints, *servinfo;
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

    /*
     * for getaddrinfo() function
     * @param name: website link or IP e.g www.google.com / 8.8.8.8
     * @param service: http/https or port number
     * @param req: points to ```struct addrinfo``` which we already filled out
     * https://stackoverflow.com/questions/23401147/what-is-the-difference-between-struct-addrinfo-and-struct-sockaddr
    */

    if ((status = getaddrinfo("localhost", "8080", &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Websocket Server started\n");

    /*
     * servinfo now points to a linked list of atleast 1 struct addrinfo
     * we can now work with it till we free it with freeaddrinfo(servinfo)
     */

    valid_ll_servinfo(servinfo);


    /*
     * This creates the socket.
     * @param domain: specifies the communications domain (IPv4) (IPv6)
     * @param type: specifies the socket type
     * @param protocol: specifies the socket protocol
     */
    int s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (s == -1) {
        perror("socket()");
        close(s);
        freeaddrinfo(servinfo);
        exit(EXIT_FAILURE);
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
    int const yes=1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
    {
        perror("setsockopt()");
        close(s);
        freeaddrinfo(servinfo);
        exit(EXIT_FAILURE);
    }


    /*
     * @param fd: is the socket file descriptor that is returned by socket().
     * @param *addr: is the pointer to a struct sockaddr, that contains the information about the address. (Port, IP, ...)
     * @param len: is the length of bytes of that address,
     */

    int b = bind(s, servinfo->ai_addr, servinfo->ai_addrlen);
    if (b == -1) {
        perror("bind()");
        close(s);
        freeaddrinfo(servinfo);
        exit(EXIT_FAILURE);
    }


        /*
     * This will listen to the connection (s)
     * @param fd: This is the socket we set earlier (s)
     * @param n: This is how many connection requests will be queued before further requests are refused
     */
    int l = listen(s, 5);
    if (l == -1) {
        perror("listen()");
        close(s);
        freeaddrinfo(servinfo);
        exit(EXIT_FAILURE);
    }


    int a = accept(s, servinfo->ai_addr, &servinfo->ai_addrlen);
    if (a == -1) {
        perror("accept()");
        close(s);
        freeaddrinfo(servinfo);
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(servinfo); // Since servinfo is a linked list we need to free it at the end.
    close(s); // close the listening socket
    return 0;
}


void valid_ll_servinfo(struct addrinfo *linked_list)
{
    struct addrinfo *ptr;

    // This steps through the linked list one at a time
    for (ptr = linked_list; ptr != NULL; ptr = ptr->ai_next) {

        // Check the address family
        if (ptr->ai_family != AF_INET && ptr->ai_family != AF_INET6) {
            fprintf(stderr, "Invalid address family: %d\n", ptr->ai_family);
            continue;
        }

        // Check the socket type
        if (ptr->ai_socktype != SOCK_STREAM) {
            fprintf(stderr, "Invalid socket type: %d\n", ptr->ai_socktype);
            continue;
        }

        // Check the protocol
        if (ptr->ai_protocol != 0 && ptr->ai_protocol != IPPROTO_TCP) {
            fprintf(stderr, "Invalid protocol: %d\n", ptr->ai_protocol);
            continue;
        }

        fprintf(stdout, "Valid entry found: family=%d, socktype:%d, protocol:%d\n",  
                ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    }
}
