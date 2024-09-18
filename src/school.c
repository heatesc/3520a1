#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

#include "../include/utils.h"
#include "../include/ring_buf.h"

#include "../include/student.h"
#include "../include/tutor.h"
#include "../include/teacher.h"
#include "../include/school.h"

extern school_t* school;

void school_run()
{
 	/* set student thread args */ 
  
	int* student_ids = malloc(sizeof(int) * school->N);
	for (int i = 0; i < school->N; i++)
		student_ids[i] = i;
        
	/* set tutor threads args */
    
	int* tutor_ids = malloc(sizeof(int) * school->K); 
	for (int i = 0; i < school->K; i++)
		tutor_ids[i] = i;
        
	/* launch student threads */

	pthread_t* student_threads = malloc(sizeof(pthread_t) * school->N);
	for (int i = 0; i < school->N; i++)
		pthread_create(student_threads + i, NULL, student, student_ids + i);
       	 
	/* launch tutor threads */
    
	pthread_t* tutor_threads = malloc(sizeof(pthread_t) * school->K);
	for (int i = 0; i < school->K; i++)
		pthread_create(tutor_threads + i, NULL, tutor, tutor_ids + i);
        
	/* launch teacher thread */
	
	pthread_t teacher_thread;
	pthread_create(&teacher_thread, NULL, teacher, NULL);	
        
	/* join threads */
        
	for (int i = 0; i < school->N; i++)
		pthread_join(student_threads[i], NULL);
	for (int i = 0; i < school->K; i++)
		pthread_join(tutor_threads[i], NULL);
	pthread_join(teacher_thread, NULL);
          
	/* free resources */
        
	free(tutor_threads);
	free(student_threads);
	free(student_ids);
	free(tutor_ids);
}

int school_init()
{
 	school_t* ret = malloc(sizeof(school_t)); 
        
	/* retrieve config from stdin */
    
    config* conf = config_get();
    if (NULL == conf)
    {
    	free(ret);
        return 1;
    }
    else if (conf_check(conf))
    {
      	free(ret);
		free(conf);
		return 1;
    }
    ret->N = conf->N;
    ret->M = conf->M;
    ret->K = conf->K;
    ret->T = conf->T;
    free(conf);
    
    /* initialise school variables */
   	 
	pthread_mutex_init(&ret->print_mut, NULL);
    
	ret->stud_group_assignments = malloc(sizeof(int) * ret->N);
	ret->stud_group_assignment_muts = malloc(sizeof(pthread_mutex_t) * ret->N);
	ret->stud_assigned_group_conds = malloc(sizeof(pthread_cond_t) * ret->N);
	for (int i = 0; i < ret->N; i++)
	{
		ret->stud_group_assignments[i] = GROUP_UNASSIGNED;
		pthread_mutex_init(ret->stud_group_assignment_muts + i, NULL);
		pthread_cond_init(ret->stud_assigned_group_conds + i, NULL);
	}
    
    ret->avail_lab_queue = ring_buf_init(ret->K);
    pthread_mutex_init(&ret->avail_lab_queue_mut, NULL);
   	pthread_cond_init(&ret->lab_is_avail_cond, NULL);
        
	ret->lab_attendance_counters = malloc(sizeof(int) * ret->K);
    for (int i = 0; i < ret->K; i++) ret->lab_attendance_counters[i] = 0;
   	ret->lab_attendance_counter_muts = malloc(sizeof(pthread_mutex_t) * 
                                              ret->K);
	for (int i = 0; i < ret->K; i++) 
          pthread_mutex_init(ret->lab_attendance_counter_muts + i, NULL);
	ret->all_students_present_conds = malloc(sizeof(pthread_cond_t) * 
                                                 ret->K);
	for (int i = 0; i < ret->K; i++)
          pthread_cond_init(ret->all_students_present_conds + i, NULL);
	
	ret->lab_assignments = malloc(sizeof(int) * ret->M);
	for (int i = 0; i < ret->M; i++) 
		ret->lab_assignments[i] = LAB_UNASSIGNED;
	ret->lab_assignment_muts = malloc(sizeof(pthread_mutex_t) * ret->M);
	for (int i = 0 ; i < ret->M; i++)
          pthread_mutex_init(ret->lab_assignment_muts + i, NULL);
	
	ret->students_gone_counter = 0;
	pthread_mutex_init(&ret->students_gone_counter_mut, NULL);
	pthread_cond_init(&ret->students_gone_cond, NULL);
	
	ret->group_sizes = malloc(sizeof(int) * ret->M);
        
	ret->lab_complete_flags = malloc(sizeof(bool) * ret->M);
	for (int i = 0; i < ret->M; i++)
		ret->lab_complete_flags[i] = false;
	ret->lab_complete_flag_muts = malloc(sizeof(pthread_mutex_t) * ret->M);
	for (int i = 0; i < ret->M; i++)
		pthread_mutex_init(ret->lab_complete_flag_muts + i, NULL);
	ret->lab_complete_conds = malloc(sizeof(pthread_cond_t) * ret->M);
	for (int i =0; i < ret->M; i++)
		pthread_cond_init(ret->lab_complete_conds + i, NULL);
	
	ret->tutors_can_go_home = false;
	pthread_mutex_init(&ret->tutors_can_go_home_mut, NULL);
	pthread_cond_init(&ret->tutors_can_go_home_cond, NULL);
	ret->tutors_gone_counter = 0;
	pthread_mutex_init(&ret->tutors_gone_counter_mut, NULL);
	pthread_cond_init(&ret->tutors_gone_cond, NULL);
	
	ret->student_counter = 0;
	pthread_mutex_init(&ret->student_counter_mut, NULL);
	pthread_cond_init(&ret->all_students_at_school_cond, NULL);
	
	ret->group_assigned_to_lab_conds = malloc(
		sizeof(pthread_cond_t) * ret->M);
	for (int i = 0; i < ret->M; i++)
		pthread_cond_init(ret->group_assigned_to_lab_conds + i, NULL);
	
	ret->entering_group = malloc(ret->K * sizeof(int));
	for (int i = 0; i < ret->K; i++) 
		ret->entering_group[i] = GROUP_UNASSIGNED;
	ret->entering_group_muts = malloc(ret->K * sizeof(pthread_mutex_t));
	ret->entering_group_assigned_conds = malloc(ret->K *  sizeof(pthread_cond_t));
	for (int i = 0; i < ret->K; i++)
	{
		pthread_mutex_init(ret->entering_group_muts + i, NULL); 
		pthread_cond_init(ret->entering_group_assigned_conds + i, NULL);
	} 
        
	ret->group_studs_gone_counter = malloc(sizeof(int) * ret->M);
	ret->group_studs_gone_counter_muts = malloc(
            sizeof(pthread_mutex_t) * ret->M);
	ret->group_gone_conds = malloc(sizeof(pthread_cond_t) * ret->M);
	for (int i = 0; i < ret->M; i++)
	{
		ret->group_studs_gone_counter[i] = 0;
		pthread_mutex_init(ret->group_studs_gone_counter_muts + i, NULL);
		pthread_cond_init(ret->group_gone_conds + i, NULL);
	}

	pthread_mutex_init(&ret->all_groups_assigned_flag_mut, NULL);
	ret->all_groups_assigned_flag = false;	
	pthread_cond_init(&ret->all_groups_assigned_cond, NULL);
        
	school = ret;
        
	return 0;
}

