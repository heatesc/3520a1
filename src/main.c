#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

#include "../include/utils.h"
#include "../include/ring_buf.h"

#define GROUP_UNASSIGNED (-1)
#define LAB_UNASSIGNED (-1)

// FILE* TEST_LOG_FILE_STREAM = fopen(TEST_MODE_LOG_FILE, "w");
//
// // void test_log(int x)
// {
//     fprintf(TEST_LOG_FILE_STREAM, "%d", x);
// }

typedef struct
{
    int student_id;
    int assigned_lab;
    pthread_mutex_t* print_mut;
    int assigned_group;
    int* group_sizes;
    pthread_mutex_t* student_counter_mut;
    int* student_counter;
    pthread_mutex_t assigned_group_mut;
    pthread_cond_t assigned_group_cond;
//    pthread_mutex_t assigned_lab_mut;
//    pthread_cond_t assigned_lab_cond;
    // this is used to signal to the teacher
    // that all students have arrived.
    pthread_cond_t* all_students_arrived;
    int N;
    pthread_cond_t* group_assigned_to_lab_conds;
    // entry i of 'lab_assignments' is the lab_id of the lab assigned to 
    // group i. entries are initialised to LAB_UNASSIGNED.
    int* lab_assignments;
    pthread_mutex_t* lab_assignments_mut;
    pthread_mutex_t* lab_assignment_muts;
    bool* running;
    int* lab_attendance_counter;
    pthread_mutex_t* lab_attendance_counter_mut;
    // represents condiiton: all studnets in group i present in their 
    // allocated lab
    pthread_cond_t* all_students_present_conds;
    pthread_mutex_t* all_students_present_muts;
    int* group_capacities;
    int* students_gone_counter;
    pthread_mutex_t students_gone_counter_mut;
    pthread_cond_t* students_gone_cond;
    pthread_cond_t* lab_complete_conds;
    bool* lab_complete_flags;
    pthread_mutex_t* lab_complete_flag_muts;
    int* lab_attendance_counters;
    pthread_mutex_t* lab_attendance_counter_muts;
} student_args;

typedef struct
{
    int id;
    int* group_sizes;
    pthread_mutex_t* avail_lab_queue_mut;
    ring_buf* avail_lab_queue;
    pthread_mutex_t* print_mut;
    int entering_group;
    pthread_mutex_t entering_group_mut;
    pthread_cond_t entering_group_assigned;
    pthread_cond_t* avail_lab_queue_not_empty;
    bool* running;
    pthread_cond_t* group_assigned_to_lab_cond;
    // entry i of 'lab_assignments' is the lab_id of the lab assigned to 
    // group i. entries are initialised to LAB_UNASSIGNED.
    int* lab_assignments;
    pthread_mutex_t* lab_assignments_mut;
    pthread_cond_t* all_students_present;
    int lab_attendance_counter;
    int lab_attendance_counter_mut;
    int T;
    int* group_capacities;
    int* students_gone_counter;
    pthread_mutex_t* students_gone_counter_mut;
    bool* tutors_can_go_home;
    pthread_mutex_t* tutors_can_go_home_mut;
    pthread_cond_t* tutors_can_go_home_cond;
    int N;
    pthread_cond_t* lab_complete_conds;
    bool* lab_complete_flags;
    pthread_mutex_t* lab_complete_flag_muts;
    int* tutors_gone_counter;
    pthread_mutex_t* tutors_gone_counter_mut;
    pthread_cond_t* tutors_gone_cond;
    pthread_mutex_t* lab_assignment_muts;
    int K;
    pthread_cond_t* all_students_present_conds;
    int* lab_attendance_counters;
    pthread_mutex_t* lab_attendance_counter_muts;
    int M;
    bool* early_exit_flags;
    pthread_mutex_t* early_exit_flag_muts;
    pthread_cond_t* early_exit_conds;
} tutor_args;

