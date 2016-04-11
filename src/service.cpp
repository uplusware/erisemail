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
#include "util/trace.h"
#include "letter.h"
#include "spool.h"

extern int Run();

typedef struct _session_arg_
{
	int sockfd;
	string client_ip;
	Service_Type svr_type;
	memory_cache* cache;
	StorageEngine* storage_engine;
	memcached_st * memcached;
} Session_Arg;

static std::queue<Session_Arg*> s_thread_pool_arg_queue;

static volatile BOOL s_thread_pool_exit = TRUE;
static pthread_mutex_t s_thread_pool_mutex;
static sem_t s_thread_pool_sem;
static volatile unsigned int s_thread_pool_size = 0;

static void session_handler(Session_Arg* session_arg)
{
	Session* pSession = NULL;
	pSession = new Session(session_arg->sockfd, session_arg->client_ip.c_str(), session_arg->svr_type, session_arg->storage_engine, session_arg->cache, session_arg->memcached);
	if(pSession != NULL)
	{
		pSession->Process();

		delete pSession;
	}
	close(session_arg->sockfd);
}

static void* new_session_handler(void * arg)
{
	Session_Arg* session_arg = (Session_Arg*)arg;

	session_handler(session_arg);

	if(session_arg)
		delete session_arg;
	pthread_exit(0);
}

static void init_thread_pool_handler()
{
	s_thread_pool_exit = TRUE;
	s_thread_pool_size = 0;
	while(!s_thread_pool_arg_queue.empty())
	{
		s_thread_pool_arg_queue.pop();
	}

	pthread_mutex_init(&s_thread_pool_mutex, NULL);
	sem_init(&s_thread_pool_sem, 0, 0);
}

static void* begin_thread_pool_handler(void* arg)
{
	s_thread_pool_size++;
	struct timespec ts;
	while(s_thread_pool_exit)
	{
		
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += 1;
		if(sem_timedwait(&s_thread_pool_sem, &ts) == 0)
		{
			Session_Arg* session_arg = NULL;
		
			pthread_mutex_lock(&s_thread_pool_mutex);

			if(!s_thread_pool_arg_queue.empty())
			{
				session_arg = s_thread_pool_arg_queue.front();
				s_thread_pool_arg_queue.pop();
			}

			pthread_mutex_unlock(&s_thread_pool_mutex);

			if(session_arg)
			{
				session_handler(session_arg);
				delete session_arg;
			}
		}
	}
	s_thread_pool_size--;
	
	if(arg != NULL)
		delete arg;
	
	pthread_exit(0);
}

static void exit_thread_pool_handler()
{
	s_thread_pool_exit = FALSE;

	pthread_mutex_destroy(&s_thread_pool_mutex);
	sem_close(&s_thread_pool_sem);

	unsigned long timeout = 200;
	while(s_thread_pool_size > 0 && timeout > 0)
	{
		usleep(1000*10);
		timeout--;
	}
	
}

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

