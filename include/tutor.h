#ifndef TUTOR_H
#define TUTOR_H

#include "ring_buf.h"

typedef struct
{
    int id;
    int* group_sizes;
    pthread_mutex_t* avail_lab_queue_mut;
    ring_buf* avail_lab_queue;
    pthread_mutex_t* print_mut;
    int entering_group;
    pthread_mutex_t entering_group_mut;
    pthread_cond_t entering_group_assigned;
    pthread_cond_t* avail_lab_queue_not_empty;
    bool* running;
    pthread_cond_t* group_assigned_to_lab_cond;
    // entry i of 'lab_assignments' is the lab_id of the lab assigned to 
    // group i. entries are initialised to LAB_UNASSIGNED.
    int* lab_assignments;
    pthread_mutex_t* lab_assignments_mut;
    pthread_cond_t* all_students_present;
    int lab_attendance_counter;
    int lab_attendance_counter_mut;
    int T;
    int* group_capacities;
    int* students_gone_counter;
    pthread_mutex_t* students_gone_counter_mut;
    bool* tutors_can_go_home;
    pthread_mutex_t* tutors_can_go_home_mut;
    pthread_cond_t* tutors_can_go_home_cond;
    int N;
    pthread_cond_t* lab_complete_conds;
    bool* lab_complete_flags;
    pthread_mutex_t* lab_complete_flag_muts;
    int* tutors_gone_counter;
    pthread_mutex_t* tutors_gone_counter_mut;
    pthread_cond_t* tutors_gone_cond;
    pthread_mutex_t* lab_assignment_muts;
    int K;
    pthread_cond_t* all_students_present_conds;
    int* lab_attendance_counters;
    pthread_mutex_t* lab_attendance_counter_muts;
    int M;
    bool* early_exit_flags;
    pthread_mutex_t* early_exit_flag_muts;
    pthread_cond_t* early_exit_conds;
} tutor_args;

void* tutor(void* args);

#endif //TUTOR_H
