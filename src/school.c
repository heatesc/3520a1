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

school_t* school_init()
{
 	school_t* ret = malloc(sizeof(school_t)); 
        
	/* retrieve config from stdin */
    
    ret->conf = config_get();
    if (NULL == ret->conf)
    {
    	free(ret);
        return NULL;
    }
    else if (conf_check(ret->conf))
    {
		free(ret->conf);
		free(ret);
		return NULL;
    }
    
    /* initialise school variables */
   	 
	pthread_mutex_init(&ret->print_mut, NULL);
    
    ret->avail_lab_queue = ring_buf_init(ret->conf->K);
    pthread_mutex_init(&ret->avail_lab_queue_mut, NULL);
   	pthread_cond_init(&ret->lab_is_avail_cond, NULL);
        
	ret->lab_attendance_counters = malloc(sizeof(int) * ret->conf->K);
    for (int i = 0; i < ret->conf->K; i++) ret->lab_attendance_counters[i] = 0;
   	ret->lab_attendance_counter_muts = malloc(sizeof(pthread_mutex_t) * 
                                              ret->conf->K);
	for (int i = 0; i < ret->conf->K; i++) 
          pthread_mutex_init(ret->lab_attendance_counter_muts + i, NULL);
	ret->all_students_present_conds = malloc(sizeof(pthread_cond_t) * 
                                                 ret->conf->K);
	for (int i = 0; i < ret->conf->K; i++)
          pthread_cond_init(ret->all_students_present_conds + i, NULL);
	
	ret->lab_assignments = malloc(sizeof(int) * ret->conf->M);
	for (int i = 0; i < ret->conf->M; i++) 
		ret->lab_assignments[i] = LAB_UNASSIGNED;
	ret->lab_assignment_muts = malloc(sizeof(pthread_mutex_t) * ret->conf->M);
	for (int i = 0 ; i < ret->conf->M; i++)
          pthread_mutex_init(ret->lab_assignment_muts + i, NULL);
	
	ret->students_gone_counter = 0;
	pthread_mutex_init(&ret->students_gone_counter_mut, NULL);
	pthread_cond_init(&ret->students_gone_cond, NULL);
	
	// todo: justify the lack of locks associated with this shared variable
	ret->group_sizes = malloc(sizeof(int) * ret->conf->M);
        
	ret->lab_complete_flags = malloc(sizeof(bool) * ret->conf->K);
	for (int i = 0; i < ret->conf->K; i++)
		ret->lab_complete_flags[i] = false;
	ret->lab_complete_flag_muts = malloc(sizeof(pthread_mutex_t) * ret->conf->K);
	for (int i = 0; i < ret->conf->K; i++)
		pthread_mutex_init(ret->lab_complete_flag_muts + i, NULL);
	ret->lab_complete_conds = malloc(sizeof(pthread_cond_t) * ret->conf->K);
	for (int i =0; i < ret->conf->K; i++)
		pthread_cond_init(ret->lab_complete_conds + i, NULL);
	
	ret->tutors_can_go_home = false;
	pthread_cond_init(&ret->tutors_can_go_home_cond, NULL);
	ret->tutors_gone_counter = 0;
	pthread_mutex_init(&ret->tutors_gone_counter_mut, NULL);
	pthread_cond_init(&ret->tutors_gone_cond, NULL);
	
	ret->student_counter = 0;
	pthread_mutex_init(&ret->student_counter_mut, NULL);
	pthread_cond_init(&ret->all_students_at_school_cond, NULL);
	
	ret->group_assigned_to_lab_conds = malloc(
            sizeof(pthread_cond_t) * ret->conf->M);
	for (int i = 0; i < ret->conf->M; i++)
		pthread_cond_init(ret->group_assigned_to_lab_conds + i, NULL);
	
	ret->entering_group = malloc(ret->conf->K * sizeof(int));
	for (int i = 0; i < ret->conf->K; i++) 
		ret->entering_group[i] = GROUP_UNASSIGNED;
}

void school_destroy(school_t* school)
{
	pthread_mutex_destroy(&school->print_mut);  
	
	pthread_mutex_destroy(&school->
}
    
void school_run(school_t* school)
{
  /* set students args */
        
	/* set tutors args */
        
    /* set teacher args */
  
	/* launch students */
        
	/* launch tutors */
        
	/* launch teacher */
        
	/* join threads */
}
