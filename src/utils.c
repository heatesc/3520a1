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

// reference: https://stackoverflow.com/questions/29381843/generate-random-number-in-range-min-max
int get_rand_num(int min, int max)
{
    return min + rand() % (max + 1 - min);
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

config* config_get()
{
    config* ret = malloc(sizeof(config));
    
    printf("Enter number of students, N: ");
    if (get_int_from_stdin(&ret->N)) goto cleanup;
    printf("Enter number of groups, M: ");
    if (get_int_from_stdin(&ret->M)) goto cleanup;
    printf("Enter number of tutors/labs, K: ");
    if (get_int_from_stdin(&ret->K)) goto cleanup;
    printf("Enter lab exercise time limit, T: ");
    if (get_int_from_stdin(&ret->T)) goto cleanup;
    
    DEBUG_PRINT("Config retrieved. N=%d, M=%d, K=%d, T=%d\n",
        ret->N, ret->M, ret->K, ret->T);

    printf("Config retrieved. N=%d, M=%d, K=%d, T=%d\n",
        ret->N, ret->M, ret->K, ret->T);

    return ret;

    cleanup:
        free(ret);
        return NULL;
}

// return 0 only if input config is valid
int conf_check(config* conf)
{
    // each variable must be positive
    if (conf->N <= 0 || conf->M <= 0 || conf->K <= 0 || conf->T <= 0)
        return 1; 
    
    // there cannot be more groups than students
    if (conf->M > conf->N) return 2;
    
    return 0;
}

int num_students_left(pthread_mutex_t* mut, int* source)
{
    pthread_mutex_lock(mut);
    const int ret = *source;
    pthread_mutex_unlock(mut);
    return ret;
}