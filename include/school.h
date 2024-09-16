#ifndef SCHOOL_H
#define SCHOOL_H

#include <pthread.h>

#include "ring_buf.h"
#include "utils.h"

typedef struct
{
    // This is the config, as given through stdin.
    config* conf;

    
    // 'print_mut' is used to lock print access, ensuring thread safety.
    pthread_mutex_t print_mut;
    
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
    
    bool* lab_complete_flags;
    pthread_mutex_t lab_complete_flag_muts;
    // Used to indicate a lab session has finished.
    pthread_cond_t lab_complete_conds;
    
    // The teacher thread should set 'tutors_can_go_home' to true,
    // once all students have gone home.
    bool tutors_can_go_home;
    pthread_cond_t tutors_can_go_home_cond;
    int tutors_gone_counter;
    pthread_mutex_t tutors_gone_counter_mut;
    pthread_cond_t tutors_gone_cond;
    
    int student_counter;
    pthread_mutex_t student_counter_mut;
    pthread_cond_t all_students_at_school_cond;
    
    pthread_cond_t* group_assigned_to_lab_conds;
    
    // entering_group[i] stores the group currently entering or in lab i.
    int* entering_group;
    
} school_t;


school_t* school_init();
void school_run(school_t* school);
void school_destroy(school_t* school);

#endif //SCHOOL_H
