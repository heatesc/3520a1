#include "../include/utils.h"
#include "../include/student.h"

#include <stdio.h>


void* student(void* args)
{
    student_args* sargs = (student_args*)args;

    printf("Student %d: I have arrived and wait for being assigned to a group."
           "\n", sargs->student_id);
    
    /* increment the shared student_counter variable */
    
    pthread_mutex_lock(sargs->student_counter_mut);
    (*sargs->student_counter)++; 
    pthread_mutex_unlock(sargs->student_counter_mut);
    
    // signal for teacher to know when all students have arrived.
    if (sargs->N == *sargs->student_counter)
        pthread_cond_signal(sargs->all_students_arrived);

    DEBUG_PRINT("line %d reached\n", __LINE__);
    
    
    pthread_mutex_lock(&sargs->assigned_group_mut);
    while (GROUP_UNASSIGNED == sargs->assigned_group)
        pthread_cond_wait(&sargs->assigned_group_cond, 
                          &sargs->assigned_group_mut);
    
    pthread_mutex_unlock(&sargs->assigned_group_mut);

    pthread_mutex_lock(&sargs->lab_assignment_muts[sargs->assigned_group]);
    DEBUG_PRINT("stu %d in group %d, assgined lab: %d\n", sargs->student_id,
                sargs->assigned_group, sargs->lab_assignments[sargs->assigned_group]);
    while (LAB_UNASSIGNED == sargs->lab_assignments[sargs->assigned_group])
    {
        mtsafe_printf(sargs->print_mut, "Student %d: OK, I'm in group %d and waiting for my turn to enter a "
           "lab room\n", sargs->student_id, sargs->assigned_group);
        DEBUG_PRINT("stu %d in group %d: waiting to be assigned to lab\n", 
                    sargs->student_id, sargs->assigned_group);
        pthread_cond_wait(&sargs->group_assigned_to_lab_conds[sargs->assigned_group], 
                          &sargs->lab_assignment_muts[sargs->assigned_group]);
        DEBUG_PRINT("stu %d in group %d: assigned to lab %d\n", sargs->student_id,
                    sargs->assigned_group, sargs->lab_assignments[sargs->assigned_group]);
    }
    pthread_mutex_unlock(&sargs->lab_assignment_muts[sargs->assigned_group]);
    
    mtsafe_printf(sargs->print_mut, "Student %d in group %d: My group is called. I will enter the lab"
           " room %d\n", sargs->student_id, sargs->assigned_group, 
           sargs->lab_assignments[sargs->assigned_group]);

    DEBUG_PRINT("line %d reached\n", __LINE__);
    bool signal_all_stud_present = false;
    DEBUG_PRINT("stu: BEFORE pointer to lab attendance ctr mut: %p\n", &sargs->lab_attendance_counter_mut[sargs->lab_assignments[sargs->assigned_group]]);
    pthread_mutex_lock(&sargs->lab_attendance_counter_muts[sargs->lab_assignments[sargs->assigned_group]]);
    DEBUG_PRINT("counter before increment: %d\n", sargs->lab_attendance_counters[sargs->lab_assignments[sargs->assigned_group]]);
    (sargs->lab_attendance_counters[sargs->lab_assignments[sargs->assigned_group]])++;
    DEBUG_PRINT("counter after increment: %d\n", sargs->lab_attendance_counters[sargs->lab_assignments[sargs->assigned_group]]);
    signal_all_stud_present = sargs->lab_attendance_counters[sargs->lab_assignments[sargs->assigned_group]] >= sargs->group_sizes[sargs->assigned_group];
    pthread_mutex_unlock(&sargs->lab_attendance_counter_muts[sargs->lab_assignments[sargs->assigned_group]]);
    
    if (signal_all_stud_present)
    {
        DEBUG_PRINT("student %d: signal for all students present called\n", sargs->student_id);
        pthread_cond_signal(&sargs->all_students_present_conds[sargs->lab_assignments[sargs->assigned_group]]);
    }

    /* wait for the tutor to broadcast that the lab is completed */
    
    pthread_mutex_lock(&sargs->lab_complete_flag_muts[sargs->lab_assignments[sargs->assigned_group]]);
    while (!sargs->lab_complete_flags[sargs->lab_assignments[sargs->assigned_group]])
        pthread_cond_wait(sargs->lab_complete_conds + sargs->lab_assignments[sargs->assigned_group],
            &sargs->lab_complete_flag_muts[sargs->lab_assignments[sargs->assigned_group]]);
    pthread_mutex_unlock(&sargs->lab_complete_flag_muts[sargs->lab_assignments[sargs->assigned_group]]);
    
    mtsafe_printf(sargs->print_mut, "Student %d in group %d: Thanks Tutor %d. Bye!\n", sargs->student_id,
           sargs->assigned_group, 
           sargs->lab_assignments[sargs->assigned_group]);
    
    /* update the shared students gone counter */

    pthread_mutex_lock(&sargs->students_gone_counter_mut);
    (*sargs->students_gone_counter)++;
    pthread_mutex_unlock(&sargs->students_gone_counter_mut);

    /* inform the teacher if this is the last student */
    
    if ((*sargs->students_gone_counter >= sargs->N))
        pthread_cond_signal(sargs->students_gone_cond);
    
    return NULL;
}