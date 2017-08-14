/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "service.h"
#include "session.h"
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <semaphore.h>
#include <mqueue.h>
#include <pthread.h>
#include "cache.h"
#include "pool.h"
#include <queue>
#include "letter.h"
#include "mta.h"
#include "session_group.h"

//extern int Run();

static void clear_queue(mqd_t qid)
{
	mq_attr attr;
	struct timespec ts;
	mq_getattr(qid, &attr);
	char* buf = (char*)malloc(attr.mq_msgsize);
	while(1)
	{
		clock_gettime(CLOCK_REALTIME, &ts);
		if(mq_timedreceive(qid, (char*)buf, attr.mq_msgsize, NULL, &ts) == -1)
		{
			break;
		}
	}
	free(buf);
}

void push_reject_list(const char* service_name, const char* ip)
{
	string strqueue = ERISEMAIL_POSIX_PREFIX;
	strqueue += service_name;
	strqueue += ERISEMAIL_POSIX_QUEUE_SUFFIX;

	string strsem = ERISEMAIL_POSIX_PREFIX;
	strsem += service_name;
	strsem += ERISEMAIL_POSIX_SEMAPHORE_SUFFIX;

	mqd_t service_qid = mq_open(strqueue.c_str(), O_RDWR);
	sem_t* service_sid = sem_open(strsem.c_str(), O_RDWR);

	if(service_qid == (mqd_t)-1 || service_sid == SEM_FAILED)
		return;

	stQueueMsg qMsg;
	qMsg.cmd = MSG_REJECT;
	strcpy(qMsg.data.reject_ip, ip);
	sem_wait(service_sid);
	mq_send(service_qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
	sem_post(service_sid);
	
	if(service_qid != (mqd_t)-1)
		mq_close(service_qid);
	if(service_sid != SEM_FAILED)
		sem_close(service_sid);
}

std::queue<Session_Arg*> Service::m_static_thread_pool_arg_queue;
volatile BOOL Service::m_static_thread_pool_exit = TRUE;
pthread_mutex_t Service::m_static_thread_pool_mutex;
sem_t Service::m_static_thread_pool_sem;
volatile unsigned int Service::m_static_thread_pool_size = 0;

void Service::session_handler(Session_Arg* session_arg)
{
	Session* pSession = NULL;
	pSession = new Session(session_arg->sockfd, session_arg->ssl, session_arg->ssl_ctx, session_arg->client_ip.c_str(), session_arg->svr_type, session_arg->is_ssl,
        session_arg->storage_engine, session_arg->cache, session_arg->memcached);
	if(pSession != NULL)
	{
		pSession->Process();

		delete pSession;
	}
    close_ssl(session_arg->ssl, session_arg->ssl_ctx);
	close(session_arg->sockfd);
}

void* Service::new_session_handler(void * arg)
{
	Session_Arg* session_arg = (Session_Arg*)arg;

	session_handler(session_arg);

	if(session_arg)
		delete session_arg;
	pthread_exit(0);
}

void Service::init_thread_pool_handler()
{
	m_static_thread_pool_exit = TRUE;
	m_static_thread_pool_size = 0;
	while(!m_static_thread_pool_arg_queue.empty())
	{
		m_static_thread_pool_arg_queue.pop();
	}

	pthread_mutex_init(&m_static_thread_pool_mutex, NULL);
	sem_init(&m_static_thread_pool_sem, 0, 0);
}

void* Service::begin_thread_pool_handler(void* arg)
{
	m_static_thread_pool_size++;
	struct timespec ts;
    srandom(time(NULL));
    Session_Group * p_session_group = NULL;
    if(CMailBase::m_prod_type == PROD_IM)
        p_session_group = new Session_Group();
    
	while(m_static_thread_pool_exit)
	{
		clock_gettime(CLOCK_REALTIME, &ts);
        if(CMailBase::m_prod_type != PROD_IM)
            ts.tv_sec += 1;
		if(sem_timedwait(&m_static_thread_pool_sem, &ts) == 0)
		{
			Session_Arg* session_arg = NULL;
		
			pthread_mutex_lock(&m_static_thread_pool_mutex);

			if(!m_static_thread_pool_arg_queue.empty())
			{
				session_arg = m_static_thread_pool_arg_queue.front();
				m_static_thread_pool_arg_queue.pop();
			}

			pthread_mutex_unlock(&m_static_thread_pool_mutex);

			if(session_arg)
			{
                if(CMailBase::m_prod_type == PROD_IM)
                {
                    p_session_group->Accept(session_arg->sockfd, session_arg->ssl, session_arg->ssl_ctx, session_arg->client_ip.c_str(), session_arg->svr_type, session_arg->is_ssl,
                        session_arg->storage_engine, session_arg->cache, session_arg->memcached);
                }
                else
                {
                    session_handler(session_arg);
                    delete session_arg;
                
                }
			}
		}
        else
        {
            if(CMailBase::m_prod_type == PROD_IM)
                p_session_group->Poll();
        }
	}
    if(CMailBase::m_prod_type == PROD_IM)
        delete p_session_group;
    
	m_static_thread_pool_size--;
	
	if(arg != NULL)
		delete arg;
	
	pthread_exit(0);
}

void Service::exit_thread_pool_handler()
{
	m_static_thread_pool_exit = FALSE;

	pthread_mutex_destroy(&m_static_thread_pool_mutex);
	sem_close(&m_static_thread_pool_sem);

	unsigned long timeout = 200;
	while(m_static_thread_pool_size > 0 && timeout > 0)
	{
		usleep(1000*10);
		timeout--;
	}
	
}

Service::Service()
{
	m_service_name = MDA_SERVICE_NAME;
    
	m_cache = NULL;
	m_memcached = NULL;
}

Service::Service(const char* service_name)
{
	
	m_service_name = service_name;
    
	m_cache = NULL;
	m_memcached = NULL;
}

Service::~Service()
{
	if(m_cache)
		delete m_cache;
}

void Service::Stop()
{
    
	string strqueue = ERISEMAIL_POSIX_PREFIX;
	strqueue += m_service_name;
	strqueue += ERISEMAIL_POSIX_QUEUE_SUFFIX;

	string strsem = ERISEMAIL_POSIX_PREFIX;
	strsem += m_service_name;
	strsem += ERISEMAIL_POSIX_SEMAPHORE_SUFFIX;
	    
	m_service_qid = mq_open(strqueue.c_str(), O_RDWR);
	m_service_sid = sem_open(strsem.c_str(), O_RDWR);

	if(m_service_qid == (mqd_t)-1 || m_service_sid == SEM_FAILED)
		return;
	
	stQueueMsg qMsg;
	qMsg.cmd = MSG_EXIT;
	sem_wait(m_service_sid);
	mq_send(m_service_qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
	sem_post(m_service_sid);

	if(m_service_qid)
		mq_close(m_service_qid);

	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);
	
	if(m_memcached)
	  memcached_free(m_memcached);
	m_memcached = NULL;
	printf("Stop %s Service OK\n", m_service_name.c_str());
}

void Service::ReloadConfig()
{
	string strqueue = ERISEMAIL_POSIX_PREFIX;
	strqueue += m_service_name;
	strqueue += ERISEMAIL_POSIX_QUEUE_SUFFIX;

	string strsem = ERISEMAIL_POSIX_PREFIX;
	strsem += m_service_name;
	strsem += ERISEMAIL_POSIX_SEMAPHORE_SUFFIX;
	
	m_service_qid = mq_open(strqueue.c_str(), O_RDWR);
	m_service_sid = sem_open(strsem.c_str(), O_RDWR);

	if(m_service_qid == (mqd_t)-1 || m_service_sid == SEM_FAILED)
		return;

	stQueueMsg qMsg;
	qMsg.cmd = MSG_GLOBAL_RELOAD;
	sem_wait(m_service_sid);
	mq_send(m_service_qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
	sem_post(m_service_sid);
	
	if(m_service_qid != (mqd_t)-1)
		mq_close(m_service_qid);
	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);

	printf("Reload %s Service OK\n", m_service_name.c_str());
}

void Service::ReloadList()
{
	string strqueue = ERISEMAIL_POSIX_PREFIX;
	strqueue += m_service_name;
	strqueue += ERISEMAIL_POSIX_QUEUE_SUFFIX;

	string strsem = ERISEMAIL_POSIX_PREFIX;
	strsem += m_service_name;
	strsem += ERISEMAIL_POSIX_SEMAPHORE_SUFFIX;
	
	m_service_qid = mq_open(strqueue.c_str(), O_RDWR);
	m_service_sid = sem_open(strsem.c_str(), O_RDWR);

	if(m_service_qid == (mqd_t)-1 || m_service_sid == SEM_FAILED)
		return;

	stQueueMsg qMsg;
	qMsg.cmd = MSG_LIST_RELOAD;
	sem_wait(m_service_sid);
	mq_send(m_service_qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
	sem_post(m_service_sid);
	
	if(m_service_qid != (mqd_t)-1)
		mq_close(m_service_qid);
	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);
}

int Service::create_server_service(CUplusTrace& uTrace, const char* hostip, unsigned short hostport, int& srv_sockfd)
{
    int nFlag = 0, result = 0;
    struct addrinfo hints;
    struct addrinfo *server_addr, *rp;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    
    char sz_port[32];
    sprintf(sz_port, "%u", hostport);
    
    int s = getaddrinfo((hostip && hostip[0] != '\0') ? hostip : NULL, sz_port, &hints, &server_addr);
    if (s != 0)
    {
       uTrace.Write(Trace_Error, "getaddrinfo: %s", gai_strerror(s));
       srv_sockfd = -1;
       return srv_sockfd;
    }
    
    for (rp = server_addr; rp != NULL; rp = rp->ai_next)
    {
       srv_sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
       if (srv_sockfd == -1)
           continue;
       
       nFlag = 1;
       setsockopt(srv_sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&nFlag, sizeof(nFlag));
    
       if (bind(srv_sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
           break;                  /* Success */
       
       perror("bind");
       close(srv_sockfd);
       srv_sockfd = -1;
    }
    
    if (rp == NULL)
    {               /* No address succeeded */
          uTrace.Write(Trace_Error, "Could not bind");
          srv_sockfd = -1;
          return srv_sockfd;
    }

    freeaddrinfo(server_addr);           /* No longer needed */
    
    nFlag = fcntl(srv_sockfd, F_GETFL, 0);
    fcntl(srv_sockfd, F_SETFL, nFlag|O_NONBLOCK);
    
    if(listen(srv_sockfd, 128) == -1)
    {
        uTrace.Write(Trace_Error, "Service LISTEN error, %s:%u", hostip ? hostip : "", hostport);
        srv_sockfd = -1;
        return srv_sockfd;
    }
    uTrace.Write(Trace_Msg, "Service works on %s:%u", hostip ? hostip : "", hostport);
    
    return srv_sockfd;
}

int Service::create_client_session(CUplusTrace& uTrace, int& clt_sockfd, Service_Type st, BOOL is_ssl, struct sockaddr_storage& clt_addr, socklen_t clt_size)
{
    struct sockaddr_in * v4_addr;
    struct sockaddr_in6 * v6_addr;
        
    char szclientip[INET6_ADDRSTRLEN];
    if (clt_addr.ss_family == AF_INET)
    {
        v4_addr = (struct sockaddr_in*)&clt_addr;
        if(inet_ntop(AF_INET, (void*)&v4_addr->sin_addr, szclientip, INET6_ADDRSTRLEN) == NULL)
        {    
            close(clt_sockfd);
            return 0;
        }
        
    }
    else if(clt_addr.ss_family == AF_INET6)
    {
        v6_addr = (struct sockaddr_in6*)&clt_addr;
        if(inet_ntop(AF_INET6, (void*)&v6_addr->sin6_addr, szclientip, INET6_ADDRSTRLEN) == NULL)
        {    
            close(clt_sockfd);
            return 0;
        }
        
    }
    
    string client_ip = szclientip;
    
    int access_result;
    if(CMailBase::m_permit_list.size() > 0)
    {
        access_result = FALSE;
        for(int x = 0; x < CMailBase::m_permit_list.size(); x++)
        {
            if(strlike(CMailBase::m_permit_list[x].c_str(), client_ip.c_str()) == TRUE)
            {
                access_result = TRUE;
                break;
            }
        }
        
        for(int x = 0; x < CMailBase::m_reject_list.size(); x++)
        {
            if( (strlike(CMailBase::m_reject_list[x].ip.c_str(), (char*)client_ip.c_str()) == TRUE) 
               && (time(NULL) < CMailBase::m_reject_list[x].expire) )
            {
                access_result = FALSE;
                break;
            }
        }
    }
    else
    {
        access_result = TRUE;
        for(int x = 0; x < CMailBase::m_reject_list.size(); x++)
        {
            if( (strlike(CMailBase::m_reject_list[x].ip.c_str(), (char*)client_ip.c_str()) == TRUE)
                && (time(NULL) < CMailBase::m_reject_list[x].expire) )
            {
                access_result = FALSE;
                break;
            }
        }
    }
    
    if(access_result == FALSE)
    {
        close(clt_sockfd);
        uTrace.Write(Trace_Msg, "Reject connection from %s", client_ip.c_str());
        return -1;
    }
    else
    {					
        Session_Arg* session_arg = new Session_Arg;
        session_arg->sockfd = clt_sockfd;
        
        session_arg->client_ip = client_ip;
        session_arg->svr_type = st;
        session_arg->is_ssl = is_ssl;
        session_arg->cache = m_cache;
        session_arg->memcached = m_memcached;
        session_arg->storage_engine = m_storageEngine;
        SSL* ssl = NULL;
        SSL_CTX* ssl_ctx = NULL;
        if(is_ssl)
        {
            if(!create_ssl(clt_sockfd, 
                CMailBase::m_ca_crt_root.c_str(),
                CMailBase::m_ca_crt_server.c_str(),
                CMailBase::m_ca_password.c_str(),
                CMailBase::m_ca_key_server.c_str(),
                CMailBase::m_enableclientcacheck,
                &ssl, &ssl_ctx))
                {
                    close(clt_sockfd);
                    delete session_arg;
                    return -1;
                }
        }
        session_arg->ssl = ssl;
        session_arg->ssl_ctx = ssl_ctx;
        
        pthread_mutex_lock(&m_static_thread_pool_mutex);
        m_static_thread_pool_arg_queue.push(session_arg);
        pthread_mutex_unlock(&m_static_thread_pool_mutex);

        sem_post(&m_static_thread_pool_sem);

    }
    
    return 0;
}

int Service::Run(int fd, vector<service_param_t> & server_params)
{    
    CUplusTrace uTrace(ERISEMAIL_SERVICE_LOGNAME, ERISEMAIL_SERVICE_LCKNAME);
    memcached_server_st * memcached_servers = NULL;
	memcached_return rc;
	m_memcached = NULL;
	for (map<string, int>::iterator iter = CMailBase::m_memcached_list.begin( ); iter != CMailBase::m_memcached_list.end( ); ++iter)
	{
        if(!m_memcached)
            m_memcached = memcached_create(NULL);
		memcached_servers = memcached_server_list_append(memcached_servers, (*iter).first.c_str(), (*iter).second, &rc);
		rc = memcached_server_push(m_memcached, memcached_servers);
		
		uTrace.Write(Trace_Msg, "memcached server@%s:%d", (*iter).first.c_str(), (*iter).second);
	}
	
	m_storageEngine = new StorageEngine(CMailBase::m_db_host.c_str(), 
        CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_max_conn,
         CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str(), CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached);

	if(!m_storageEngine)
	{
		uTrace.Write(Trace_Error, "%s::%s Create Storage Engine Error", __FILE__, __FUNCTION__);
		return -1;
	}
	m_child_list.clear();
	unsigned int result = 0;
	string strqueue = ERISEMAIL_POSIX_PREFIX;
	strqueue += m_service_name;
	strqueue += ERISEMAIL_POSIX_QUEUE_SUFFIX;

	string strsem = ERISEMAIL_POSIX_PREFIX;
	strsem += m_service_name;
	strsem += ERISEMAIL_POSIX_SEMAPHORE_SUFFIX;
    
	mq_attr attr;
	attr.mq_maxmsg = 8;
	attr.mq_msgsize = 1448; 
	attr.mq_flags = 0;

	m_service_qid = (mqd_t)-1;
	m_service_sid = SEM_FAILED;
	
	m_service_qid = mq_open(strqueue.c_str(), O_CREAT|O_RDWR, 0644, &attr);
	m_service_sid = sem_open(strsem.c_str(), O_CREAT|O_RDWR, 0644, 1);

	if((m_service_qid == (mqd_t)-1) || (m_service_sid == SEM_FAILED))
	{		
		if(m_service_sid != SEM_FAILED)
			sem_close(m_service_sid);
	
		if(m_service_qid != (mqd_t)-1)
			mq_close(m_service_qid);

		sem_unlink(strsem.c_str());
		mq_unlink(strqueue.c_str());

		result = 1;
		write(fd, &result, sizeof(unsigned int));
		close(fd);
		return -1;
	}
	
	clear_queue(m_service_qid);
	
	BOOL svr_exit = FALSE;
	int queue_buf_len = attr.mq_msgsize;
	char* queue_buf_ptr = (char*)malloc(queue_buf_len);
    
	ThreadPool worker_pool(CMailBase::m_prod_type == PROD_IM ? CMailBase::m_xmpp_worker_thread_num : CMailBase::m_max_conn, init_thread_pool_handler, begin_thread_pool_handler, NULL, exit_thread_pool_handler);
	
	while(!svr_exit)
	{
        int max_fd = -1;
        for(int x = 0; x < server_params.size(); x++)
        {
            create_server_service(uTrace, server_params[x].host_ip.c_str(), server_params[x].host_port, server_params[x].sockfd);
            if(server_params[x].sockfd > 0)
            {
                if(server_params[x].st == stHTTP)
                {
                    if(!m_cache)
                    {
                        m_cache = new memory_cache();
                        if(m_cache)
                            m_cache->load(CMailBase::m_html_path.c_str());
                    }
                }
                max_fd = max_fd > server_params[x].sockfd ? max_fd : server_params[x].sockfd;
            }
    
        }
         
		if(max_fd == -1)
        {
            result = 1;
            write(fd, &result, sizeof(unsigned int));
            close(fd);
            break;
        }
        else
        {
            result = 0;
            write(fd, &result, sizeof(unsigned int));
            close(fd);
        }
		fd_set accept_mask;
		struct timeval accept_timeout;
		struct timespec ts;
		stQueueMsg* pQMsg;
		int rc;
        	
		while(1)
		{		
			clock_gettime(CLOCK_REALTIME, &ts);
			rc = mq_timedreceive(m_service_qid, queue_buf_ptr, queue_buf_len, 0, &ts);
			if( rc != -1)
			{
				pQMsg = (stQueueMsg*)queue_buf_ptr;
				if(pQMsg->cmd == MSG_EXIT)
				{
					svr_exit = TRUE;
					break;
				}
				else if(pQMsg->cmd == MSG_GLOBAL_RELOAD)
				{
					CMailBase::UnLoadConfig();
					CMailBase::LoadConfig();
				}
				else if(pQMsg->cmd == MSG_LIST_RELOAD)
				{
					CMailBase::UnLoadList();
					CMailBase::LoadList();
				}
				else if(pQMsg->cmd == MSG_REJECT)
				{
					//firstly erase the expire record
					vector<stReject>::iterator x;
					for(x = CMailBase::m_reject_list.begin(); x != CMailBase::m_reject_list.end();)
					{
						if(x->expire < time(NULL))
							CMailBase::m_reject_list.erase(x);
					}
	
					stReject sr;
					sr.ip = pQMsg->data.reject_ip;
					sr.expire = time(NULL) + 5;
					CMailBase::m_reject_list.push_back(sr);
				}
			}
			else
			{
				if((errno != ETIMEDOUT)&&(errno != EINTR)&&(errno != EMSGSIZE))
				{
					perror("");
					svr_exit = TRUE;
					break;
				}
				
			}
            
            FD_ZERO(&accept_mask);
            
            for(int x = 0; x < server_params.size(); x++)
            {
                if(server_params[x].sockfd > 0)
                    FD_SET(server_params[x].sockfd, &accept_mask);
            }
            
            if(max_fd == -1)
            {
				sleep(5);
                break;
            }
            
            accept_timeout.tv_sec = 1;
			accept_timeout.tv_usec = 0;
            
			rc = select(max_fd + 1, &accept_mask, NULL, NULL, &accept_timeout);
			if(rc == -1)
			{	
				sleep(5);
				break;
			}
			else if(rc == 0)
			{
                continue;
			}
			else
			{
                BOOL is_ssl;
                struct sockaddr_storage clt_addr;
				socklen_t clt_size = sizeof(struct sockaddr_storage);
				int clt_sockfd = -1;
                
                for(int x = 0; x < server_params.size(); x++)
                {
                    if(server_params[x].sockfd > 0 && FD_ISSET(server_params[x].sockfd, &accept_mask))
                    {
                        FD_CLR(server_params[x].sockfd, &accept_mask);
                        clt_sockfd = accept(server_params[x].sockfd, (sockaddr*)&clt_addr, &clt_size);
                        if(clt_sockfd > 0)
                        {
                            is_ssl = server_params[x].is_ssl;
                            if(create_client_session(uTrace, clt_sockfd, server_params[x].st, is_ssl, clt_addr, clt_size) < 0)
                                continue;
                        }
                    }
                }
			}
		}
        
        for(int x = 0; x < server_params.size(); x++)
        {
            if(server_params[x].sockfd > 0)
            {
                close(server_params[x].sockfd);
                server_params[x].sockfd = -1;
            }
        }       
	}
	free(queue_buf_ptr);
    
	if(m_service_qid != (mqd_t)-1)
		mq_close(m_service_qid);
	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);

	mq_unlink(strqueue.c_str());
	sem_unlink(strsem.c_str());

	if(m_storageEngine)
		delete m_storageEngine;

	m_storageEngine = NULL;
	
    if(m_memcached)
        memcached_free(m_memcached);
    
    if(memcached_servers)
        memcached_server_list_free(memcached_servers);
	
    if(m_cache)
        delete m_cache;
    
    m_cache = NULL;
    
    CMailBase::UnLoadConfig();
	
	return 0;
}

Watcher::Watcher()
{
    m_watcher_name = WATCHER_SERVICE_NAME;
}

Watcher::~Watcher()
{

}

void Watcher::Stop()
{
	string strqueue = ERISEMAIL_POSIX_PREFIX;
	strqueue += m_watcher_name;
	strqueue += ERISEMAIL_POSIX_QUEUE_SUFFIX;

	string strsem = ERISEMAIL_POSIX_PREFIX;
	strsem += m_watcher_name;
	strsem += ERISEMAIL_POSIX_SEMAPHORE_SUFFIX;
	
	m_watcher_qid = mq_open(strqueue.c_str(), O_RDWR);
	m_watcher_sid = sem_open(strsem.c_str(), O_RDWR);

	if(m_watcher_qid == (mqd_t)-1 || m_watcher_sid == SEM_FAILED)
		return;
	
	stQueueMsg qMsg;
	qMsg.cmd = MSG_EXIT;
	sem_wait(m_watcher_sid);
	mq_send(m_watcher_qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
	sem_post(m_watcher_sid);

	if(m_watcher_qid)
		mq_close(m_watcher_qid);

	if(m_watcher_sid != SEM_FAILED)
		sem_close(m_watcher_sid);

	printf("Stop Service Watcher OK\n");
}

int Watcher::Run(int fd, vector<service_param_t> & server_params, vector<service_param_t> & xmpp_params)
{	
	string strqueue = ERISEMAIL_POSIX_PREFIX;
	strqueue += m_watcher_name;
	strqueue += ERISEMAIL_POSIX_QUEUE_SUFFIX;

	string strsem = ERISEMAIL_POSIX_PREFIX;
	strsem += m_watcher_name;
	strsem += ERISEMAIL_POSIX_SEMAPHORE_SUFFIX;
	unsigned int result = 0;
    
	mq_attr attr;
	attr.mq_maxmsg = 8;
	attr.mq_msgsize = 1448; 
	attr.mq_flags = 0;

	m_watcher_qid = (mqd_t)-1;
	m_watcher_sid = SEM_FAILED;
	
	m_watcher_qid = mq_open(strqueue.c_str(), O_CREAT|O_RDWR, 0644, &attr);
	m_watcher_sid = sem_open(strsem.c_str(), O_CREAT|O_RDWR, 0644, 1);

	if((m_watcher_qid == (mqd_t)-1) || (m_watcher_sid == SEM_FAILED))
	{		
		if(m_watcher_sid != SEM_FAILED)
			sem_close(m_watcher_sid);
	
		if(m_watcher_qid != (mqd_t)-1)
			mq_close(m_watcher_qid);

		sem_unlink(strsem.c_str());
		mq_unlink(strqueue.c_str());

		result = 1;
		write(fd, &result, sizeof(unsigned int));
		close(fd);
		return -1;
	}
	
	result = 0;
	write(fd, &result, sizeof(unsigned int));
	close(fd);
    
	clear_queue(m_watcher_qid);

	BOOL svr_exit = FALSE;
	int queue_buf_len = attr.mq_msgsize;
	char* queue_buf_ptr = (char*)malloc(queue_buf_len);
    
	while(!svr_exit)
	{
		struct timespec ts;
		stQueueMsg* pQMsg;
		int rc;
		while(1)
		{		
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += 5;
			rc = mq_timedreceive(m_watcher_qid, queue_buf_ptr, queue_buf_len, 0, &ts);
			if( rc != -1)
			{
				pQMsg = (stQueueMsg*)queue_buf_ptr;
				if(pQMsg->cmd == MSG_EXIT)
				{
					svr_exit = TRUE;
					break;
				}
			}
			else
			{
				if((errno != ETIMEDOUT)&&(errno != EINTR)&&(errno != EMSGSIZE))
				{
					perror("");
					svr_exit = TRUE;
					break;
				}
				
			}
            
            int pfd[2];            
            BOOL isRun = FALSE;
            char szFlag[128];
            sprintf(szFlag, "/tmp/erisemail/%s.pid", MDA_SERVICE_NAME);
            if(!try_single_on(szFlag) && server_params.size() > 0)   
            {
                printf("%s Service stopped.\n", MDA_SERVICE_NAME);
                pipe(pfd);
                int mda_pid = fork();
                if(mda_pid == 0)
                {                      
                    close(pfd[0]);
                    if(check_single_on(szFlag)) 
                    {
                        printf("%s Service is aready runing.\n", MDA_SERVICE_NAME);   
                        exit(-1);
                    }
                    Service mda_svr;
                    if(check_single_on(szFlag)) 
                    {
                        printf("%s Service is aready runing.\n", MDA_SERVICE_NAME);   
                        exit(-1);
                    }
                    mda_svr.Run(pfd[1], server_params);
                    exit(0);
                }
                else if(mda_pid > 0)
                {
                    unsigned int result;
                    close(pfd[1]);
                    read(pfd[0], &result, sizeof(unsigned int));
                    if(result == 0)
                        printf("Start %s Service OK \t\t\t[%u]\n", MDA_SERVICE_NAME, mda_pid);
                    else
                    {
                        printf("Start %s Service Failed. \t\t\t[Error]\n", MDA_SERVICE_NAME);
                    }
                    close(pfd[0]);
                }                  
            }

            sprintf(szFlag, "/tmp/erisemail/%s.pid", MTA_SERVICE_NAME);
            if(!try_single_on(szFlag) && CMailBase::m_enablemta)   
            {
                printf("Service %s stopped.\n", MTA_SERVICE_NAME);   
                int mta_pids;
                pipe(pfd);
                mta_pids = fork();
                if(mta_pids == 0)
                {
                    close(pfd[0]);
                    if(check_single_on(szFlag)) 
                    {
                        printf("Service %s is aready runing.\n", MTA_SERVICE_NAME);   
                        exit(-1);
                    }
                    MTA mta;
                    mta.Run(pfd[1]);
                    exit(0);
                }
                else if(mta_pids > 0)
                {
                    unsigned int result;
                    close(pfd[1]);
                    read(pfd[0], &result, sizeof(unsigned int));
                    if(result == 0)
                        printf("Start %s Service OK \t\t\t[%u]\n", MTA_SERVICE_NAME, mta_pids);
                    else
                    {
                        printf("Start %s Service Failed \t\t\t[Error]\n", MTA_SERVICE_NAME);
                    }
                    close(pfd[0]);
                } 
            }
            
            sprintf(szFlag, "/tmp/erisemail/%s.pid", XMPP_SERVICE_NAME);
            if(!try_single_on(szFlag) && xmpp_params.size() > 0)   
            {
                printf("%s Service stopped.\n", XMPP_SERVICE_NAME);
                pipe(pfd);
                int xmpp_pid = fork();
                if(xmpp_pid == 0)
                {
                    CMailBase::m_prod_type = PROD_IM;
                    
                    close(pfd[0]);
                    if(check_single_on(szFlag)) 
                    {
                        printf("%s Service is aready runing.\n", XMPP_SERVICE_NAME);   
                        exit(-1);
                    }
                    Service xmpp_svr(XMPP_SERVICE_NAME);
                    if(check_single_on(szFlag)) 
                    {
                        printf("%s Service is aready runing.\n", XMPP_SERVICE_NAME);   
                        exit(-1);
                    }
                    xmpp_svr.Run(pfd[1], xmpp_params);
                    exit(0);
                }
                else if(xmpp_pid > 0)
                {
                    unsigned int result;
                    close(pfd[1]);
                    read(pfd[0], &result, sizeof(unsigned int));
                    if(result == 0)
                        printf("Start %s Service OK \t\t\t[%u]\n", XMPP_SERVICE_NAME, xmpp_pid);
                    else
                    {
                        printf("Start %s Service Failed. \t\t\t[Error]\n", XMPP_SERVICE_NAME);
                    }
                    close(pfd[0]);
                }                  
            }
		}
	}
	free(queue_buf_ptr);
	if(m_watcher_qid != (mqd_t)-1)
		mq_close(m_watcher_qid);
	if(m_watcher_sid != SEM_FAILED)
		sem_close(m_watcher_sid);

	mq_unlink(strqueue.c_str());
	sem_unlink(strsem.c_str());
	
	return 0;
}
