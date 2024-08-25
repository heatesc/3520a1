#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include <unistd.h>
#include <stdbool.h>

#include "../include/utils.h"
#include "../include/ring_buf.h"

#define GROUP_UNASSIGNED (-1)
#define LAB_UNASSIGNED (-1)

typedef struct
{
    // things a student thread needs to know:
    int student_id;
    int assigned_lab;
    int assigned_group;
    pthread_mutex_t* student_counter_mut;
    int* student_counter;
    pthread_mutex_t assigned_group_mut;
    pthread_cond_t assigned_group_cond;
    pthread_mutex_t assigned_lab_mut;
    pthread_cond_t assigned_lab_cond;
    pthread_cond_t* all_students_arrived;
    int N;
} student_args;

typedef struct
{
    pthread_cond_t* all_students_arrived;
    pthread_mutex_t* student_counter_mut;
    student_args* student_args_arr;
    tutor_args* tutor_args_arr;
    int* student_counter;
    int N;
    int M;
} teacher_args;

typedef struct
{
    int id;
    pthread_mutex_t labq_mut;
    ring_buf* labq;
    int entering_group_id;
    pthread_mutex_t entering_group_mut;
    pthread_cond_t entering_group_assigned;
} tutor_args;

void* tutor(void* args)
{
    /* wait till lab rooms are vacated
     * */
    
}

void* teacher(void* args)
{
    teacher_args* targs = (teacher_args*)args;
    printf("Teacher: I'm waiting for all the students to arrive.\n");
    pthread_mutex_lock(targs->student_counter_mut);
    while (*targs->student_counter < targs->N)
    {
        printf("i ran\n");
        pthread_cond_wait(targs->all_students_arrived, 
                          targs->student_counter_mut);
    }
    pthread_mutex_unlock(targs->student_counter_mut);
    printf("Teacher: All students have arrived. I start to assign group ids"
           " to students.\n"); 
    
    /* assign the students to groups */
    
    // the ith entry of 'stud_left_to_fill_group' stores the number of 
    // additional students needed to fill up the group.
    int* stud_left_to_fill_group = malloc(sizeof(int) * targs->M);
    if (targs->M % 2 == 1)
    {
        stud_left_to_fill_group[0] = targs->N / targs->M + 1;
        for (int i = 1; i < targs->M; i++)
            stud_left_to_fill_group[i] = targs->N / targs->M;
    }
    else for (int i = 0; i < targs->M; i++) 
        stud_left_to_fill_group[i] = targs->M / targs->N;
    
    for (int i = 0; i < targs->N; i++)
    {
        int new_group = -1;
        bool found_group_with_space = false;
        while (!found_group_with_space)
        {
            new_group = get_rand_num(0, targs->M - 1);
            if (stud_left_to_fill_group[new_group] > 0)
                found_group_with_space = true;
        }
        pthread_mutex_lock(&targs->student_args_arr[i].assigned_lab_mut);
        targs->student_args_arr[i].assigned_group = new_group;
        pthread_cond_signal(&targs->student_args_arr[i].assigned_group_cond);
        pthread_mutex_unlock(&targs->student_args_arr[i].assigned_lab_mut);
        printf("Teacher: student %d is in group %d.\n", i, new_group);
        stud_left_to_fill_group[new_group]--;
    }
    pthread_mutex_lock()
    printf("Teacher: I'm waiting for lab rooms to become available.\n");
    
    
    /* free resources */
    
    free(stud_left_to_fill_group); 
    
    return NULL;
}

