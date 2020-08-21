/**
 * @file nsync_utils.h
 * 
 * @author Ben London
 * @date 7/27/2020
 * @copyright Copyright 2020, Hyannis Port Research, Inc. All rights reserved.
 */

#ifndef NSYNC_UTILS_H
#define NSYNC_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define ERR_LEN 10000
#define MAX_OUTPUT_LEN 1000


#define MEM_CHECK(ptr, ret_fail)                        \
    if (ptr == NULL){                                   \
        sprintf(err_msg, "could not allocate memory");  \
        return ret_fail;                                \
    }

/*Macros that can be used to create color in text*/
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define KBLD  "\x1B[1m"
#define KDIM  "\x1B[2m"
#define KUNL  "\x1B[4m"
#define KINV  "\x1B[7m"


#define min(a,b)                \
    ({ typeof (a) _a = (a);     \
        typeof (b) _b = (b);    \
        _a < _b ? _a : _b; })

extern void *fatal_err_ptr;

bool file_exists(const char *);

char *safe_strncpy(char *, const char *, size_t);

char *get_field_delim(char *, char const *, size_t, size_t, const char *);

char *ltrim(char *str, const char *seps);

char *rtrim(char *str, const char *seps);

char *trim(char *str, const char *seps);

bool dir_check(const char *path);

char *bitmask_to_netmask_ipv4(int bits);

#endif