void push_reject_list(Service_Type st, const char* ip)
{
	string strqueue = "/.";
	strqueue += SVR_NAME_TBL[st];
	strqueue += "_queue";

	string strsem = "/.";
	strsem += SVR_NAME_TBL[st];;
	strsem += "_lock";

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

Service::Service(Service_Type st)
{
	m_sockfd = -1;
	m_st = st;
	m_service_name = SVR_NAME_TBL[m_st];

	m_cache = NULL;
	m_memcached = NULL;
	
	if((m_st == stHTTP) || (m_st == stHTTPS))
	{
		m_cache = new memory_cache();
		m_cache->load(CMailBase::m_html_path.c_str());
	}
}

Service::~Service()
{
	if(m_cache)
		delete m_cache;
}

void Service::Stop()
{
	string strqueue = "/.";
	strqueue += m_service_name;
	strqueue += "_queue";

	string strsem = "/.";
	strsem += m_service_name;
	strsem += "_lock";
	
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
	printf("Stop %s OK\n", SVR_DESP_TBL[m_st]);
}

void Service::ReloadConfig()
{
	string strqueue = "/.";
	strqueue += m_service_name;
	strqueue += "_queue";

	string strsem = "/.";
	strsem += m_service_name;
	strsem += "_lock";
	
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

	printf("Reload %s OK\n", SVR_DESP_TBL[m_st]);
}

void Service::ReloadList()
{
	string strqueue = "/.";
	strqueue += m_service_name;
	strqueue += "_queue";

	string strsem = "/.";
	strsem += m_service_name;
	strsem += "_lock";
	
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

int Service::Run(int fd, const char* hostip, unsigned short nPort)
{	
	CUplusTrace uTrace(LOGNAME, LCKNAME);
	CMailBase::LoadConfig();
	memcached_server_st * memcached_servers = NULL;
	memcached_return rc;
	m_memcached = memcached_create(NULL);
	for (map<string, int>::iterator iter = CMailBase::m_memcached_list.begin( ); iter != CMailBase::m_memcached_list.end( ); ++iter)
	{
		memcached_servers = memcached_server_list_append(memcached_servers, (*iter).first.c_str(), (*iter).second, &rc);
		rc = memcached_server_push(m_memcached, memcached_servers);
		
		//printf("memcached: %s %d %d\n", (*iter).first.c_str(), (*iter).second, rc);
	}
	
	m_storageEngine = new StorageEngine(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_max_conn);

	if(!m_storageEngine)
	{
		printf("%s::%s Create Storage Engine Error\n", __FILE__, __FUNCTION__);
		return -1;
	}
	m_child_list.clear();
	unsigned int result = 0;
	string strqueue = "/.";
	strqueue += m_service_name;
	strqueue += "_queue";

	string strsem = "/.";
	strsem += m_service_name;
	strsem += "_lock";
	
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
	int qBufLen = attr.mq_msgsize;
	char* qBufPtr = (char*)malloc(qBufLen);

	ThreadPool thd_pool(CMailBase::m_max_conn, init_thread_pool_handler, begin_thread_pool_handler, NULL, exit_thread_pool_handler);
	
	while(!svr_exit)
	{
		int nFlag;
		
		struct sockaddr_in svr_addr;
		
		bzero(&svr_addr, sizeof(struct sockaddr_in));
		
		m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		
		svr_addr.sin_family = AF_INET;
		svr_addr.sin_port = htons(nPort);
		svr_addr.sin_addr.s_addr = (checkip(hostip) == -1 ? INADDR_ANY : inet_addr(hostip));
		
		nFlag =1;
		setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&nFlag, sizeof(nFlag));
		
		if(bind(m_sockfd, (struct sockaddr*)&svr_addr, sizeof(struct sockaddr_in)) == -1)
		{
			uTrace.Write(Trace_Error, "Service BIND error, Port: %d.", nPort);
			result = 1;
			write(fd, &result, sizeof(unsigned int));
			close(fd);
			break;
		}
		
		nFlag = fcntl(m_sockfd, F_GETFL, 0);
		fcntl(m_sockfd, F_SETFL, nFlag|O_NONBLOCK);
		
		if(listen(m_sockfd, 128) == -1)
		{
			uTrace.Write(Trace_Error, "Service LISTEN error.");
			result = 1;
			write(fd, &result, sizeof(unsigned int));
			close(fd);
			break;
		}

		result = 0;
		write(fd, &result, sizeof(unsigned int));
		close(fd);
		fd_set accept_mask;
		struct timeval accept_timeout;
		struct timespec ts;
		stQueueMsg* pQMsg;
		int rc;
		while(1)
		{		
			clock_gettime(CLOCK_REALTIME, &ts);
			rc = mq_timedreceive(m_service_qid, qBufPtr, qBufLen, 0, &ts);
			if( rc != -1)
			{
				pQMsg = (stQueueMsg*)qBufPtr;
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
			FD_SET(m_sockfd, &accept_mask);
			
			accept_timeout.tv_sec = 1;
			accept_timeout.tv_usec = 0;
			rc = select(m_sockfd + 1, &accept_mask, NULL, NULL, &accept_timeout);
			if(rc == 0)
			{	
				continue;
			}
			else if(rc == 1)
			{
				struct sockaddr_in clt_addr;
				socklen_t clt_size = sizeof(struct sockaddr_in);
				int clt_sockfd;
				if(FD_ISSET(m_sockfd, &accept_mask))
				{
					
					CMailBase::m_global_uid++;
					clt_sockfd = accept(m_sockfd, (sockaddr*)&clt_addr, &clt_size);

					if(clt_sockfd < 0)
					{
						continue;
					}
					string client_ip = inet_ntoa(clt_addr.sin_addr);
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
							if( (strlike(CMailBase::m_reject_list[x].ip.c_str(), (char*)client_ip.c_str()) == TRUE) && (time(NULL) < CMailBase::m_reject_list[x].expire) )
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
							if( (strlike(CMailBase::m_reject_list[x].ip.c_str(), (char*)client_ip.c_str()) == TRUE) && (time(NULL) < CMailBase::m_reject_list[x].expire) )
							{
								access_result = FALSE;
								break;
							}
						}
					}
					
					if(access_result == FALSE)
					{
						//printf("reject: %s\n", client_ip.c_str());
						close(clt_sockfd);
					}
					else
					{					
						Session_Arg* session_arg = new Session_Arg;
						session_arg->sockfd = clt_sockfd;
						
						session_arg->client_ip = client_ip;
						session_arg->svr_type = m_st;
						session_arg->cache = m_cache;
						session_arg->memcached = m_memcached;
						session_arg->storage_engine = m_storageEngine;

						/*
						pthread_attr_t attr;
						pthread_attr_init(&attr);
						pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
						pthread_t new_session_pth_id;
						
						if( pthread_create(&new_session_pth_id, &attr, new_session_handler, (void*)session_arg) != 0 )
						{
							close(clt_sockfd);
							delete session_arg;
						}

						pthread_attr_destroy (&attr);
						*/

						pthread_mutex_lock(&s_thread_pool_mutex);
						s_thread_pool_arg_queue.push(session_arg);
						pthread_mutex_unlock(&s_thread_pool_mutex);

						sem_post(&s_thread_pool_sem);
		
					}
				}
				else
				{
					continue;
				}
				
			}
			else
			{
				break;
			}
		}
		if(m_sockfd)
		{
			m_sockfd = -1;
			close(m_sockfd);
		}
	}
	free(qBufPtr);
	if(m_service_qid != (mqd_t)-1)
		mq_close(m_service_qid);
	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);

	mq_unlink(strqueue.c_str());
	sem_unlink(strsem.c_str());

	if(m_storageEngine)
		delete m_storageEngine;

	m_storageEngine = NULL;
	
	CMailBase::UnLoadConfig();
	
	return 0;
}

