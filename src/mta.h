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
#include <queue>
#include "util/general.h"
#include "base.h"
#include "storage.h"

typedef struct
{
	Mail_Info mail_info;
	StorageEngine* _storageEngine;
	memcached_st * _memcached;
}ReplyInfo;

class MailTransferAgent
{
public:
	MailTransferAgent();
	virtual ~MailTransferAgent();

	int Run(int fd);
	void Stop();
	void ReloadConfig();
	
	StorageEngine* GetStorageEngine() { return m_storageEngine; }
	
    static void INIT_RELAY_HANDLER();
    static void* BEGIN_RELAY_HANDLER(void* arg);
    static void EXIT_RELAY_HANDLER();
    
    static BOOL ReturnMail(MailStorage* mailStg, memcached_st * memcached,
        int mid, const char* uniqid, const char *mail_from, const char *rcpt_to, const char * errormsg);
        
    static BOOL SendMail(MailStorage* mailStg, memcached_st * memcached, 
        const char* mxserver, const char* fromaddr, const char* toaddr,
        unsigned int mid, string& errormsg);
        
    static BOOL RelayMail(MailStorage* mailStg, memcached_st * memcached, int mid, const char* uniqid, const char *mail_from, const char *rcpt_to, const char * mail_host, string& errormsg);
    
private:
	mqd_t m_mta_qid;
	sem_t* m_mta_sid;
	string m_mta_name;
	
	StorageEngine* m_storageEngine;
	memcached_st * m_memcached;
    
    static volatile unsigned int        m_STATIC_THREAD_POOL_SIZE;
    static pthread_mutex_t              m_STATIC_THREAD_POOL_SIZE_MUTEX;

    static pthread_rwlock_t m_STATIC_THREAD_IDLE_NUM_LOCK;
    static volatile unsigned int m_STATIC_THREAD_IDLE_NUM;
    
    static volatile BOOL m_s_relay_stop;
    
    static std::queue<ReplyInfo*> m_s_thread_pool_arg_queue;
    static pthread_mutex_t m_s_thread_pool_mutex;
    static sem_t m_s_thread_pool_sem;
    static volatile unsigned int m_s_thread_pool_size;
};

#endif /* _MTA_H_ */

