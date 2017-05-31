/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include <stdio.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include "posixname.h"
#include "postnotify.h"

int mta_post_notify(bool print)
{
    int result = -1;
    sem_t* post_sid = sem_open(ERISEMAIL_POST_NOTIFY, O_RDWR);
    if(post_sid != SEM_FAILED && sem_post(post_sid) == 0)
    {
       result = 0;
    }
    
    if(print)
        perror("Notify the local MTA");
    
    if(post_sid != SEM_FAILED)
    {
        sem_close(post_sid);
    }
    
    return result;
}