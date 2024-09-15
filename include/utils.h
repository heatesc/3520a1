#ifndef UTILS_H
#define UTILS_H
#include <pthread.h>

// DEBUG being set to 1 activates debug functionality, e.g. DEBUG_PRINT.
#define DEBUG (1)
// sources:
// - https://stackoverflow.com/questions/1644868/define-macro-for-debug-printing-in-c
// - https://www.theurbanpenguin.com/4184-2/
// note: debug ou
#define DEBUG_PRINT(fmt, ...) \
            do { if (DEBUG) fprintf(stdout, "\033[0;31m"fmt"\033[0m", ##__VA_ARGS__); } while (0)
            
/*
 * When the program is in test mode, threads will log numbers to a file.
 * These numbers will be used by "validator.py" to determine whether
 * the program has executed correctly. For more information, see the
 * testing-related portion of part 2 of this assignment.
 */
#define TEST_MODE (1)
#define TEST_MODE_LOG_FILE "test_log.txt"
// #define TEST_LOG(x) if (TEST_MODE) test_log(x);

#define MAX_INT_DIGITS (10)

int parse_int(int* dest, char* src);

void strip_newline(char* s);

int get_rand_num(int min, int max);

// returns 0 only if success
int get_int_from_stdin(int* dest);

/**
 * Note: mtsafe stands for multi threading safe.
 *
 * Note: when writing this function, I needed to learn how to
 * pass a format string as an argument. I learnt this from
 * stackoverflow:
 * https://stackoverflow.com/questions/68154231/how-do-i-define-a-function-that-accepts-a-formatted-input-string-in-c
 * 
 * @param can_print_mut 
 * @param str 
 * @param ... 
 */
void mtsafe_printf(pthread_mutex_t* can_print_mut, char* str, ...);

#endif // UTILS_H