#include "../include/utils.h"
#include "../include/tutor.h"

#include <unistd.h>

void* tutor(void* args)
{
    tutor_args* tut_args = (tutor_args*)args;

    int students_gone = num_students_left(tut_args->students_gone_counter_mut, tut_args->students_gone_counter);

    bool tutor_required = tut_args->id < tut_args->M;
    if (tutor_required)
    {
        while (students_gone < tut_args->N)
        {
            /* reset lab session variables */

            tut_args->entering_group = GROUP_UNASSIGNED;
            tut_args->lab_complete_flags[tut_args->id] = false;
            tut_args->lab_attendance_counters[tut_args->id] = 0;
            
            /* join the available tutor/lab queue */

            pthread_mutex_lock(tut_args->avail_lab_queue_mut);
            mtsafe_printf(tut_args->print_mut, "Tutor %d: The lab room %d is vacated and ready for one "
                   " group\n", tut_args->id, tut_args->id);
            ring_buf_add(tut_args->avail_lab_queue, tut_args->id);
            pthread_mutex_unlock(tut_args->avail_lab_queue_mut);
            
            /* signal to the teacher that the available lab queue isn't empty */

            pthread_cond_signal(tut_args->avail_lab_queue_not_empty);

            /* wait to be assigned a group */
            
            pthread_mutex_lock(&tut_args->entering_group_mut);
            while (GROUP_UNASSIGNED == tut_args->entering_group)
                pthread_cond_wait(&tut_args->entering_group_assigned, 
                                  &tut_args->entering_group_mut);
            pthread_mutex_unlock(&tut_args->entering_group_mut);
            
            /*  wait till the whole group has entered the lab */
            
            DEBUG_PRINT("tutor %d: I am aware that I have been assigned a group\n", 
                   tut_args->id);

            DEBUG_PRINT("tut %d: Locking mutex at %p\n", tut_args->id,  
                        &tut_args->lab_attendance_counter_muts[tut_args->id]);
            pthread_mutex_lock(
                    &tut_args->lab_attendance_counter_muts[tut_args->id]);
            while (tut_args->lab_attendance_counters[tut_args->id] < 
                tut_args->group_sizes[tut_args->entering_group])
            {
                DEBUG_PRINT("tutor %d: waiting for a student to send an all students present signal\n", 
                            tut_args->id);
                pthread_cond_wait(&tut_args->all_students_present_conds[tut_args->id],
                                  &tut_args->lab_attendance_counter_muts[tut_args->id]);
            }
            
            DEBUG_PRINT("tutor %d: line %d executed\n", tut_args->id, __LINE__);
                
            pthread_mutex_unlock(
                    &tut_args->lab_attendance_counter_muts[tut_args->id]);
            mtsafe_printf(tut_args->print_mut, "Tutor %d: All students in group %d have entered the lab room"
                   " %d now.\n", tut_args->id, tut_args->entering_group, 
                   tut_args->id);

            /* simulate lab time */
            
            int exercise_duration = get_rand_num(tut_args->T/2, tut_args->T);
            sleep(exercise_duration);
            mtsafe_printf(tut_args->print_mut, "Tutor %d: students in group %d have completed the lab exercise"
                   " in %d units of time. You may leave this room now.\n",
                   tut_args->id, tut_args->entering_group, exercise_duration);

            /* update lab complete flag and broadcast, so students know lab is complete */
            
            tut_args->lab_complete_flags[tut_args->id] = true;
            pthread_cond_broadcast(tut_args->lab_complete_conds + tut_args->id);

           
            
            students_gone = num_students_left(tut_args->students_gone_counter_mut, tut_args->students_gone_counter);
        } 
    }
    

    /* wait for teacher to allow going home */

    pthread_mutex_lock(tut_args->tutors_can_go_home_mut);
    while (!(*tut_args->tutors_can_go_home))
        pthread_cond_wait(tut_args->tutors_can_go_home_cond, tut_args->tutors_can_go_home_mut);
    pthread_mutex_unlock(tut_args->tutors_can_go_home_mut);

    /* update tutors gone counter, signal teacher if all tutors gone */

    bool signal_teacher = false;
    pthread_mutex_lock(tut_args->tutors_gone_counter_mut);
    (*tut_args->tutors_gone_counter)++;
    signal_teacher = *tut_args->tutors_gone_counter == tut_args->K; 
    pthread_mutex_unlock(tut_args->tutors_gone_counter_mut);
    
    mtsafe_printf(tut_args->print_mut, "Tutor %d: Thanks teacher. Bye!\n", tut_args->id);

    if (signal_teacher) pthread_cond_signal(tut_args->tutors_gone_cond);
    
    return NULL;
}