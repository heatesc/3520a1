#include "../include/utils.h"
#include "../include/tutor.h"
#include "../include/school.h"

#include <unistd.h>

extern school_t* school;

void* tutor(void* args)
{
    int tid = *(int*)args;

    int students_gone = num_students_left(&school->students_gone_counter_mut, 
                                          &school->students_gone_counter);

    bool tutor_required = tid < school->M;
    if (tutor_required)
    {
        while (students_gone < school->N)
        {
            /* reset lab session variables */

        	pthread_mutex_lock(school->entering_group_muts + tid);
        	school->entering_group[tid] = GROUP_UNASSIGNED;
        	pthread_mutex_unlock(school->entering_group_muts + tid);

        	pthread_mutex_lock(school->lab_attendance_counter_muts + tid);
        	school->lab_attendance_counters[tid] = 0;
        	pthread_mutex_unlock(school->lab_attendance_counter_muts + tid);
                
			/* wait for teacher to indicate that each student has been assigned
			 * a group */
                
			pthread_mutex_lock(&school->all_groups_assigned_flag_mut);
			while (!school->all_groups_assigned_flag)
			{
				pthread_cond_wait(&school->all_groups_assigned_cond,
					&school->all_groups_assigned_flag_mut);
			}
			pthread_mutex_unlock(&school->all_groups_assigned_flag_mut);
            
            /* join the available tutor/lab queue */

            pthread_mutex_lock(&school->avail_lab_queue_mut);
            mtsafe_printf(&school->print_mut, "Tutor %d: The lab room %d is " 
				"vacated and ready for one group\n", tid, tid);
            ring_buf_add(school->avail_lab_queue, tid);
            pthread_mutex_unlock(&school->avail_lab_queue_mut);
            
            /* signal to the teacher that the available lab queue isn't empty */

            pthread_cond_signal(&school->lab_is_avail_cond);

            /* wait to be assigned a group */
            
            pthread_mutex_lock(school->entering_group_muts + tid);
            while (GROUP_UNASSIGNED == school->entering_group[tid])
            {
                pthread_cond_wait(school->entering_group_assigned_conds + tid, 
                                  school->entering_group_muts + tid);  
                DEBUG_PRINT("tutor %d: assigned entering group %d\n", 
                            tid, school->entering_group[tid]);
            }
            bool no_group_avail = GROUP_UNAVAILABLE == 
                                  school->entering_group[tid];
            pthread_mutex_unlock(school->entering_group_muts + tid);
            
            /* if the group assigned is GROUP_UNAVAILABLE, then it means 
		     * there is no group left to be assigned, so the tutor should
			 * not re-enter the available labs queue */
            
            if (no_group_avail) break;
            
            /*  wait till the whole group has entered the lab */
            
            DEBUG_PRINT("tutor %d: I am aware that I have been assigned a group"
				"\n", tid);

            pthread_mutex_lock(school->lab_attendance_counter_muts + tid);
            while (school->lab_attendance_counters[tid] < 
                school->group_sizes[school->entering_group[tid]])
            {
                pthread_cond_wait(school->all_students_present_conds + tid,
                                  school->lab_attendance_counter_muts + tid);
            }
            pthread_mutex_unlock(school->lab_attendance_counter_muts + tid);
            
            mtsafe_printf(&school->print_mut, "Tutor %d: All students in group " 
				"%d have entered the lab room %d now.\n", 
				tid, school->entering_group[tid], tid);

            /* simulate lab time */
            
            int exercise_duration = get_rand_num(school->T/2, school->T);
            sleep(exercise_duration);
            
            mtsafe_printf(&school->print_mut, "Tutor %d: students in group %d "
				"have completed the lab exercise in %d units of time. You may "
				"leave this room now.\n",
				tid, school->entering_group[tid], exercise_duration);

            /* update lab complete flag and broadcast, so students know lab is 
			 * complete */
            
            pthread_mutex_lock(school->lab_complete_flag_muts + 
                               school->entering_group[tid]);
            school->lab_complete_flags[school->entering_group[tid]] = true;
			pthread_cond_broadcast(school->lab_complete_conds + 
                                   school->entering_group[tid]);
            pthread_mutex_unlock(school->lab_complete_flag_muts + 
									school->entering_group[tid]);
            
        	/* update the students gone counter */

        	pthread_mutex_lock(&school->students_gone_counter_mut);
        	school->students_gone_counter += school->group_sizes[school->entering_group[tid]];
        	pthread_mutex_unlock(&school->students_gone_counter_mut);
			
			/* wait for all the students of the current group to leave the lab*/
                
			pthread_mutex_lock(school->group_studs_gone_counter_muts + 
                                   school->entering_group[tid]);
			while (school->group_studs_gone_counter[school->entering_group[tid]]
				< school->group_sizes[school->entering_group[tid]])
			{
				pthread_cond_wait(school->group_gone_conds + 
					school->entering_group[tid],
					school->group_studs_gone_counter_muts + 
					school->entering_group[tid]);
			}
			pthread_mutex_unlock(school->group_studs_gone_counter_muts + 
												   school->entering_group[tid]);                 
                        
        	/* inform the teacher if all the students have had their labs */
    
        	if ((school->students_gone_counter >= school->N))
        		pthread_cond_signal(&school->students_gone_cond);
    
            students_gone = num_students_left(&school->students_gone_counter_mut
				, &school->students_gone_counter);
        } 
    }

    /* wait for teacher to allow going home */

    pthread_mutex_lock(&school->tutors_can_go_home_mut);
    while (!school->tutors_can_go_home)
        pthread_cond_wait(&school->tutors_can_go_home_cond, 
                        &school->tutors_can_go_home_mut);
    pthread_mutex_unlock(&school->tutors_can_go_home_mut);

    
    /* update tutors gone counter, signal teacher if all tutors gone */

    bool signal_teacher = false;
    pthread_mutex_lock(&school->tutors_gone_counter_mut);
    school->tutors_gone_counter++;
    signal_teacher = school->tutors_gone_counter == school->K; 
    pthread_mutex_unlock(&school->tutors_gone_counter_mut);
    
    mtsafe_printf(&school->print_mut, "Tutor %d: Thanks teacher. Bye!\n", tid);

    if (signal_teacher) pthread_cond_signal(&school->tutors_gone_cond);
    
    return NULL;
}