/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _SESSION_GROUP_H_
#define _SESSION_GROUP_H_

#include <map>
#include <sys/epoll.h>  
#include "xmpp.h"

using namespace std;

#define MAX_EVENTS_NUM  655360
#define MAX_SOCKFD_NUM  655360


typedef struct{
    Service_Type st;
    CMailBase * protocol;
    string recvbuf;
    string sendbuf;
} Session_Info;

class Session_Group
{
public:
    Session_Group();
    virtual ~Session_Group();
    BOOL Accept(int sockfd, SSL *ssl, SSL_CTX * ssl_ctx, const char* clientip, Service_Type st, BOOL is_ssl,
        StorageEngine* storage_engine, memory_cache* ch, memcached_st * memcached);
    BOOL Poll();
    
private:
    map<int, Session_Info*> m_session_list;
    int m_epoll_fd;
    struct epoll_event * m_events;
};

#endif /* _SESSION_GROUP_H_ */