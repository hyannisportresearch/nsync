/**
 * @file nsync_utils.c
 * File to store helper functions for the nsync utility
 * 
 * @author Ben London
 * @date 7/27/2020
 * @copyright Copyright 2020, Hyannis Port Research, Inc. All rights reserved.
 */

#include "nsync_utils.h"


/** Global Error Message String */
char err_msg[ERR_LEN];

/** Global Error Pointer (Alternative to NULL) */
int fatal_err_to_p;
void *fatal_err_ptr = &fatal_err_to_p;


/**
 * @brief Checks if a file exists
 * 
 * @param path the location of the file that is being looked up
 * @returns a boolean value for whether or not a file exists
 */
bool file_exists(const char *path)
{
    /** Check that path exists */
    struct stat filestat;
    if (stat(path, &filestat))
        return false;

    /** Check that path is a normal file */
    if (S_ISREG(filestat.st_mode))
        return true;

    /** Not a file */
    return false;
}

/**
 * @brief Perform strncpy, but ensure null-termination
 * Intended to avoid buffer overruns.
 * Can only be used safely if you know the size of dest.
 * Will silently truncate if src string length >= n
 *
 * @param dest - null-terminated output string
 * @param src - input string (need not be null-terminated)
 * @param n - maximum possible bytes to be written to dest (inlcuding null-terminator)
 *
 * @return a pointer to dest
 */
char *safe_strncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    if (!n)
        return dest;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    dest[n-1] = '\0';
    return dest;
}

/**
 * @brief Get field(s) from a delimted line with filtered leading and trailing whitespace.
 *
 * @param dest - Destination buffer.
 * @param src - Source buffer.
 * @param col - 1-indexed CSV column with index 0 indicating the entire line (awk-style).
 * @param destsz - Size of the destination buffer in bytes. If nonzero, a maximum of destsz
 * minus one bytes will be written (not including the null terminator).
 * @param delim - delimeter to split line over, must be a null terminated string
 *
 * @return - The dest argument containing the requested string. On any errors,
 * the empty string is written.
 */
char *get_field_delim(char *dest, char const *src, size_t col, size_t destsz, const char * delim)
{
    /** Make a mutable copy of the input string for tokenizing.*/
    char srccpy[strlen(src) + 1];
    strcpy(srccpy, src);

    /** Tokenize until the requested field is found. */
    size_t i = 0;
    char *srccpyptr = srccpy;
    char const *field = srccpyptr;
    while ((i != col) && ((field = strsep(&srccpyptr, delim)) != NULL)) { ++i; }
    if ((i != col) || (field == NULL)) { return safe_strncpy(dest, "", destsz); }

    /** Trim leading and trailing whitespace. */
    char const *start = field;
    char const *end = field + strlen(field);

    /** Copy the trimmed field to the user, truncating if necessary. */
    return safe_strncpy(dest, start, min((size_t)(end - start) + 1, destsz));
}

/**
 * @brief Trims the given chars off the left side of a string
 * 
 * @param str the string to be trimmed
 * @param seps the values to be trimmed. If NULL, 
 * then "\t\n\v\f\r" will be used
 * 
 * @returns the string that has been trimmed
 */
char *ltrim(char *str, const char *seps)
{
    size_t totrim;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    totrim = strspn(str, seps);
    if (totrim > 0) {
        size_t len = strlen(str);
        if (totrim == len) {
            str[0] = '\0';
        }
        else {
            memmove(str, str + totrim, len + 1 - totrim);
        }
    }
    return str;
}


/**
 * @brief Trims the given chars off the right side of a string
 * 
 * @param str the string to be trimmed
 * @param seps the values to be trimmed. If NULL, 
 * then "\t\n\v\f\r" will be used
 * 
 * @returns the string that has been trimmed
 */
char *rtrim(char *str, const char *seps)
{
    int i;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    i = strlen(str) - 1;
    while (i >= 0 && strchr(seps, str[i]) != NULL) {
        str[i] = '\0';
        i--;
    }
    return str;
}


/**
 * @brief Trims the given chars off of both sides of a string
 * 
 * @param str the string to be trimmed
 * @param seps the values to be trimmed. If NULL, 
 * then "\t\n\v\f\r" will be used
 * 
 * @returns the string that has been trimmed
 */
char *trim(char *str, const char *seps)
{
    return ltrim(rtrim(str, seps), seps);
}


/**
 * @brief Determines if a directory exists
 * @param path the absolute path to the directory
 * @returns boolean value. If the directory exists then
 * returns true. Otherwise returns false.
 */
bool dir_check(const char *path)
{
    struct stat filestat;

    if (stat(path, &filestat)) {
        return false;
    }

    return S_ISDIR(filestat.st_mode);
}

/**
 * @brief convert number of netmask bits into a netmask string 
 * @param bits the number of bits on in the netmask
 * @returns netmask string
 */
char *bitmask_to_netmask_ipv4(int bits){
    
    unsigned bitmask = (~0U) << (32-bits);
    /** Special Case */
    if (!bits) bitmask = 0;
    /** ipv4: a.b.c.d */
    u_int8_t d = ((u_int8_t *) &bitmask)[0];
    u_int8_t c = ((u_int8_t *) &bitmask)[1];
    u_int8_t b = ((u_int8_t *) &bitmask)[2];
    u_int8_t a = ((u_int8_t *) &bitmask)[3];

    char *netmask = malloc(20);
    sprintf(netmask, "%d.%d.%d.%d", a, b, c, d);

    return netmask;
}