typedef struct
{
    int* group_sizes;
    pthread_mutex_t* print_mut;
    pthread_cond_t* lab_is_avail_cond;
    pthread_cond_t* all_students_arrived;
    pthread_mutex_t* student_counter_mut;
    student_args* student_args_arr;
    ring_buf* avail_lab_queue;
    pthread_mutex_t* avail_lab_queue_mut;
    tutor_args* tutor_args_arr;
    int* student_counter;
    int N;
    int M;
    bool* running;
    pthread_mutex_t* running_mut;
    pthread_cond_t* group_assigned_to_lab_conds;
    // entry i of 'lab_assignments' is the lab_id of the lab assigned to 
    // group i. entries are initialised to LAB_UNASSIGNED.
    int* lab_assignments;
    pthread_mutex_t* lab_assignments_mut;
    pthread_cond_t* students_gone_cond;
    pthread_mutex_t* students_gone_counter_mut;
    int* students_gone_counter;
    int K;
    pthread_cond_t* tutor_done_cond;
    bool* tutors_can_go_home;
    pthread_mutex_t* tutors_can_go_home_mut;
    pthread_cond_t* tutors_can_go_home_cond;
    int* tutors_gone_counter;
    pthread_mutex_t* tutors_gone_counter_mut;
    pthread_cond_t* tutors_gone_cond;
    pthread_mutex_t* lab_assignment_muts;
    int* lab_attendance_counters;
    pthread_mutex_t* lab_attendance_counter_muts;
    bool* early_exit_flags;
    pthread_cond_t* early_exit_conds;
    pthread_mutex_t* early_exit_flag_muts;
} teacher_args;

static int num_students_left(pthread_mutex_t* mut, int* source)
{
    pthread_mutex_lock(mut);
    const int ret = *source;
    pthread_mutex_unlock(mut);
    return ret;
}

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

            
            // /* determine whether this tutor is required, if not suspend till exit condition is signalled by teacher */
        
            // if (tut_args->id >= tut_args->M)
            // {
            //     pthread_mutex_lock(tut_args->early_exit_flag_muts + tut_args->id);
            //     while (false == tut_args->early_exit_flags[tut_args->id])
            //         pthread_cond_wait(tut_args->early_exit_conds + tut_args->id, 
            //                           tut_args->early_exit_flag_muts + tut_args->id);
            //     pthread_mutex_unlock(tut_args->early_exit_flag_muts + tut_args->id);
            //     bool signal_teacher = false;
            //     pthread_mutex_lock(tut_args->tutors_gone_counter_mut);
            //     (*tut_args->tutors_gone_counter)++;
            //     signal_teacher = *tut_args->tutors_gone_counter == tut_args->K; 
            //     pthread_mutex_unlock(tut_args->tutors_gone_counter_mut);
            //     mtsafe_printf(tut_args->print_mut, "Tutor %d: Thanks teacher. Bye!\n", tut_args->id);
            //     if (signal_teacher) pthread_cond_signal(tut_args->tutors_gone_cond);
            //     return NULL;
            // }
            
            /* wait to be assigned a group */
            
            pthread_mutex_lock(&tut_args->entering_group_mut);
            while (GROUP_UNASSIGNED == tut_args->entering_group)
                pthread_cond_wait(&tut_args->entering_group_assigned, 
                                  &tut_args->entering_group_mut);
    //        int assigned_group = tut_args->entering_group;
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
        
        /* Dismiss tutors that aren't currently needed.
         * */
        
        // if (avail_lab >= targs->M)
        // {
        //     // This isn't the most elegant, but the purpose here is simple: no group-lab assignment occurs
        //     // in this iteration, so the group has to be decremented.
        //     --group;
        //     DEBUG_PRINT("teacher: tutor %d may go home early\n", avail_lab);
        //     pthread_mutex_lock(targs->early_exit_flag_muts + avail_lab);
        //     targs->early_exit_flags[avail_lab] = true; // indicates the tutor may go home
        //     pthread_mutex_unlock(targs->early_exit_flag_muts + avail_lab);
        //     mtsafe_printf(targs->print_mut, "Teacher: there are no students waiting. Tutor %d, you can"
        // " go home now.\n", avail_lab); 
        //     pthread_cond_signal(targs->early_exit_conds + avail_lab);
        // }
        
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

//    DEBUG_PRINT("line %d reached\n", __LINE__);
//    DEBUG_PRINT("stu %d: LOCKING l%d; stu: assigned to group %d\n", sargs->student_id, __LINE__, sargs->assigned_group);
    pthread_mutex_lock(&sargs->lab_assignment_muts[sargs->assigned_group]);
