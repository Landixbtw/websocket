//
// Created by ole on 25.10.24.
//

#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <netdb.h>

#include <arpa/inet.h>

#include <errno.h>

void *valid_ll_servinfo(struct addrinfo *linked_list);
void show_usage(char *program_name);
void sigchld_handler(int s);
void *get_in_addr(struct sockaddr *sa);

#endif 
