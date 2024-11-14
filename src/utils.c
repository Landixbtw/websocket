#include "../include/utils.h"

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

void show_usage(char *program_name)
{
    printf("Usage: \e[1m%s [server hostname / server host address]\e[0m\n", program_name);
}


void sigchld_handler(int s)
{
    (void)s;
    // waitpid() might overwrite errno so we save errno
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
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



