//
// Created by ole on 25.10.24.
//

#ifndef MAIN_H
#define MAIN_H

#include <netdb.h>

void valid_ll_servinfo(struct addrinfo *linked_list);
int server(void);
int client(void);


#endif //MAIN_H
