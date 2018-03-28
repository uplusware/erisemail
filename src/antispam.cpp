/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "antispam.h"

int popen2(const char *cmdline, struct popen2 *childinfo)
{
    pid_t p;
    int pipe_stdin[2], pipe_stdout[2];

    if(pipe(pipe_stdin))
        return -1;
    
    if(pipe(pipe_stdout))
        return -1;

    p = fork();
    
    if(p < 0)
        return p; /* Fork failed */
    
    if(p == 0)
    { /* child */
        close(pipe_stdin[1]);
        dup2(pipe_stdin[0], 0);
        close(pipe_stdout[0]);
        dup2(pipe_stdout[1], 1);
        execl("/bin/sh", "sh", "-c", cmdline, 0);
        perror("execl");
        exit(99);
    }
    
    childinfo->child_pid = p;
    childinfo->to_child = pipe_stdin[1];
    childinfo->from_child = pipe_stdout[0];
    
    return 0; 
}

void* mfilter_init(const char* param)
{
    //TODO:
	MailFilter * filter = new MailFilter;
	if(filter)
	{
		filter->is_spam = -1;
        filter->is_virs = -1;
        filter->param = param;
        filter->fd = NULL;
	}

	return (void*)filter;
}

void mfilter_emaildomain(void* filter, const char* domain, unsigned int len)
{
	//TODO:
}

void mfilter_clientip(void* filter, const char* ip, unsigned int len)
{
    //TODO:
}

void mfilter_clientdomain(void * filter, const char* domain, unsigned int len)
{
	//TODO:
}

void mfilter_mailfrom(void * filter, const char* from, unsigned int len)
{
	//TODO:
}

void mfilter_rcptto(void * filter, const char* to, unsigned int len)
{
	//TODO:
}

void mfilter_data(void * filter, const char* data, unsigned int len)
{
    MailFilter * tfilter = (MailFilter *)filter;
    if(!tfilter->fd)
    {
        tfilter->fd = new struct popen2;
        tfilter->fd->to_child = -1;
        tfilter->fd->from_child = -1;
        
        char result_buf[1024], command[4096];

        //checking via SpamAssassin
        snprintf(command, sizeof(command), "test spamc && spamc -c %s", tfilter ? (tfilter->param.c_str()) : "-t 5");
    
        if(popen2(command, tfilter->fd) == -1)
        {
            delete tfilter->fd;
            tfilter->fd = NULL;
        }
    }
    
    if(tfilter->fd && tfilter->fd->to_child != -1)
    {
        int total_len = 0;
        while(1)
        {
            int wroten_len = write(tfilter->fd->to_child, data, len - total_len);
            if(wroten_len < 0)
            {
                close(tfilter->fd->to_child);
                tfilter->fd->to_child = -1;
                
                delete tfilter->fd;
                tfilter->fd = NULL;
                break;
            }
            else
            {
                total_len += wroten_len;
                if(total_len == len)
                    break;
            }
                
        }
    }
}

void mfilter_eml(void * filter, const char* emlpath, unsigned int len)
{
    //TODO:
    
}

void mfilter_result(void * filter, int* is_spam)
{
    MailFilter * tfilter = (MailFilter *)filter;
    
    if(tfilter->fd->to_child != -1)
    {
        close(tfilter->fd->to_child);
        tfilter->fd->to_child = -1;
    }
    
    char result_buf[1025];
    result_buf[0] = '\0';
    
    if(tfilter->fd && tfilter->fd->from_child != -1)
    {
        int total_len = 0;
        while(1)
        {
            int read_len = read(tfilter->fd->from_child, result_buf, 1024 - total_len);
            if(read_len < 0)
            {
                close(tfilter->fd->from_child);
                tfilter->fd->from_child = -1;
                
                delete tfilter->fd;
                tfilter->fd = NULL;
                break;
            }
            else
            {
                total_len += read_len;
                result_buf[total_len] = '\0';
                
                if(memchr(result_buf, '\n', total_len) != NULL || memchr(result_buf, '\r', total_len) != NULL)
                    break;
                
                if(total_len == 1024)
                    break;
            }
                
        }
        
        close(tfilter->fd->from_child);
        tfilter->fd->from_child = -1;
        
        char* p = NULL;
        
        p = (char*)memchr(result_buf, '\n', total_len);
        if(p)
            *p = '\0';
        
        p = (char*)memchr(result_buf, '\r', total_len);
        if(p)
            *p = '\0';
    }

    double score = 0.0;
    double threshold = 0.0;
    
    if(sscanf(result_buf, "%lf/%lf", &score, &threshold) == 2 && threshold != 0.0 && score <= threshold)
    {
        tfilter->is_spam = 1;
    }
    
    if(tfilter)
        *is_spam = tfilter->is_spam;
    else
        *is_spam = -1;
}

void mfilter_exit(void * filter)
{
    //TODO:
    if(((MailFilter*)filter)->fd)
    {
        if(((MailFilter*)filter)->fd->to_child != -1)
            close(((MailFilter*)filter)->fd->to_child);
        
        if(((MailFilter*)filter)->fd->from_child != -1)
            close(((MailFilter*)filter)->fd->from_child);
        
        delete ((MailFilter*)filter)->fd;
    }
	if(filter)
        delete filter;
}

