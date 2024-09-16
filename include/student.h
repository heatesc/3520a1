#ifndef STUDENT_H
#define STUDENT_H

#include <stdbool.h>

typedef struct
{
    int student_id;
    pthread_mutex_t* print_mut;
    int assigned_group;
    int* group_sizes;
    pthread_mutex_t* student_counter_mut;
    int* student_counter;
    pthread_mutex_t assigned_group_mut;
    pthread_cond_t assigned_group_cond;
    // this is used to signal to the teacher
    // that all students have arrived.
    pthread_cond_t* all_students_arrived;
    int N;
    pthread_cond_t* group_assigned_to_lab_conds;
    // entry i of 'lab_assignments' is the lab_id of the lab assigned to 
    // group i. entries are initialised to LAB_UNASSIGNED.
    int* lab_assignments;
    pthread_mutex_t* lab_assignments_mut;
    pthread_mutex_t* lab_assignment_muts;
    bool* running;
    int* lab_attendance_counter;
    pthread_mutex_t* lab_attendance_counter_mut;
    // represents condiiton: all studnets in group i present in their 
    // allocated lab
    pthread_cond_t* all_students_present_conds;
    pthread_mutex_t* all_students_present_muts;
    int* group_capacities;
    int* students_gone_counter;
    pthread_mutex_t students_gone_counter_mut;
    pthread_cond_t* students_gone_cond;
    pthread_cond_t* lab_complete_conds;
    bool* lab_complete_flags;
    pthread_mutex_t* lab_complete_flag_muts;
    int* lab_attendance_counters;
    pthread_mutex_t* lab_attendance_counter_muts;
} student_args;


void* student(void* args);

#endif //STUDENT_H
