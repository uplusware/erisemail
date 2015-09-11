#include <stdio.h>
#include "antijunk.h"

void* mfilter_init()
{
	//printf("mfilter_init\n");
	
	MailFilter * filter = new MailFilter;
	if(filter)
	{
		filter->isJunk = -1;
	}

	tm * ltm;
	time_t tt = time(NULL);
	ltm = localtime(&tt);
	char szFileName[1024];
	sprintf(szFileName, "/var/log/erisemail/MTA-%04d-%02d-%02d.log", 1900 + ltm->tm_year, ltm->tm_mon + 1, ltm->tm_mday);
	
	
	filter->semLog = sem_open("/.erisemail-mta-log.sem", O_CREAT | O_RDWR, 0644, 1);		
	filter->fLog = fopen(szFileName, "ab+");
	
	return (void*)filter;
}

void mfilter_emaildomain(void* filter, const char* domain, unsigned int len)
{
	//printf("mfilter_emaildomain: %s\n", domain);
}

void mfilter_clientip(void* filter, const char* ip, unsigned int len)
{
	MailFilter * tfilter = (MailFilter *)filter;

	if(tfilter->semLog != SEM_FAILED && tfilter->fLog)
	{
		sem_wait(tfilter->semLog);

		tm * ltm;
		time_t tt = time(NULL);
		ltm = localtime(&tt);
		fprintf(tfilter->fLog, "[%04d-%02d-%02d %02d:%02d:%02d]: %s\r\n", 1900 + ltm->tm_year, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec, ip);
		fflush(tfilter->fLog);
		sem_post(tfilter->semLog);

		//printf("[%04d-%02d-%02d %02d:%02d:%02d]: %s\r\n", 1900 + ltm->tm_year, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec, ip);
	}
}

void mfilter_clientdomain(void * filter, const char* domain, unsigned int len)
{
	//printf("mfilter_clientdomain: %s\n", domain);
}

void mfilter_mailfrom(void * filter, const char* from, unsigned int len)
{
	//printf("mfilter_mailfrom: %s\n", from);
}

void mfilter_rcptto(void * filter, const char* to, unsigned int len)
{
	//printf("mfilter_rcptto: %s\n", to);
}

void mfilter_data(void * filter, const char* from, unsigned int len)
{
	//printf("mfilter_data: %d\n", len);
}

void mfilter_result(void * filter, int* isjunk)
{
	*isjunk = -1;
}

void mfilter_exit(void * filter)
{
	MailFilter * tfilter = (MailFilter *)filter;
	if(tfilter->fLog)
		fclose(tfilter->fLog);

	if(tfilter->semLog != SEM_FAILED)
		sem_close(tfilter->semLog);
		
	delete filter;
	//printf("mfilter_exit\n");
}

