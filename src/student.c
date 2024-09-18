#include "../include/utils.h"
#include "../include/student.h"
#include "../include/school.h"

#include <stdio.h>

extern school_t* school;

void* student(void* args)
{
    int sid = *(int*)args;

    mtsafe_printf(&school->print_mut, "Student %d: I have arrived and wait for"
        "being assigned to a group.\n", sid);
    
    /* increment the shared student_counter variable */
    
    pthread_mutex_lock(&school->student_counter_mut);
    school->student_counter++; 
    pthread_mutex_unlock(&school->student_counter_mut);
    
    /* signal teacher if all students have arrived */ 
    
    if (school->N == school->student_counter)
        pthread_cond_signal(&school->all_students_at_school_cond);

    /* wait until assigned to a group */ 
    
    pthread_mutex_lock(school->stud_group_assignment_muts + sid);
    while (GROUP_UNASSIGNED == school->stud_group_assignments[sid])
        pthread_cond_wait(school->stud_assigned_group_conds + sid, 
                          school->stud_group_assignment_muts + sid);
    pthread_mutex_unlock(school->stud_group_assignment_muts + sid);
    // This student will never be assigned to a different group, so 
    // for convenience we store the group in the local variable 'my_group'.
    int my_group = school->stud_group_assignments[sid];
    
    /* wait until assigned a lab */

    pthread_mutex_lock(school->lab_assignment_muts + my_group);
    while (LAB_UNASSIGNED == school->lab_assignments[my_group])
    {
        mtsafe_printf(&school->print_mut, "Student %d: OK, I'm in group %d "
            "and waiting for my turn to enter a lab room\n", sid, my_group);
        DEBUG_PRINT("stu %d in group %d: waiting to be assigned to lab\n", 
            sid, my_group);
        pthread_cond_wait(school->group_assigned_to_lab_conds + my_group, 
                          school->lab_assignment_muts + my_group);
        DEBUG_PRINT("stu %d in group %d: assigned to lab %d\n", sid,
                    my_group, school->lab_assignments[my_group]);
    }
    pthread_mutex_unlock(school->lab_assignment_muts + my_group);
    // This student's group will never be assigned to a different lab, so
    // for convenience we store the lab in the local variable 'my_lab'.
    int my_lab = school->lab_assignments[my_group];
    
    mtsafe_printf(&school->print_mut, "Student %d in group %d: My group "
        "is called. I will enter the lab room %d\n", sid, my_group, my_lab);

    /* signal the tutor if all students have arrived */ 
    
    bool signal_all_stud_present = false;
    pthread_mutex_lock(school->lab_attendance_counter_muts + my_lab);
    (school->lab_attendance_counters[my_lab])++;
    signal_all_stud_present = school->lab_attendance_counters[my_lab] 
                              >= school->group_sizes[my_group];
    pthread_mutex_unlock(school->lab_attendance_counter_muts + my_lab);
    
    if (signal_all_stud_present)
    {
        DEBUG_PRINT("student %d: signal for all students present called\n", sid);
        pthread_cond_signal(school->all_students_present_conds + my_lab);
    }

    /* wait for the tutor to broadcast that the lab is completed */
    
    pthread_mutex_lock(school->lab_complete_flag_muts + my_group);
    while (!school->lab_complete_flags[my_group])
    {
        pthread_cond_wait(school->lab_complete_conds + my_group,
            school->lab_complete_flag_muts + my_group);  
    }
    pthread_mutex_unlock(school->lab_complete_flag_muts + my_group);
    
    mtsafe_printf(&school->print_mut, "Student %d in group %d: Thanks Tutor %d."
                                      " Bye!\n", sid, my_group, my_lab);
    
   	/* signal the tutor if all students of the group are gone */ 
    
    pthread_mutex_lock(school->group_studs_gone_counter_muts + my_group);
    if (++school->group_studs_gone_counter[my_group]) 
		pthread_cond_signal(school->group_gone_conds + my_group);
    pthread_mutex_unlock(school->group_studs_gone_counter_muts + my_group);
    
    return NULL;
}