void school_destroy()
{
	pthread_mutex_destroy(&school->print_mut);
        
	for (int i = 0; i < school->N; i++)
	{
		pthread_mutex_destroy(school->stud_group_assignment_muts + i);
		pthread_cond_destroy(school->stud_assigned_group_conds + i);
	}	
	
	ring_buf_destroy(school->avail_lab_queue);		
	pthread_mutex_destroy(&school->avail_lab_queue_mut);	
	pthread_cond_destroy(&school->lab_is_avail_cond);

	free(school->lab_attendance_counters);
	for (int i = 0; i < school->K; i++)
	{
		pthread_mutex_destroy(school->lab_attendance_counter_muts + i);
		pthread_cond_destroy(school->all_students_present_conds + i);
	}
	free(school->lab_attendance_counter_muts);
	free(school->all_students_present_conds);

	free(school->lab_assignments);
	for (int i = 0; i < school->M; i++)
		pthread_mutex_destroy(school->lab_assignment_muts + i);
   	free(school->lab_assignment_muts);
        
	pthread_mutex_destroy(&school->students_gone_counter_mut);
	pthread_cond_destroy(&school->students_gone_cond);
        
	free(school->group_sizes);
	
	free(school->lab_complete_flags);
	for (int i = 0; i < school->M; i++)
    {
		pthread_mutex_destroy(school->lab_complete_flag_muts + i);
		pthread_cond_destroy(school->lab_complete_conds + i);
    }
	free(school->lab_complete_flag_muts);
	free(school->lab_complete_conds);
        
	pthread_cond_destroy(&school->tutors_can_go_home_cond);	
	pthread_mutex_destroy(&school->tutors_can_go_home_mut);
	pthread_mutex_destroy(&school->tutors_gone_counter_mut);
	pthread_cond_destroy(&school->tutors_gone_cond);

	pthread_mutex_destroy(&school->student_counter_mut);
	pthread_cond_destroy(&school->all_students_at_school_cond);
        
	for (int i = 0; i < school->M; i++)
		pthread_cond_destroy(school->group_assigned_to_lab_conds + i);
	free(school->group_assigned_to_lab_conds);
        
	free(school->entering_group);
	for (int i = 0; i < school->K; i++)
	{
          pthread_mutex_destroy(school->entering_group_muts + i);
          pthread_cond_destroy(school->entering_group_assigned_conds + i);
	}
	free(school->entering_group_muts);
	free(school->entering_group_assigned_conds);
	
	for (int i = 0; i < school->M; i++)
	{
		pthread_mutex_destroy(school->group_studs_gone_counter_muts + i);
		pthread_cond_destroy(school->group_gone_conds + i);
	}
	free(school->group_studs_gone_counter_muts);
	free(school->group_gone_conds);

	pthread_mutex_destroy(&school->all_groups_assigned_flag_mut);        
	pthread_cond_destroy(&school->all_groups_assigned_cond);	
        
	free(school);
}

