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

int main()
{
    // retrieve config and initialise school variables
    school_t* school = school_init();
    
    // run the school: launch and join the school threads
    school_run(school);
    
    // free school resources
    school_destroy(school);
    
    return 0; 
    
    /* misc synchronisation variables */

    // only the thread that locks the print_mut may print (excluding the main thread)
    pthread_mutex_t print_mut;
    pthread_cond_t* lab_complete_conds = malloc(sizeof(pthread_cond_t) * K);
    for (int i = 0; i < K; i++)
        pthread_cond_init(lab_complete_conds + i, NULL);
    
    pthread_mutex_init(&print_mut, NULL);
    pthread_mutex_t avail_lab_queue_mut;
    pthread_mutex_init(&avail_lab_queue_mut, NULL);
    pthread_mutex_t* lab_attendance_counter_muts = malloc(
            sizeof(pthread_mutex_t) * K);
    for (int i = 0; i < K; i++) 
        pthread_mutex_init(lab_attendance_counter_muts + i, NULL);
    pthread_cond_t* all_students_present_conds = malloc(sizeof(pthread_cond_t) * K);
    for (int i = 0; i < K; i++)
        pthread_cond_init(all_students_present_conds + i, NULL);
    int* lab_assignments = malloc(sizeof(int) * M);
    for (int i = 0; i < M; i++) lab_assignments[i] = LAB_UNASSIGNED;
    int students_gone_counter = 0;
    pthread_cond_t students_gone_cond;
    pthread_cond_init(&students_gone_cond, NULL);
    ring_buf* lab_avail_queue = ring_buf_init(K);
    pthread_cond_t lab_is_avail_cond;
    pthread_cond_init(&lab_is_avail_cond, NULL);
    pthread_mutex_t lab_assignments_mut;
    pthread_mutex_init(&lab_assignments_mut, NULL);
    pthread_mutex_t students_gone_counter_mut;
    pthread_mutex_init(&students_gone_counter_mut, NULL);
    int* group_sizes = malloc(sizeof(int) * M);
    // pthread_cond_t tutor_done_cond;
    // pthread_cond_init(&tutor_done_cond, NULL);
    pthread_mutex_t* lab_complete_flag_muts = malloc(sizeof(pthread_mutex_t) * K);
    for (int i = 0; i < K; i++)
        pthread_mutex_init(&lab_complete_flag_muts[i], NULL);
    bool* lab_complete_flags = calloc(K, sizeof(bool));
    bool tutors_can_go_home = false;
    pthread_mutex_t tutors_can_go_home_mut;
    pthread_mutex_init(&tutors_can_go_home_mut, NULL);
    pthread_cond_t tutors_can_go_home_cond;
    pthread_cond_init(&tutors_can_go_home_cond, NULL);
    int tutors_gone_counter = 0;
    pthread_mutex_t tutors_gone_counter_mut;
    pthread_mutex_init(&tutors_gone_counter_mut, NULL);
    pthread_cond_t tutors_gone_cond;
    pthread_cond_init(&tutors_gone_cond, NULL);
    pthread_cond_t all_students_present; // todo: disambiguate with all_students_arrived
    pthread_cond_init(&all_students_present, NULL);
    pthread_mutex_t* lab_assignment_muts = malloc(sizeof(pthread_mutex_t) * M);
    for (int i = 0; i < M; i++) pthread_mutex_init(lab_assignment_muts + i, NULL);
    int* lab_attendance_counters = malloc(sizeof(int) * K);
    for (int i = 0; i < K; i++) lab_attendance_counters[i] = 0;
    
    /* create student thread args */
    
    // entry i stores the condition variable associated with the condition:
    // the teacher has allocated group i to a lab.
    pthread_cond_t* group_assigned_to_lab_conds = malloc(
            sizeof(pthread_cond_t) * M);
    // counts the number of students that have arrived in the classroom
    int student_counter = 0;
    pthread_mutex_t student_counter_mut;
    pthread_mutex_init(&student_counter_mut, NULL);
    pthread_cond_t all_students_arrived;
    pthread_cond_init(&all_students_arrived, NULL);
    student_args* student_args_arr = malloc(sizeof(student_args) * N);
    for (int i = 0; i < N; i++)
    {
        student_args_arr[i].student_id = i;
        student_args_arr[i].assigned_group = GROUP_UNASSIGNED;
        student_args_arr[i].N = N;
        student_args_arr[i].student_counter = &student_counter;
        student_args_arr[i].student_counter_mut = &student_counter_mut;
        student_args_arr[i].all_students_arrived = &all_students_arrived;
        student_args_arr[i].all_students_present_muts = lab_attendance_counter_muts;
        student_args_arr[i].all_students_present_conds = all_students_present_conds;
        student_args_arr[i].lab_assignments = lab_assignments;
        student_args_arr[i].students_gone_counter = &students_gone_counter;
        student_args_arr[i].students_gone_cond = &students_gone_cond;
        student_args_arr[i].print_mut = &print_mut;
        student_args_arr[i].lab_complete_flags = lab_complete_flags;
        student_args_arr[i].lab_complete_flag_muts = lab_complete_flag_muts;
        student_args_arr[i].lab_complete_conds = lab_complete_conds;
        student_args_arr[i].group_sizes = group_sizes;
        student_args_arr[i].group_assigned_to_lab_conds = group_assigned_to_lab_conds;
        student_args_arr[i].lab_assignment_muts = lab_assignment_muts;
        student_args_arr[i].lab_attendance_counters = lab_attendance_counters;
        student_args_arr[i].lab_attendance_counter_muts = lab_attendance_counter_muts;
    }
    
    /* create tutor thread args */
    
    tutor_args* tut_args = malloc(sizeof(tutor_args) * K);
    for (int i = 0; i < K; i++)
    {
        tut_args[i].entering_group = GROUP_UNASSIGNED;
        tut_args[i].id = i;
        pthread_mutex_init(&tut_args->entering_group_mut, NULL);
        tut_args[i].lab_attendance_counter_muts = lab_attendance_counter_muts;
        tut_args[i].avail_lab_queue_mut = &avail_lab_queue_mut;
        tut_args[i].avail_lab_queue = lab_avail_queue;
        pthread_cond_init(&tut_args->entering_group_assigned, NULL);
        tut_args[i].T = T;
        tut_args[i].running = &running;
        tut_args[i].avail_lab_queue_not_empty = &lab_is_avail_cond;
        tut_args[i].group_sizes = group_sizes;
        tut_args[i].N = N;
        tut_args[i].students_gone_counter = &students_gone_counter;
        tut_args[i].students_gone_counter_mut = &students_gone_counter_mut;
        tut_args[i].print_mut = &print_mut;
        tut_args[i].lab_complete_flag_muts = lab_complete_flag_muts;
        tut_args[i].lab_complete_flags = lab_complete_flags;
        tut_args[i].lab_complete_conds = lab_complete_conds;
        tut_args[i].tutors_can_go_home = &tutors_can_go_home;
        tut_args[i].tutors_can_go_home_mut = &tutors_can_go_home_mut;
        tut_args[i].tutors_can_go_home_cond = &tutors_can_go_home_cond;
        tut_args[i].tutors_gone_counter = &tutors_gone_counter;
        tut_args[i].tutors_gone_counter_mut = &tutors_gone_counter_mut;
        tut_args[i].tutors_gone_cond = &tutors_gone_cond;
        tut_args[i].K = K;
        tut_args[i].all_students_present = &all_students_present;
        tut_args[i].all_students_present_conds = all_students_present_conds;
        tut_args[i].lab_attendance_counters = lab_attendance_counters;
        tut_args[i].M = M;
        // tut_args[i].early_exit_flags = early_exit_flags;
        // tut_args[i].early_exit_conds = early_exit_conds;
        // tut_args[i].early_exit_flag_muts = early_exit_flag_muts;
    }

    /* create teacher thread args and launch teacher thread */

    teacher_args* targs = malloc(sizeof(teacher_args));
    targs->student_counter_mut = &student_counter_mut;
    targs->student_counter = &student_counter;
    targs->student_args_arr = student_args_arr;
    targs->N = N;
    targs->M = M;
    targs->K = K;
    targs->avail_lab_queue = lab_avail_queue;
    targs->avail_lab_queue_mut = &avail_lab_queue_mut;
    targs->all_students_arrived = &all_students_arrived;
    pthread_t teacher_thread;
    pthread_create(&teacher_thread, NULL, teacher, targs);
    targs->lab_is_avail_cond = &lab_is_avail_cond;
    targs->tutor_args_arr = tut_args;
    targs->lab_assignments = lab_assignments;
    targs->lab_assignments_mut = &lab_assignments_mut;
    targs->group_assigned_to_lab_conds = group_assigned_to_lab_conds;
    targs->students_gone_counter_mut = &students_gone_counter_mut;
    targs->students_gone_counter = &students_gone_counter;
    targs->group_sizes = group_sizes;
    targs->students_gone_cond = &students_gone_cond;
    targs->tutor_done_cond = &tutor_done_cond;
    targs->print_mut = &print_mut;
    targs->tutors_can_go_home = &tutors_can_go_home;
    targs->tutors_can_go_home_mut = &tutors_can_go_home_mut;
    targs->tutors_can_go_home_cond = &tutors_can_go_home_cond;
    targs->tutors_gone_counter = &tutors_gone_counter;
    targs->tutors_gone_counter_mut = &tutors_gone_counter_mut;
    targs->tutors_gone_cond = &tutors_gone_cond;
    targs->lab_attendance_counters = lab_attendance_counters;
    targs->lab_assignment_muts = lab_assignment_muts;
    
    /* launch student threads */
    
    pthread_t* student_threads = malloc(sizeof(pthread_t) * N);
    for (int i = 0; i < N; i++)
        pthread_create(student_threads + i, NULL, student, student_args_arr +i);
    
    /* launch tutor threads */

    pthread_t* tutor_threads = malloc(sizeof(pthread_t) * K) ;
    for (int i = 0; i < K; i++)
        pthread_create(tutor_threads + i, NULL, tutor, tut_args + i);
    
    /* cancel and join threads */
    
    for (int i = 0; i < N; i++)
        pthread_join(student_threads[i], NULL);
    for (int i = 0; i < K; i++)
        pthread_join(tutor_threads[i], NULL);
    pthread_join(teacher_thread, NULL);
    
    /* free resources */
    
    free(student_args_arr);
    free(student_threads);
    free(tut_args);
    free(targs);
    pthread_mutex_destroy(&student_counter_mut);
    pthread_cond_destroy(&all_students_arrived);
    for (int i = 0; i < M; i++) 
        pthread_mutex_destroy(lab_attendance_counter_muts + i);
    free(lab_attendance_counter_muts);
    free(all_students_present_conds);
    free(tutor_threads);
    for (int i = 0; i < K; i++)
        pthread_cond_destroy(lab_complete_conds + i);
    free(lab_complete_conds);
    free(lab_complete_flags);
    for (int i = 0; i < K; i++)
        pthread_mutex_destroy(lab_assignment_muts + i);
    free(lab_assignment_muts);
    
    free(lab_attendance_counters);
    free(early_exit_flags);

    return 0;
}