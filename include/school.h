#ifndef SCHOOL_H
#define SCHOOL_H

#include <pthread.h>

#include "ring_buf.h"
#include "utils.h"

typedef struct school_t
{
    // 'print_mut' is used to lock print access, ensuring thread safety.
    pthread_mutex_t print_mut;

    // stud_group_assignments[i] stores the group that student i is assigned to 
    int* stud_group_assignments;
    pthread_mutex_t* stud_group_assignment_muts;
    pthread_cond_t* stud_assigned_group_conds;
    
    // Available labs are stored in 'ring_buf'
    ring_buf* avail_lab_queue;
    pthread_mutex_t avail_lab_queue_mut;
    pthread_cond_t lab_is_avail_cond;
    
    int* lab_attendance_counters;
    pthread_mutex_t* lab_attendance_counter_muts;
    // all_students_present_conds[i] is used to indicate that all students
    // in the group currently assigned to lab i are present.
    pthread_cond_t* all_students_present_conds;
    
    // lab_assignments[i] stores the lab assigned to group i.
    int* lab_assignments;
    pthread_mutex_t* lab_assignment_muts;
    
    int students_gone_counter;
    pthread_mutex_t students_gone_counter_mut;
    pthread_cond_t students_gone_cond;
    
    int* group_sizes;

    // lab_complete_flags[i] stores whether the lab for group i is complete.
    bool* lab_complete_flags;
    pthread_mutex_t* lab_complete_flag_muts;
    // Used to indicate that a group's lab session has finished.
    pthread_cond_t* lab_complete_conds;
    
    // The teacher thread should set 'tutors_can_go_home' to true,
    // once all students have gone home.
    bool tutors_can_go_home;
    pthread_mutex_t tutors_can_go_home_mut;
    pthread_cond_t tutors_can_go_home_cond;
    int tutors_gone_counter;
    pthread_mutex_t tutors_gone_counter_mut;
    pthread_cond_t tutors_gone_cond;
    
    int student_counter;
    pthread_mutex_t student_counter_mut;
    pthread_cond_t all_students_at_school_cond;

    // group_assigned_to_lab_conds[i] is used to indicate that group i
    // has been assigned to a lab.
    pthread_cond_t* group_assigned_to_lab_conds;
    
    // entering_group[i] stores the group currently entering or in lab i.
    int* entering_group;
    pthread_mutex_t* entering_group_muts;
    pthread_cond_t* entering_group_assigned_conds;

    // this stores the number of students gone. the purpose of this variable
    // is for a tutor to know when all students of a given group have exited
    // the lab. group_studs_gone_counter[i] stores the number of students gone
    // in group i.
    int* group_studs_gone_counter;
    pthread_mutex_t* group_studs_gone_counter_muts;
    pthread_cond_t* group_gone_conds;

    // all_groups_assigned_flag is used to indicate to tutors that the
    // teacher has completed assigning students to groups.
    pthread_mutex_t all_groups_assigned_flag_mut;
    bool all_groups_assigned_flag;
    pthread_cond_t all_groups_assigned_cond;

    /* config variables */

    int N;
    int M;
    int K;
    int T;
} school_t;

// returns 0 if successful
int school_init();
void school_run();
void school_destroy();

#endif //SCHOOL_H