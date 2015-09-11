#ifndef _ANTIJUNK_H_
#define _ANTIJUNK_H_

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
	int isJunk;
	int isVirs;
	FILE * fLog;
	sem_t* semLog;
} MailFilter;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus__ */

	void* mfilter_init();

	void mfilter_emaildomain(void* filter, const char* domain, unsigned int len);

	void mfilter_clientip(void* filter, const char* ip, unsigned int len);

	void mfilter_clientdomain(void * filter, const char* domain, unsigned int len);

	void mfilter_mailfrom(void * filter, const char* from, unsigned int len);

	void mfilter_rcptto(void * filter, const char* to, unsigned int len);

	void mfilter_data(void * filter, const char* from, unsigned int len);

	void mfilter_result(void * filter, int* isjunk);

	void mfilter_exit(void * filter);

#ifdef __cplusplus
}
#endif /* __cplusplus__ */

#endif /* _ANTIJUNK_H_ */
