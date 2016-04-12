/*
	Copyright (c) erisesoft
*/

#include <stdio.h>
#include "antijunk.h"

/*
Brief:
	Initiate the mfilter, invoked by MTA in a new session beginning
Parameter:
	None
Return:
	Return a filter's handler
*/
void* mfilter_init()
{
	///////////////////////////////////////////////////////////////
	// Example codes
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
	// End example
	///////////////////////////////////////////////////////////////
}

/*
Brief:
	Get the local email domain name
Parameter:
	The handler of the exist filter 
Return:
	None
*/
void mfilter_emaildomain(void* filter, const char* domain, unsigned int len)
{

}

/*
Brief:
	Get the client site's IP in a MTA session
Parameter:
	The handler of the exist filter
	A pointer of the buffer which storage the ip
	The length of buffer
Return:
	None
*/

void mfilter_clientip(void* filter, const char* ip, unsigned int len)
{
	///////////////////////////////////////////////////////////////
	// Example codes
	MailFilter * tfilter = (MailFilter *)filter;

	if(tfilter->semLog && tfilter->fLog)
	{
		sem_wait(tfilter->semLog);

		tm * ltm;
		time_t tt = time(NULL);
		ltm = localtime(&tt);
		fprintf(tfilter->fLog, "[%04d-%02d-%02d %02d:%02d:%02d]: %s\r\n", 1900 + ltm->tm_year, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec, ip);
		fflush(tfilter->fLog);
		sem_post(tfilter->semLog);

	}
	// End example
	///////////////////////////////////////////////////////////////
}

/*
Brief:
	Get the client site's domai nname in a MTA session
Parameter:
	The handler of the exist filter
	A pointer of the buffer which storage the domain name
	The length of buffer
Return:
	None
*/
void mfilter_clientdomain(void * filter, const char* domain, unsigned int len)
{

}

/*
Brief:
	Get the address of "MAIL FROM"
Parameter:
	The handler of the exist filter
	A pointer of the buffer which storage the address of "MAIL FROM"
	The length of buffer
Return:
	None
*/
void mfilter_mailfrom(void * filter, const char* from, unsigned int len)
{

}

/*
Brief:
	Get the address of "RCPT TO"
Parameter:
	The handler of the exist filter
	A pointer of the buffer which storage the address of "RCPT TO"
	The length of buffer
Return:
	None
*/
void mfilter_rcptto(void * filter, const char* to, unsigned int len)
{
	
}

/*
Brief:
	Get each line of mail body
Parameter:
	The handler of the exist filter
	A pointer of the buffer which storage the mail body
	The length of buffer
Return:
	None
*/
void mfilter_data(void * filter, const char* buffer, unsigned int len)
{
	
}

/*
Brief:
	Get the result of filter
Parameter:
	The handler of the exist filter
	The flag whether the mail is a junk mail. -1 is a general mail, other value is junk mail
Return:
	None
*/
void mfilter_result(void * filter, int* isjunk)
{
	///////////////////////////////////////////////////////////////
	// Example codes
	
	*isjunk = -1;
	
	// End example
	///////////////////////////////////////////////////////////////
}

/*
Brief:
	Destory the filter
Parameter:
	The handler of the exist filter
Return:
	None
*/

void mfilter_exit(void * filter)
{
	///////////////////////////////////////////////////////////////
	// Example codes
	MailFilter * tfilter = (MailFilter *)filter;

	if(tfilter->fLog)
		fclose(tfilter->fLog);

	if(tfilter->semLog)
		sem_close(tfilter->semLog);
		
	delete filter;
	// End example
	///////////////////////////////////////////////////////////////
}

