#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "../include/utils.h"


#include <pthread.h>
#include <bits/pthreadtypes.h>

int parse_int(int* dest, char* src)
{
    if (NULL == src || '\0' == *src) return 1;
    if (NULL == dest) return 2;
    char* first_invalid = NULL;
    int x = (int)strtol(src, &first_invalid, 10);
    if ('\0' == *first_invalid)
    {
        *dest = x;
        return 0;
    }
    return 3;
}

void strip_newline(char* s)
{
    if ('\n' == s[strlen(s) - 1])
        s[strlen(s) - 1] = '\0';
}

int get_rand_num(int min, int max)
{
    return min + rand() % (max - min + 1);
}

// returns 0 only if success
int get_int_from_stdin(int* dest)
{
    char input_buf[MAX_INT_DIGITS + 1];
    fgets(input_buf, sizeof(input_buf), stdin);
    strip_newline(input_buf);
    int parse_status = parse_int(dest, input_buf);
    return parse_status > 0 ? 1 : 0;
}

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
void mtsafe_printf(pthread_mutex_t* can_print_mut, char* str, ...)
{
    va_list argp;
    va_start(argp, str);
    pthread_mutex_lock(can_print_mut);
    vprintf(str, argp);
    fflush(stdout);
    pthread_mutex_unlock(can_print_mut);
    va_end(argp);    
}