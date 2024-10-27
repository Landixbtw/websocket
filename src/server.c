#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <wait.h>
#include <signal.h>
#include <poll.h>


// https://beej.us/guide/bgnet/source/examples/server.c
// https://beej.us/guide/bgnet/html/index-wide.html#getaddrinfoprepare-to-launch

#define BACKLOG 10
#define PORT "3490"

void *valid_ll_servinfo(struct addrinfo *linked_list);
void *get_in_addr(struct sockaddr *sa);
void sigchld_handler(int s);
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size);
void del_from_pfds(struct pollfd *pfds[], int i, int *fd_count);

int main(void)
{
    // We start linstening on s_fd (sockfd), and all new connection go on new_fd
    int status, s_fd, new_fd;
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
    char *ip_grep= "ip a| grep -Eo 'inet (addr:)?([0-9]*\\.){3}[0-9]*' | grep -Eo '([0-9]*\\.){3}[0-9]*' | grep -v '127.0.0.1'";

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
    printf("Websocket Server started\n");

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
        if ((s_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
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
        if (setsockopt(s_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
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
         * @param *addr: is the pointer to a struct sockaddr, that contains the information about the address. (Port, IP, ...)
         * @param len: is the length of bytes of that address,
         */

        if (bind(s_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
        {
            close(s_fd);
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
    if (listen(s_fd, BACKLOG) == -1)
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

    // NOTE: listener -> s_fd
    for(;;)
    {
        addr_size = sizeof client_addr;
        /*
         * When a new client connects accept() return a new file descriptor (new_fd) and fills it with the client address
         * for communicating with the new client
         */
        new_fd = accept(s_fd, (struct sockaddr *)&client_addr, &addr_size);
        if (new_fd == -1)
        {
            perror("accept()");
            continue;
        }

        /*
         * inet_ntop converts the client IP address from binary to a readable string
         * get_in_addr is explained in the implemenation
         * The resulting address is stored in s
         */
        inet_ntop(client_addr.ss_family, 
                  get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);
        printf("server got connection from %s\n", s);

        /*
         * fork() creates a child process to handle the new client connection
         * if fork returns 0 a new child process has been created
         * the child process will now independently handle the connection
         */ 
        // Why was it !fork()?
        if (!fork()) 
        {
            char *msg = "Welcome on the [Websocket]";
            int len = strlen(msg);
            // the child process doesnt need the listener, we have new_fd
            close(s_fd);
            if (send(new_fd, msg, len , 0) == -1)
            {
                perror("server: send");
            }
            close(new_fd);
            exit(0);
        }
        // the parent doesnt need this it has s_fd
        close(new_fd);
    }


    /*
     * the server obv also need to be able to receive something. So after 
     * accepting the request from the client we can receive messages
     * @param socket: The socket 
     * @param buffer: Points to a buffer where the message should be stored
     * @param length: length of the buffer in bytes.
     * @param flags: type of message reception. (MSG_PEEK. Data is treated as 
     * unread, next recv or similar function can still return the data)
     */

    char *buf = malloc(sizeof(char) * 50);
    int bytes_received, len;
    len = strlen(buf);

    bytes_received = recv(s_fd, &buf, len, 0);
    if (bytes_received == -1)
    {
        perror("recv()");
    } else if (bytes_received == 0){
        printf("The client has closed the connection\n");
    }

    close(s_fd);
    return 0;
}


// This function returns the last valid ptr address that matches all 3 checks

void *valid_ll_servinfo(struct addrinfo *linked_list)
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

        printf("Valid entry found: family=%d, socktype:%d, protocol:%d\n",
                ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    }
    /* 
    * This casts ptr to the struct sockaddr_in *ptr and then access the address,
    * where the struct sockaddr_in *ptr->sin_addr is pointing at
    */    return &(((struct sockaddr_in*)ptr)->sin_addr);
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void sigchld_handler(int s)
{
    (void)s;
    // waitpid() might overwrite errno so we save errno
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
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