void* student(void* args)
{
    student_args* sargs = (student_args*)args;
    
    /* increment the shared student_counter variable */
    
    pthread_mutex_lock(sargs->student_counter_mut);
    (*sargs->student_counter)++; 
    pthread_mutex_unlock(sargs->student_counter_mut);
    
    // signal for teacher to know when all students have arrived.
    if (sargs->N == *sargs->student_counter)
        pthread_cond_signal(sargs->all_students_arrived);
    
    printf("Student %d: I have arrived and wait for being assigned to a group."
           "\n", sargs->student_id);
    
    pthread_mutex_lock(&sargs->assigned_group_mut);
    while (GROUP_UNASSIGNED == sargs->assigned_group)
        pthread_cond_wait(&sargs->assigned_group_cond, 
                          &sargs->assigned_group_mut);
    printf("Student %d: OK, I'm in group %d and waiting for my turn to enter a "
           "lab room\n", sargs->student_id, sargs->assigned_group);
    pthread_mutex_unlock(&sargs->assigned_group_mut);
    
    pthread_mutex_lock(&sargs->assigned_lab_mut);
    while (LAB_UNASSIGNED == sargs->assigned_lab)
        pthread_cond_wait(&sargs->assigned_group_cond, 
                          &sargs->assigned_lab_mut);
    pthread_mutex_unlock(&sargs->assigned_lab_mut);
    printf("Student %d in group %d: My group is called. I will enter the lab"
           " room %d\n", sargs->student_id, sargs->assigned_group, 
           sargs->assigned_lab);
    
    return NULL;
}

// later i should refactor the code into a big classroom object sort of thing
// this will make the code much more readable.
/*
typedef struct classroom
{
   int* labs;
   int lab_ct;
   
};
*/

int main()
{
    /* retrieve config info */
    
    int N; // number of students
    int M; // number of groups
    int K; // number of tutors, lab groups
    int T; // time limit for each group of students to do the lab exercise
    printf("Enter number of students, N: ");
    if (get_int_from_stdin(&N)) return 1;
    printf("Enter number of groups, M: ");
    if (get_int_from_stdin(&M)) return 1;
    printf("Enter number of tutors, lab groups: ");
    if (get_int_from_stdin(&K)) return 1;
    printf("Enter lab exercise time limit: ");
    if (get_int_from_stdin(&T)) return 1;
    DEBUG_PRINT("Config retrieved. N=%d, M=%d, K=%d, T=%d\n", N, M, K, T);
    
    /* create student thread args */
    
    // counts the number of students that have arrived in the classroom
    int student_counter = 0;
    pthread_mutex_t student_counter_mut;
    pthread_mutex_init(&student_counter_mut, NULL);
    pthread_cond_t all_students_arrived;
    pthread_cond_init(&all_students_arrived, NULL);
    student_args* student_args_arr = malloc(sizeof(student_args) * N);
    for (int i = 0; i < N; i++)
    {
        student_args_arr[i].student_id = i;
        pthread_mutex_init(&student_args_arr[i].assigned_lab_mut, NULL);
        student_args_arr[i].assigned_group = GROUP_UNASSIGNED;
        student_args_arr[i].N = N;
        student_args_arr[i].student_counter = &student_counter;
        student_args_arr[i].student_counter_mut = &student_counter_mut;
        student_args_arr[i].all_students_arrived = &all_students_arrived;
    }
    
    /* create teacher thread args and launch teacher thread */
    
    teacher_args* targs = malloc(sizeof(teacher_args));
    targs->student_counter_mut = &student_counter_mut;
    targs->student_counter = &student_counter;
    targs->student_args_arr = student_args_arr;
    targs->N = N;
    targs->M = M;
    targs->all_students_arrived = &all_students_arrived;
    pthread_t teacher_thread;
    pthread_create(&teacher_thread, NULL, teacher, targs);
    
    /* create tutor thread args */
    
    tutor_args* tutor_args = malloc(sizeof(tutor) * K);
    
    
    /* launch student threads */
    
    pthread_t* student_threads = malloc(sizeof(pthread_t) * N);
    for (int i = 0; i < N; i++)
        pthread_create(student_threads + i, NULL, student, student_args_arr +i);
    
    /* join student and teacher threads */
    
    for (int i = 0; i < N; i++)
        pthread_join(student_threads[i], NULL);
    pthread_join(teacher_thread, NULL);
    
    /* free resources */
    
    for (int i = 0; i < N; i++)
    {
        pthread_mutex_destroy(&student_args_arr[i].assigned_lab_mut);
        pthread_cond_destroy(&student_args_arr[i].assigned_lab_cond);
    }
    free(student_args_arr);
    free(student_threads);
    free(tutor_args);
    free(targs);
    pthread_mutex_destroy(&student_counter_mut);
    pthread_cond_destroy(&all_students_arrived);
    return 0;
}