#ifndef UTILS_H
#define UTILS_H

#define DEBUG (1)
// source: https://stackoverflow.com/questions/1644868/define-macro-for-debug-printing-in-c
#define DEBUG_PRINT(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

#define MAX_INT_DIGITS (10)

int parse_int(int* dest, char* src);

void strip_newline(char* s);

int get_rand_num(int min, int max);

// returns 0 only if success
int get_int_from_stdin(int* dest);

#endif // UTILS_H