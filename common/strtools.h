#ifndef STRTOOLS_H
#define STRTOOLS_H

#include <stddef.h>

/**
 * Finds the end of the line for the current string. Must be given a valid null-
 * terminated string. Recognizes \0 or \n as end-of-line.
*/
const char* eol(const char* str);

/**
 * Finds the first space or terminator in the given null-terminated string.
 */
const char* eow(const char* str);

/**
 * If str starts with exactly pattern, return a pointer to the first character
 * after pattern in str, otherwise returns null. Must be given valid null-
 * terminated strings.
 */
const char* startswith(const char* pattern, const char* str);

/**
 * Platform-independent implementation of Linux's strnchr. Finds a character
 * in a null-terminated string, looking at at most n characters.
 */
const char* strnchr(const char* str, char c, size_t n);

/**
 * Returns a pointer to the first non-whitespace character in a null-terminated
 * string.
 */
const char* ftrim(const char* str);

char *strcpy_dup(const char *str);

/**
 * (Named slightly weirdly to avoid conflicts with newer STL)
 * Copies at most the first n characters (without null) of str into a new heap
 * variable; ensures null termination
 */
char* strncpy_dup(const char* str, size_t n);

#endif