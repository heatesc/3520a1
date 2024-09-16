#ifndef TEACHER_H
#define TEACHER_H

#include "../include/student.h"
#include "../include/ring_buf.h"
#include "../include/tutor.h"

typedef struct
{
    int* group_sizes;
    pthread_mutex_t* print_mut;
    pthread_cond_t* lab_is_avail_cond;
    pthread_cond_t* all_students_arrived;
    pthread_mutex_t* student_counter_mut;
    student_args* student_args_arr;
    ring_buf* avail_lab_queue;
    pthread_mutex_t* avail_lab_queue_mut;
    tutor_args* tutor_args_arr;
    int* student_counter;
    int N;
    int M;
    bool* running;
    pthread_mutex_t* running_mut;
    pthread_cond_t* group_assigned_to_lab_conds;
    // entry i of 'lab_assignments' is the lab_id of the lab assigned to 
    // group i. entries are initialised to LAB_UNASSIGNED.
    int* lab_assignments;
    pthread_mutex_t* lab_assignments_mut;
    pthread_cond_t* students_gone_cond;
    pthread_mutex_t* students_gone_counter_mut;
    int* students_gone_counter;
    int K;
    // pthread_cond_t* tutor_done_cond;
    bool* tutors_can_go_home;
    pthread_mutex_t* tutors_can_go_home_mut;
    pthread_cond_t* tutors_can_go_home_cond;
    int* tutors_gone_counter;
    pthread_mutex_t* tutors_gone_counter_mut;
    pthread_cond_t* tutors_gone_cond;
    pthread_mutex_t* lab_assignment_muts;
    int* lab_attendance_counters;
    pthread_mutex_t* lab_attendance_counter_muts;
    bool* early_exit_flags;
    pthread_cond_t* early_exit_conds;
    pthread_mutex_t* early_exit_flag_muts;
} teacher_args;

void* teacher(void* args);

#endif //TEACHER_H
