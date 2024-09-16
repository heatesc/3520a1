#include "../include/utils.h"
#include "../include/teacher.h"

#include <stdio.h>
#include <malloc.h>


void* teacher(void* args)
{
    teacher_args* targs = (teacher_args*)args;
    printf("Teacher: I'm waiting for all the students to arrive.\n");
    pthread_mutex_lock(targs->student_counter_mut);
    while (*targs->student_counter < targs->N)
        pthread_cond_wait(targs->all_students_arrived, 
                          targs->student_counter_mut);
    pthread_mutex_unlock(targs->student_counter_mut);
    printf("Teacher: All students have arrived. I start to assign group ids"
           " to students.\n"); 
    
    /* assign the students to groups */
    
    // the ith entry of 'stud_left_to_fill_group' stores the number of 
    // additional students needed to fill up the group.
    int* stud_left_to_fill_group = malloc(sizeof(int) * targs->M);
    if (targs->N % targs->M != 0)
    {
        for (int i = 0; i < targs->N % targs->M; i++)
            targs->group_sizes[i] = targs->N / targs->M + 1;
        for (int i = targs->N % targs->M; i < targs->M; i++)
            targs->group_sizes[i] = targs->N / targs->M;
    }
    else for (int i = 0; i < targs->M; i++)
    {
        targs->group_sizes[i] = targs->N / targs->M;
        DEBUG_PRINT("size of group %d set to %d\n", i, targs->group_sizes[i]);
    }
    for (int i = 0; i < targs->M; i++)
        stud_left_to_fill_group[i] = targs->group_sizes[i];
    DEBUG_PRINT("Teacher: Group sizes have been set\n");
    
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
        pthread_mutex_lock(&targs->student_args_arr[i].assigned_group_mut);
        targs->student_args_arr[i].assigned_group = new_group;
        pthread_cond_signal(&targs->student_args_arr[i].assigned_group_cond);
        pthread_mutex_unlock(&targs->student_args_arr[i].assigned_group_mut);
        mtsafe_printf(targs->print_mut, "Teacher: student %d is in group %d.\n", i, new_group);
        stud_left_to_fill_group[new_group]--;
    }
    
    /* assign groups to labs */
    
    for (int group = 0; group < targs->M; group++)
    {
        /* get an available tutor/lab from the available lab queue */
        
        pthread_mutex_lock(targs->avail_lab_queue_mut);
        while (ring_buf_empty(targs->avail_lab_queue))
        {
            printf("Teacher: I'm waiting for lab rooms to become available.\n");
            pthread_cond_wait(targs->lab_is_avail_cond,
                              targs->avail_lab_queue_mut);
        }
        int avail_lab = ring_buf_remove(targs->avail_lab_queue);

        
        pthread_mutex_unlock(targs->avail_lab_queue_mut);

        mtsafe_printf(targs->print_mut, "Teacher: The lab %d is now available. Students in group %d can"
               " enter the room and start your lab exercise.\n", avail_lab, group);
        
        pthread_mutex_lock(&targs->tutor_args_arr[avail_lab]
            .entering_group_mut);
        DEBUG_PRINT("assigning entering group of lab %d to group %d\n", 
                    avail_lab, group);
        targs->tutor_args_arr[avail_lab].entering_group = group;
        // wake up the tutor/lab thread
        pthread_cond_signal(&targs->tutor_args_arr[avail_lab]
            .entering_group_assigned);
        pthread_mutex_unlock(&targs->tutor_args_arr[avail_lab]
            .entering_group_mut);

        
        /* assign the retrieved available tutor to the current group */
        
        
        pthread_mutex_lock(&targs->lab_assignment_muts[group]);
        targs->lab_assignments[group] = avail_lab;
        pthread_mutex_unlock(&targs->lab_assignment_muts[group]);
        
        // wake up students threads of the current lab
        pthread_cond_broadcast(&targs->group_assigned_to_lab_conds[group]);
    }
    
    pthread_mutex_lock(targs->students_gone_counter_mut);
        while (*(targs->students_gone_counter) < targs->N)
            pthread_cond_wait(targs->students_gone_cond, 
                              targs->student_counter_mut);
         pthread_mutex_unlock(targs->students_gone_counter_mut);
    for (int tutor = 0; tutor < targs->K; tutor++) 
        mtsafe_printf(targs->print_mut, "Teacher: there are no students waiting. Tutor %d, you can"
               " go home now.\n", tutor);
    *(targs->tutors_can_go_home) = true;
    pthread_cond_broadcast(targs->tutors_can_go_home_cond);


    /* wait for all tutors to go */

    DEBUG_PRINT("Teacher thread is now waiting");
    

    pthread_mutex_lock(targs->tutors_gone_counter_mut);
    while (*targs->tutors_gone_counter < targs->K)
        pthread_cond_wait(targs->tutors_gone_cond, targs->tutors_gone_counter_mut);
    pthread_mutex_unlock(targs->tutors_gone_counter_mut);
    
    mtsafe_printf(targs->print_mut, "Teacher: All students and tutors are left. I can now go home"
           ".\n");
    
    /* free resources */
    
    free(stud_left_to_fill_group); 
    
    return NULL;
}