//    DEBUG_PRINT("stu %d: line %d reached\n", sargs->student_id, __LINE__);
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
//    DEBUG_PRINT("stu %d: UNLOCKING l%d; stu: assigned to group %d\n", sargs->student_id, __LINE__, sargs->assigned_group);
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

// later i should refactor the code into a big classroom object sort of thing
// this will make the code much more readable.
/*
typedef struct classroom
{
   int* labs;
   int lab_ct;
   
};
*/

// return 0 only if input config is valid
int conf_check(int N, int M, int K, int T)
{
    // each variable must be positive
    if (N < 0 || M < 0 || K < 0 || T < 0) return 1; 
    
    // there cannot be more groups than students
    if (M > N) return 2;
    
    return 0;
}

int main()
{
    /* retrieve config info */
    
    int N; // number of students
    int M; // number of groups
    int K; // number of tutors, lab groups
    int T; // time limit for each group of students to do the lab exercise
    printf("Enter number of students, N: ");
    if (get_int_from_stdin(&N)) return 1;
    printf("Enter number of groups, M: ");
    if (get_int_from_stdin(&M)) return 1;
    printf("Enter number of tutors, lab groups: ");
    if (get_int_from_stdin(&K)) return 1;
    printf("Enter lab exercise time limit: ");
    if (get_int_from_stdin(&T)) return 1;
    DEBUG_PRINT("Config retrieved. N=%d, M=%d, K=%d, T=%d\n", N, M, K, T);
    printf("Config retrieved. N (student ct) =%d, M (group ct) =%d, K (lab i.e. tutor ct) =%d, (time bw T/2 and T) T=%d\n", N, M, K, T);
    
    if (conf_check(N, M, K, T)) return 1;

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
    bool running = true;
    int lab_attendance_counter = 0;
    pthread_mutex_t lab_attendance_counter_mut;
    pthread_mutex_init(&lab_attendance_counter_mut, NULL);
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
    pthread_mutex_t running_mut;
    pthread_mutex_init(&running_mut, NULL);
    int* group_sizes = malloc(sizeof(int) * M);
    pthread_cond_t tutor_done_cond;
    pthread_cond_init(&tutor_done_cond, NULL);
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
    bool* early_exit_flags = malloc(sizeof(bool) * K);
    for (int i = 0; i < K; i++) early_exit_flags[i] = false;
    pthread_mutex_t* early_exit_flag_muts = malloc(sizeof(pthread_mutex_t) * K);
    for (int i = 0; i < K; i++) pthread_mutex_init(early_exit_flag_muts + i, NULL);
    pthread_cond_t* early_exit_conds = malloc(sizeof(pthread_cond_t) * K);
    for (int i = 0; i < K; i++) pthread_cond_init(early_exit_conds, NULL);
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
//        pthread_mutex_init(&student_args_arr[i].assigned_lab_mut, NULL);
        student_args_arr[i].assigned_group = GROUP_UNASSIGNED;
        student_args_arr[i].assigned_lab = LAB_UNASSIGNED;
        student_args_arr[i].N = N;
        student_args_arr[i].student_counter = &student_counter;
        student_args_arr[i].student_counter_mut = &student_counter_mut;
        student_args_arr[i].all_students_arrived = &all_students_arrived;
        student_args_arr[i].lab_attendance_counter_mut = &lab_attendance_counter_mut;
        student_args_arr[i].lab_attendance_counter = &lab_attendance_counter;
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
        tut_args[i].early_exit_flags = early_exit_flags;
        tut_args[i].early_exit_conds = early_exit_conds;
        tut_args[i].early_exit_flag_muts = early_exit_flag_muts;
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
    targs->running = &running;
    targs->running_mut = &running_mut;
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
    
//    for (int i = 0; i < N; i++)
//    {
//        pthread_mutex_destroy(&student_args_arr[i].assigned_lab_mut);
//        pthread_cond_destroy(&student_args_arr[i].assigned_lab_cond);
//    }
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

    // fclose(TEST_LOG_FILE_STREAM);
    return 0;
}