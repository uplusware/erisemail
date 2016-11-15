/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _MTA_H_
#define _MTA_H_
#include <mqueue.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <libmemcached/memcached.h>
#include "util/general.h"
#include "base.h"
#include "storage.h"

class MTA
{
public:
	MTA();
	virtual ~MTA();

	int Run(int fd);
	void Stop();
	void ReloadConfig();
	
	StorageEngine* GetStorageEngine() { return m_storageEngine; }
	
private:
	mqd_t m_mta_qid;
	sem_t* m_mta_sid;
	string m_mta_name;
	
	StorageEngine* m_storageEngine;
	memcached_st * m_memcached;
};

#endif /* _MTA_H_ */