WatchDog::WatchDog()
{
}

WatchDog::~WatchDog()
{

}

void WatchDog::Stop()
{
	string strqueue = "/.";
	strqueue += "WATCHDOG";
	strqueue += "_queue";

	string strsem = "/.";
	strsem += "WATCHDOG";
	strsem += "_lock";
	
	m_watchdog_qid = mq_open(strqueue.c_str(), O_RDWR);
	m_watchdog_sid = sem_open(strsem.c_str(), O_RDWR);

	if(m_watchdog_qid == (mqd_t)-1 || m_watchdog_sid == SEM_FAILED)
		return;
	
	stQueueMsg qMsg;
	qMsg.cmd = MSG_EXIT;
	sem_wait(m_watchdog_sid);
	mq_send(m_watchdog_qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
	sem_post(m_watchdog_sid);

	if(m_watchdog_qid)
		mq_close(m_watchdog_qid);

	if(m_watchdog_sid != SEM_FAILED)
		sem_close(m_watchdog_sid);

	printf("Stop Watch Dog OK\n");
}

int WatchDog::Run(int fd)
{	
	string strqueue = "/.";
	strqueue += "WATCHDOG";
	strqueue += "_queue";

	string strsem = "/.";
	strsem += "WATCHDOG";
	strsem += "_lock";
	unsigned int result = 0;
    
	mq_attr attr;
	attr.mq_maxmsg = 8;
	attr.mq_msgsize = 1448; 
	attr.mq_flags = 0;

	m_watchdog_qid = (mqd_t)-1;
	m_watchdog_sid = SEM_FAILED;
	
	m_watchdog_qid = mq_open(strqueue.c_str(), O_CREAT|O_RDWR, 0644, &attr);
	m_watchdog_sid = sem_open(strsem.c_str(), O_CREAT|O_RDWR, 0644, 1);

	if((m_watchdog_qid == (mqd_t)-1) || (m_watchdog_sid == SEM_FAILED))
	{		
		if(m_watchdog_sid != SEM_FAILED)
			sem_close(m_watchdog_sid);
	
		if(m_watchdog_qid != (mqd_t)-1)
			mq_close(m_watchdog_qid);

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
    
	clear_queue(m_watchdog_qid);

	BOOL svr_exit = FALSE;
	int qBufLen = attr.mq_msgsize;
	char* qBufPtr = (char*)malloc(qBufLen);
    
	while(!svr_exit)
	{
		struct timespec ts;
		stQueueMsg* pQMsg;
		int rc;
		while(1)
		{		
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += 5;
			rc = mq_timedreceive(m_watchdog_qid, qBufPtr, qBufLen, 0, &ts);
			if( rc != -1)
			{
				pQMsg = (stQueueMsg*)qBufPtr;
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
            sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stSMTP]);
            if(!try_single_on(szFlag))   
            {
                printf("%s stopped.\n", SVR_DESP_TBL[stSMTP]);
                pipe(pfd);
                int smtp_pid = fork();
                if(smtp_pid == 0)
                {                      
                    close(pfd[0]);
                    if(check_single_on(szFlag)) 
                    {
                        printf("%s is aready runing.\n", SVR_DESP_TBL[stSMTP]);   
                        exit(-1);
                    }
                    Service smtp_svr(stSMTP);
                    if(check_single_on(szFlag)) 
                    {
                        printf("%s is aready runing.\n", SVR_DESP_TBL[stSMTP]);   
                        exit(-1);
                    }
                    smtp_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_smtpport);
                    exit(0);
                }
                else if(smtp_pid > 0)
                {
                    unsigned int result;
                    close(pfd[1]);
                    read(pfd[0], &result, sizeof(unsigned int));
                    if(result == 0)
                        printf("Start SMTP Service OK \t\t\t[%u]\n", smtp_pid);
                    else
                    {
                        printf("Start SMTP Service Failed. \t\t\t[Error]\n");
                    }
                    close(pfd[0]);
                }                  
            }
            
            sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stPOP3]);
            if(!try_single_on(szFlag)  && CMailBase::m_enablepop3)   
            {
                printf("%s stopped.\n", SVR_DESP_TBL[stPOP3]);   
                pipe(pfd);
                int pop3_pid = fork();
                if(pop3_pid == 0)
                {
                    close(pfd[0]);
                    if(check_single_on(szFlag)) 
                    {
                        printf("%s is aready runing.\n", SVR_DESP_TBL[stPOP3]);   
                        exit(-1);
                    }

                    Service pop3_svr(stPOP3);
                    pop3_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_pop3port);
                    exit(0);
                }
                else if(pop3_pid > 0)
                {
                    unsigned int result;
                    close(pfd[1]);
                    read(pfd[0], &result, sizeof(unsigned int));
                    if(result == 0)
                        printf("Start POP3 Service OK \t\t\t[%u]\n", pop3_pid);
                    else
                    {
                        printf("Start POP3 Service Failed. \t\t\t[Error]\n");
                    }
                    close(pfd[0]);
                } 
            }

            sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stIMAP]);
            if(!try_single_on(szFlag)  && CMailBase::m_enableimap)    
            {
                printf("%s stopped.\n", SVR_DESP_TBL[stIMAP]);   
                pipe(pfd);
                int imap_pid = fork();
                if(imap_pid == 0)
                {
                    close(pfd[0]);
                    if(check_single_on(szFlag)) 
                    {
                        printf("%s is aready runing.\n", SVR_DESP_TBL[stSMTP]);   
                        exit(-1);
                    }
                    Service imap_svr(stIMAP);
                    imap_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_imapport);
                    exit(0);
                }
                else if(imap_pid > 0)
                {
                    unsigned int result;
                    close(pfd[1]);
                    read(pfd[0], &result, sizeof(unsigned int));
                    if(result == 0)
                        printf("Start IMAP Service OK \t\t\t[%u]\n", imap_pid);
                    else
                    {
                        printf("Start IMAP Service Failed. \t\t\t[Error]\n");
                    }
                    close(pfd[0]);
                } 
            }
            sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stHTTP]);
            if(!try_single_on(szFlag)  && CMailBase::m_enablehttp)    
            {
                printf("%s stopped.\n", SVR_DESP_TBL[stHTTP]);   
                pipe(pfd);
                int http_pid = fork();
                if(http_pid == 0)
                {
                    close(pfd[0]);
                    if(check_single_on(szFlag)) 
                    {
                        printf("%s is aready runing.\n", SVR_DESP_TBL[stSMTP]);   
                        exit(-1);
                    }
                    Service http_svr(stHTTP);
                    http_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_httpport);
                    exit(0);
                }
                else if(http_pid > 0)
                {
                    unsigned int result;
                    close(pfd[1]);
                    read(pfd[0], &result, sizeof(unsigned int));
                    if(result == 0)
                        printf("Start HTTP Service OK \t\t\t[%u]\n", http_pid);
                    else
                    {
                        printf("Start HTTP Service Failed. \t\t\t[Error]\n");
                    }
                    close(pfd[0]);
                } 
            }

            sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stSMTPS]);
            if(!try_single_on(szFlag) && CMailBase::m_enablesmtps)    
            {
                printf("%s stopped.\n", SVR_DESP_TBL[stSMTPS]);   
                pipe(pfd);
                int smtps_pid = fork();
                if(smtps_pid == 0)
                {
                    close(pfd[0]);
                    if(check_single_on(szFlag)) 
                    {
                        printf("%s is aready runing.\n", SVR_DESP_TBL[stSMTP]);   
                        exit(-1);
                    }
                    Service smtps_svr(stSMTPS);
                    smtps_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_smtpsport);
                    exit(0);
                    
                }
                else if(smtps_pid > 0)
                {
                    unsigned int result;
                    close(pfd[1]);
                    read(pfd[0], &result, sizeof(unsigned int));
                    if(result == 0)
                        printf("Start SMTP on SSL Service OK \t\t[%u]\n", smtps_pid);
                    else
                    {
                        printf("Start SMTP on SSL Service Failed. \t\t[Error]\n");
                    }
                    close(pfd[0]);
                } 
            }
            
            sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stPOP3S]);
            if(!try_single_on(szFlag)  && CMailBase::m_enablepop3s )   
            {
                printf("%s stopped.\n", SVR_DESP_TBL[stPOP3S]);   
                pipe(pfd);
                int pop3s_pid = fork();
                if(pop3s_pid == 0)
                {
                    close(pfd[0]);
                    if(check_single_on(szFlag)) 
                    {
                        printf("%s is aready runing.\n", SVR_DESP_TBL[stSMTP]);   
                        exit(-1);
                    }
                    Service pop3s_svr(stPOP3S);
                    pop3s_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_pop3sport);
                    exit(0);
                }
                else if(pop3s_pid > 0)
                {
                    unsigned int result;
                    close(pfd[1]);
                    read(pfd[0], &result, sizeof(unsigned int));
                    if(result == 0)
                        printf("Start POP3 on SSL Service OK \t\t[%u]\n", pop3s_pid);
                    else
                    {
                        printf("Start POP3 on SSL Service Failed. \t\t[Error]\n");
                    }
                    close(pfd[0]);
                } 
            }

            sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stIMAPS]);
            if(!try_single_on(szFlag)  && CMailBase::m_enableimaps)    
            {
                printf("%s stopped.\n", SVR_DESP_TBL[stIMAPS]);   
                pipe(pfd);
                int imaps_pid = fork();
                if(imaps_pid == 0)
                {
                    close(pfd[0]);
                    if(check_single_on(szFlag)) 
                    {
                        printf("%s is aready runing.\n", SVR_DESP_TBL[stSMTP]);   
                        exit(-1);
                    }
                    Service imaps_svr(stIMAPS);
                    imaps_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_imapsport);
                    exit(0);
                }
                else if(imaps_pid > 0)
                {
                    unsigned int result;
                    close(pfd[1]);
                    read(pfd[0], &result, sizeof(unsigned int));
                    if(result == 0)
                        printf("Start IMAP on SSL Service OK \t\t[%u]\n", imaps_pid);
                    else
                    {
                        printf("Start IMAP on SSL Service Failed. \t\t[Error]\n");
                    }
                    close(pfd[0]);
                } 
            }
            sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stHTTPS]);
            if(!try_single_on(szFlag)  && CMailBase::m_enablehttps)    
            {
                printf("%s stopped.\n", SVR_DESP_TBL[stHTTPS]);   
                pipe(pfd);
                int https_pid = fork();
                if(https_pid == 0)
                {
                    close(pfd[0]);
                    if(check_single_on(szFlag)) 
                    {
                        printf("%s is aready runing.\n", SVR_DESP_TBL[stSMTP]);   
                        exit(-1);
                    }
                    Service https_svr(stHTTPS);
                    https_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_httpsport);
                    exit(0);
                }
                else if(https_pid > 0)
                {
                    unsigned int result;
                    close(pfd[1]);
                    read(pfd[0], &result, sizeof(unsigned int));
                    if(result == 0)
                        printf("Start HTTP on SSL Service OK \t\t[%u]\n", https_pid);
                    else
                    {
                        printf("Start HTTP on SSL Service Failed. \t\t[Error]\n");
                    }
                    close(pfd[0]);
                } 
            }
            
            sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stSPOOL]);
            if(!try_single_on(szFlag))   
            {
                printf("%s stopped.\n", SVR_DESP_TBL[stSPOOL]);   
                int spool_pids;
                pipe(pfd);
                spool_pids = fork();
                if(spool_pids == 0)
                {
                    close(pfd[0]);
                    if(check_single_on(szFlag)) 
                    {
                        printf("%s is aready runing.\n", SVR_DESP_TBL[stSMTP]);   
                        exit(-1);
                    }
                    Spool spool;
                    spool.Run(pfd[1]);
                    exit(0);
                }
                else if(spool_pids > 0)
                {
                    unsigned int result;
                    close(pfd[1]);
                    read(pfd[0], &result, sizeof(unsigned int));
                    if(result == 0)
                        printf("Start Relay Service OK \t\t\t[%u]\n", spool_pids);
                    else
                    {
                        printf("Start Relay Service Failed \t\t\t[Error]\n");
                    }
                    close(pfd[0]);
                } 
            }
		}
	}
	free(qBufPtr);
	if(m_watchdog_qid != (mqd_t)-1)
		mq_close(m_watchdog_qid);
	if(m_watchdog_sid != SEM_FAILED)
		sem_close(m_watchdog_sid);

	mq_unlink(strqueue.c_str());
	sem_unlink(strsem.c_str());
	
	return 0;
}
