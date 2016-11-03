/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _SERVICE_H_
#define _SERVICE_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/select.h>
#include "util/general.h"
#include "base.h"
#include "session.h"
#include <mqueue.h>
#include <semaphore.h>
#include <libmemcached/memcached.h>

#include "cache.h"

static const char* SVR_NAME_TBL[] = {NULL, "SMTP",	"POP3",	"IMAP",	"HTTP", "MTA", NULL};
static const char* SVR_DESP_TBL[] = {NULL, "SMTP",	"POP3",	"IMAP",	"HTTP", "MTA", NULL};

void push_reject_list(Service_Type st, const char* ip);

class Service
{
public:
	Service(Service_Type st);
	virtual ~Service();
	int Run(int fd, const char* hostip, unsigned short port, unsigned short ssl_port);
	void Stop();
	void ReloadConfig();
	void ReloadList();
	
	memory_cache* m_cache;
	
protected:
    int Accept(int& clt_sockfd, BOOL is_ssl, struct sockaddr_storage& clt_addr, socklen_t clt_size);
    
	mqd_t m_service_qid;
	
	sem_t* m_service_sid;
	string m_service_name;
	int m_sockfd;
    int m_sockfd_ssl;
	Service_Type m_st;
	list<pid_t> m_child_list;

	StorageEngine* m_storageEngine;
	memcached_st * m_memcached;
};

class WatchDog
{
public:
	WatchDog();
	virtual ~WatchDog();
	int Run(int fd);
	void Stop();
protected:
	mqd_t m_watchdog_qid;
	sem_t* m_watchdog_sid;
};

#endif /* _SERVICE_H_ */

