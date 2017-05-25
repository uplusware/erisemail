/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include "antispam.h"

void* mfilter_init(const char* param)
{
    //TODO:
	MailFilter * filter = new MailFilter;
	if(filter)
	{
		filter->is_spam = -1;
        filter->is_virs = -1;
        filter->param = param;
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
	//TODO:
}

void mfilter_eml(void * filter, const char* emlpath, unsigned int len)
{
    MailFilter * tfilter = (MailFilter *)filter;
    
    float score = 0.0;
    float threshold = 0.0;
    char result_buf[1024], command[4096];

    //checking via SpamAssassin
    snprintf(command, sizeof(command), "test spamc && spamc -c %s < %s", tfilter ? (tfilter->param.c_str()) : "-t 5", emlpath);

    FILE * fp = popen(command, "r");
    
    while(fp && fgets(result_buf, sizeof(result_buf), fp) != NULL)
    {
        if('\n' == result_buf[strlen(result_buf) - 1])
        {
            result_buf[strlen(result_buf) - 1] = '\0';
        }
        sscanf(result_buf, "%f/%f", &score, &threshold);
        break;
    }
    printf("%s / %s [%f %f]\n", command, result_buf, score, threshold);
    
    if(fp)
        pclose(fp);
    
    if(tfilter && threshold != 0.0 && score <= threshold)
    {
        tfilter->is_spam = 1;
    }
}

void mfilter_result(void * filter, int* is_spam)
{
    MailFilter * tfilter = (MailFilter *)filter;
    if(tfilter)
        *is_spam = tfilter->is_spam;
    else
        *is_spam = -1;
}

void mfilter_exit(void * filter)
{
    //TODO:
	if(filter)
        delete filter;
}

