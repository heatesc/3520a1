#ifndef UTILS_H
#define UTILS_H
#include <pthread.h>
#include <stdio.h>

// DEBUG being set to 1 activates debug functionality, e.g. DEBUG_PRINT.
#define DEBUG (1)
// sources:
// - https://stackoverflow.com/questions/1644868/define-macro-for-debug-printing-in-c
// - https://www.theurbanpenguin.com/4184-2/
// note: debug ou
#define DEBUG_PRINT(fmt, ...) \
            do { if (DEBUG) fprintf(stdout, "\033[0;31m"fmt"\033[0m", ##__VA_ARGS__); } while (0)
            
#define MAX_INT_DIGITS (10)
#define GROUP_UNASSIGNED (-1)
#define LAB_UNASSIGNED (-1)
#define GROUP_UNAVAILABLE (-2) // this represents no groups left

typedef struct
{
    int N;
    int M;
    int K;
    int T;
} config;

// returns 0 if successful
config* config_get();

int parse_int(int* dest, char* src);

void strip_newline(char* s);

int get_rand_num(int min, int max);

// returns 0 only if success
int get_int_from_stdin(int* dest);

/**
 * Note: mtsafe stands for multi threading safe.
 *
 * Note: when writing this function, I needed to learn how to
 * pass a format string as an argument. I learned this from
 * stackoverflow:
 * https://stackoverflow.com/questions/68154231/how-do-i-define-a-function-that-accepts-a-formatted-input-string-in-c
 * 
 * @param can_print_mut 
 * @param str 
 * @param ... 
 */
void mtsafe_printf(pthread_mutex_t* can_print_mut, char* str, ...);

// return 0 only if input config is valid
int conf_check(config* conf);

int num_students_left(pthread_mutex_t* mut, int* source);

#endif // UTILS_H