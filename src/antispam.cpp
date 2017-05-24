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
		filter->isSpam = -1;
        filter->isVirs = -1;
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
    
    float scope = 0.0;
    float threshold = 0.0;
    char result_buf[4096], command[4096];

    int rc = 0;
    FILE *fp;
    
    //checking via SpamAssassin
    snprintf(command, sizeof(command), "test spamc && spamc -c %s < %s", tfilter ? tfilter->param : "-t 5", emlpath);

    fp = popen(command, "r");
    
    while(fp && fgets(result_buf, sizeof(result_buf), fp) != NULL)
    {
        if('\n' == result_buf[strlen(result_buf) - 1])
        {
            result_buf[strlen(result_buf) - 1] = '\0';
        }
        sscanf(result_buf, "%f/%f", &scope, &threshold);
        break;
    }
    if(fp)
        pclose(fp);
    
    if(threshold != 0.0 && scope <= threshold)
    {
        if(tfilter)
            tfilter->isSpam = 1;
    }
}

void mfilter_result(void * filter, int* isspam)
{
    MailFilter * tfilter = (MailFilter *)filter;
    if(tfilter)
        *isspam = tfilter->isSpam;
}

void mfilter_exit(void * filter)
{
    //TODO:
		
	delete filter;
}

