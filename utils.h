#ifndef utils_h
#define utils_h

#include <stdio.h>

#define BUF_SIZE 512
#define BLOCK_SIZE 4096

/* Open file and return its object */
FILE *process_file(char *filename);

/* Returns the result of stripping white spaces from the end of s (if any)
 * By Dr. William McCarthy
 * string_appendix_b3.c
 */
char *rstrip(char *s);

/* Shows a separator */
void printline(void);

/* Case insensitive string comparison */
int stricmp(const char *s1, const char *s2);

void printleft(const char *left);
void printright(const char *right);
void printboth(const char *left_right);

#endif
