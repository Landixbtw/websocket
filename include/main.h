//
// Created by ole on 25.10.24.
//

#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>

void show_usage(char *PROGRAM_NAME)
{
    fprintf(stdout, 
            "Usage: \e[1m./%s [server hostname / server host address]\e[0m\n", PROGRAM_NAME);
}

#endif //MAIN_H
