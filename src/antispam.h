/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#ifndef _ANTISPAM_H_
#define _ANTISPAM_H_

#include <arpa/inet.h>
#include <fcntl.h>
#include <mqueue.h>
#include <netdb.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <time.h>
#include <unistd.h>
#include <string>
using namespace std;

struct popen2 {
  pid_t child_pid;
  int from_child;
  int to_child;
};

typedef struct {
  int is_spam;
  int is_virs;
  string param;
  struct popen2* fd;
} MailFilter;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus__ */

void* mfilter_init(const char* param);

void mfilter_emaildomain(void* filter, const char* domain, unsigned int len);

void mfilter_clientip(void* filter, const char* ip, unsigned int len);

void mfilter_clientdomain(void* filter, const char* domain, unsigned int len);

void mfilter_mailfrom(void* filter, const char* from, unsigned int len);

void mfilter_rcptto(void* filter, const char* to, unsigned int len);

void mfilter_data(void* filter, const char* data, unsigned int len);

void mfilter_eml(void* filter, const char* emlpath, unsigned int len);

void mfilter_result(void* filter, int* is_spam);

void mfilter_exit(void* filter);

#ifdef __cplusplus
}
#endif /* __cplusplus__ */

#endif /* _ANTISPAM_H_ */
