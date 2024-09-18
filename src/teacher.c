#include "../include/utils.h"
#include "../include/teacher.h"
#include "../include/school.h"

#include <stdio.h>
#include <malloc.h>

extern school_t* school;

void* teacher(void* args)
{
    mtsafe_printf(&school->print_mut, 
                "Teacher: I'm waiting for all the students to arrive.\n");
    
    /* wait till all students have arrived */
    
    pthread_mutex_lock(&school->student_counter_mut);
    while (school->student_counter < school->N)
        pthread_cond_wait(&school->all_students_at_school_cond, 
                          &school->student_counter_mut);
    pthread_mutex_unlock(&school->student_counter_mut);
    
    mtsafe_printf(&school->print_mut, "Teacher: All students have arrived. "
		"I start to assign group ids to students.\n"); 
    
    /* assign the students to groups */
    
    // the ith entry of 'stud_left_to_fill_group' stores the number of 
    // additional students needed to fill up the group.
    int* stud_left_to_fill_group = malloc(sizeof(int) * school->M);
    if (school->N % school->M != 0)
    {
        for (int i = 0; i < school->N % school->M; i++)
            school->group_sizes[i] = school->N / school->M + 1;
        for (int i = school->N % school->M; i < school->M; i++)
            school->group_sizes[i] = school->N / school->M;
    }
    else for (int i = 0; i < school->M; i++)
    {
        school->group_sizes[i] = school->N / school->M;
        DEBUG_PRINT("size of group %d set to %d\n", i, school->group_sizes[i]);
    }
    for (int i = 0; i < school->M; i++)
        stud_left_to_fill_group[i] = school->group_sizes[i];
    DEBUG_PRINT("Teacher: Group sizes have been set\n");
    
    for (int i = 0; i < school->N; i++)
    {
        int new_group = -1;
        bool found_group_with_space = false;
        while (!found_group_with_space)
        {
            new_group = get_rand_num(0, school->M - 1);
            if (stud_left_to_fill_group[new_group] > 0)
                found_group_with_space = true;
        }
//        pthread_mutex_lock(school->stud_group_assignment_muts + i);
        school->stud_group_assignments[i] = new_group;
        pthread_cond_signal(school->stud_assigned_group_conds + i);
//        pthread_mutex_unlock(school->stud_group_assignment_muts + i);
        mtsafe_printf(&school->print_mut, "Teacher: student %d is in group "
                                              "%d.\n", i, new_group);
        stud_left_to_fill_group[new_group]--;
    }
    
    /* indicate to the tutors that all students have been assigned to groups */
    
	pthread_mutex_lock(&school->all_groups_assigned_flag_mut);
	school->all_groups_assigned_flag = true;
	pthread_cond_broadcast(&school->all_groups_assigned_cond);
    pthread_mutex_unlock(&school->all_groups_assigned_flag_mut);    
    
    /* assign groups to labs */
    
    for (int group = 0; group < school->M; group++)
    {
        /* get an available tutor/lab from the available lab queue */
        
        pthread_mutex_lock(&school->avail_lab_queue_mut);
        while (ring_buf_empty(school->avail_lab_queue))
        {
            mtsafe_printf(&school->print_mut, "Teacher: I'm waiting for lab "
                                            "rooms to become available.\n");
            pthread_cond_wait(&school->lab_is_avail_cond,
                              &school->avail_lab_queue_mut);
        }
        int avail_lab = ring_buf_remove(school->avail_lab_queue);
        pthread_mutex_unlock(&school->avail_lab_queue_mut);

        mtsafe_printf(&school->print_mut, "Teacher: The lab %d is now"
			" available. Students in group %d can enter the room and start "
			"your lab exercise.\n", avail_lab, group);  
        
        pthread_mutex_lock(school->entering_group_muts + avail_lab);
        DEBUG_PRINT("assigning entering group of lab %d to group %d\n", 
                    avail_lab, group);
        school->entering_group[avail_lab] = group;
        // wake up the tutor/lab thread
        pthread_mutex_unlock(school->entering_group_muts + avail_lab);
        pthread_cond_signal(school->entering_group_assigned_conds + avail_lab);
        
        /* assign the retrieved available tutor to the current group */
        
        pthread_mutex_lock(school->lab_assignment_muts + group);
        school->lab_assignments[group] = avail_lab;
        pthread_mutex_unlock(school->lab_assignment_muts + group);
        
        // wake up students threads of the current lab
        pthread_cond_broadcast(school->group_assigned_to_lab_conds + group);
    }
    
    pthread_mutex_lock(&school->students_gone_counter_mut);
        while (school->students_gone_counter < school->N)
            pthread_cond_wait(&school->students_gone_cond, 
                              &school->students_gone_counter_mut);
    pthread_mutex_unlock(&school->students_gone_counter_mut);
    
    for (int tutor = 0; tutor < school->K; tutor++) 
        mtsafe_printf(&school->print_mut, "Teacher: there are no students"
			" waiting. Tutor %d, you can go home now.\n", tutor);
    
   	/* set assigned lab of all tutors to LAB_UNAVAILABLE to indicate no 
	 * groups left, send corresponding signals */
    
   	for (int tutor_id = 0; tutor_id < school->K; tutor_id++)
	{
		pthread_mutex_lock(school->entering_group_muts + tutor_id);
		school->entering_group[tutor_id] = GROUP_UNAVAILABLE;
        pthread_cond_signal(school->entering_group_assigned_conds + tutor_id);
		pthread_mutex_unlock(school->entering_group_muts + tutor_id);
	}	
    
	pthread_mutex_lock(&school->tutors_can_go_home_mut);
    school->tutors_can_go_home = true;
    pthread_cond_broadcast(&school->tutors_can_go_home_cond);
	pthread_mutex_unlock(&school->tutors_can_go_home_mut);

    /* wait for all tutors to go */

    DEBUG_PRINT("Teacher thread is now waiting");

    pthread_mutex_lock(&school->tutors_gone_counter_mut);
    while (school->tutors_gone_counter < school->K)
        pthread_cond_wait(&school->tutors_gone_cond, 
                        &school->tutors_gone_counter_mut);
    pthread_mutex_unlock(&school->tutors_gone_counter_mut);
    
    mtsafe_printf(&school->print_mut, "Teacher: All students and tutors are "
                                      "left. I can now go home.\n");
    
    /* free resources */
    
    free(stud_left_to_fill_group); 
    
    return NULL;
}