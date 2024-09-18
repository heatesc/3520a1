#include "../include/school.h"

school_t* school = NULL;

int main()
{
    if (school_init()) return 1;
    school_run();
    school_destroy();

    printf("All students have completed their lab exercises. This is"
           " the end of the simulation.\n");
    
    return 0; 
}