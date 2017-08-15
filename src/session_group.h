/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _SESSION_GROUP_H_
#define _SESSION_GROUP_H_

#include <sys/epoll.h>  
#include "xmpp.h"

#include <map>
#include <string>
using namespace std;

#define MAX_EVENTS_NUM  65536*2
#define MAX_SOCKFD_NUM  65536*2

class Session_Info
{
public:
    Session_Info(Service_Type st, int epoll_fd, int sockfd, SSL * ssl, SSL_CTX * ssl_ctx, const char* clientip,
        StorageEngine* storage_engine, memcached_st * memcached, BOOL isSSL)
    {
        m_st = st;
        if(m_st == stXMPP)
            m_protocol = new CXmpp(epoll_fd, sockfd, ssl, ssl_ctx, clientip, storage_engine, memcached, isSSL);
        else
            m_protocol = NULL;
    }
    
    virtual ~Session_Info()
    {
        if(m_protocol)
            delete m_protocol;
    }
    
    CMailBase * GetProtocol()
    {
        return m_protocol;
    }
    
    Service_Type GetServiceType() { return m_st; }
    
    string& RecvBuf() { return m_recvbuf; }
private:
    Service_Type m_st;
    CMailBase * m_protocol;
    string m_recvbuf;
};

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