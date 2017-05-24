/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _ANTISPAM_H_
#define _ANTISPAM_H_

#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include<string.h>

typedef struct
{
	int isSpam;
	int isVirs;
	string param;
} MailFilter;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus__ */

	void* mfilter_init(const char* param);

	void mfilter_emaildomain(void* filter, const char* domain, unsigned int len);

	void mfilter_clientip(void* filter, const char* ip, unsigned int len);

	void mfilter_clientdomain(void * filter, const char* domain, unsigned int len);

	void mfilter_mailfrom(void * filter, const char* from, unsigned int len);

	void mfilter_rcptto(void * filter, const char* to, unsigned int len);

	void mfilter_data(void * filter, const char* data, unsigned int len);
    
    void mfilter_eml(void * filter, const char* emlpath, unsigned int len);
    
	void mfilter_result(void * filter, int* isspam);

	void mfilter_exit(void * filter);

#ifdef __cplusplus
}
#endif /* __cplusplus__ */

#endif /* _ANTISPAM_H_ */
