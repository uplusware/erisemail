/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include <stdio.h>
#include "antispam.h"

/*
Brief:
	Initiate the mfilter, invoked by MTA in a new session beginning
Parameter:
	Parameter string for this filter
Return:
	Return a filter's handler
*/
void* mfilter_init(const char* param)
{
	///////////////////////////////////////////////////////////////
	// Example codes
	MailFilter * filter = new MailFilter;
	if(filter)
	{
		filter->is_spam = -1;
        filter->is_virs = -1;
        filter->param = param;
    
	}

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
void mfilter_data(void * filter, const char* data, unsigned int len)
{
	
}

/*
Brief:
	check the eml file
Parameter:
	The handler of the exist filter
	A zero teminated string to the eml file path
	The length of path string
Return:
	None
*/
void mfilter_eml(void * filter, const char* emlpath, unsigned int len)
{
    
}

/*
Brief:
	Get the result of filter
Parameter:
	The handler of the exist filter
	The flag whether the mail is a spam mail. -1 is a general mail, other value is spam mail
Return:
	None
*/
void mfilter_result(void * filter, int* is_spam)
{
	///////////////////////////////////////////////////////////////
	// Example codes
	
	*is_spam = -1;
	
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
		
	delete filter;
    
	// End example
	///////////////////////////////////////////////////////////////
}

