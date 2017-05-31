/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include "postudf.h"
#include "posixname.h"

my_bool post_notify_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    initid->max_length = 8;
    return 0;
}

void post_notify_deinit(UDF_INIT *initid)
{
    
}

char * post_notify(UDF_INIT *initid, 
               UDF_ARGS *args,
               char *result,
               unsigned long *length,
               char *is_null,
               char *error)
{
    *is_null = 0; //not NULL returned
    *error = 0; //no error happens
    
    sem_t* post_sid = sem_open(ERISEMAIL_POST_NOTIFY, O_RDWR);
    if(post_sid != SEM_FAILED && sem_post(post_sid) == 0)
    {
        strcpy(result, "Yes");
        *length = 4;
    }
    else
    {
        strcpy(result, "No");
        *length = 3;
    }
    if(post_sid != SEM_FAILED)
    {
        sem_close(post_sid);
    }
    return result;
}
