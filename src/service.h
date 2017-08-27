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
#include "util/trace.h"
#include "cache.h"
#include "posixname.h"

void push_reject_list(const char* service_name, const char* ip);

typedef struct
{
    Service_Type st;
    string host_ip;
    unsigned host_port;
    BOOL is_ssl;
    
    int sockfd;
    
} service_param_t;

typedef struct _session_arg_
{
	int sockfd;
	string client_ip;
    unsigned short client_port;
	Service_Type svr_type;
    BOOL is_ssl;
    SSL * ssl;
    SSL_CTX * ssl_ctx;
	memory_cache* cache;
	StorageEngine* storage_engine;
	memcached_st * memcached;
} Session_Arg;

class Service
{
public:
	Service();
    Service(const char* service_name);
	virtual ~Service();
	int Run(int fd, vector<service_param_t> & server_params);
	void Stop();
	void ReloadConfig();
	void ReloadList();
	
	memory_cache* m_cache;
	
    //static
    static void session_handler(Session_Arg* session_arg);
    static void* new_session_handler(void * arg);
    static void init_thread_pool_handler();
    static void* begin_thread_pool_handler(void* arg);
    static void exit_thread_pool_handler();
    
protected:
    int create_server_service(CUplusTrace& uTrace, const char* hostip, unsigned short hostport, int& srv_sockfd);
    int create_client_session(CUplusTrace& uTrace, int& clt_sockfd, Service_Type st, BOOL is_ssl, struct sockaddr_storage& clt_addr, socklen_t clt_size);
    
	mqd_t m_service_qid;
	
	sem_t* m_service_sid;
	string m_service_name;
    
	list<pid_t> m_child_list;

	StorageEngine* m_storageEngine;
	memcached_st * m_memcached;
    
    //static
    static std::queue<Session_Arg*>     m_static_thread_pool_arg_queue;
    static volatile BOOL                m_static_thread_pool_exit;
    static pthread_mutex_t              m_static_thread_pool_mutex;
    static sem_t                        m_static_thread_pool_sem;
    static volatile unsigned int        m_static_thread_pool_size;

};

class Watcher
{
public:
	Watcher();
	virtual ~Watcher();
	int Run(int fd, vector<service_param_t> & server_params, vector<service_param_t> & xmpp_params);
	void Stop();
protected:
	mqd_t m_watcher_qid;
	sem_t* m_watcher_sid;
    string m_watcher_name;
};

#endif /* _SERVICE_H_ */

