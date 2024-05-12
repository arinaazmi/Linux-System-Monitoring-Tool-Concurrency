#ifndef STATS_FUNCTIONS_H
#define STATS_FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <ctype.h>
#include <getopt.h>
#include <math.h> 

#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <utmp.h>

#define BUF_SIZE 1024

#include <errno.h>
#include <limits.h>

struct MemoryInfo {
    double total_physical;
    double physical_used;
    double total_virtual;
    double virtual_used;
    char final_string[BUF_SIZE];
};

struct user {
    char username[BUF_SIZE];
    char terminal[BUF_SIZE];
    char host[BUF_SIZE];
};



#endif
