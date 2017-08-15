/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "session_group.h"

Session_Group::Session_Group()
{
    m_epoll_fd = epoll_create1(0);
    if (m_epoll_fd == -1)  
    {
        fprintf(stderr, "%s %u# epoll_create1: %s\n", __FILE__, __LINE__, strerror(errno));
        throw new string(strerror(errno));
    }
    
    m_events = new struct epoll_event[MAX_EVENTS_NUM]; 
    
}

Session_Group::~Session_Group()
{
    map<int, Session_Info *>::iterator iter_session;
    for(iter_session = m_session_list.begin(); iter_session != m_session_list.end(); iter_session++)
    {
        if(iter_session->second)
        {
            delete iter_session->second;
        }
    }
    if(m_events)
        delete[] m_events;
}

BOOL Session_Group::Accept(int sockfd, SSL *ssl, SSL_CTX * ssl_ctx, const char* clientip, Service_Type st, BOOL is_ssl,
        StorageEngine* storage_engine, memory_cache* ch, memcached_st * memcached)
{
    Session_Info * pSessionInstance = new Session_Info(st, m_epoll_fd, sockfd, ssl, ssl_ctx, clientip, storage_engine, memcached, is_ssl);
    if(!pSessionInstance)
        return FALSE;
    
    if(!pSessionInstance->GetProtocol())
        return FALSE;
    
    struct epoll_event event;
    event.data.fd = sockfd;  
    event.events = EPOLLIN /* | EPOLLOUT */ |  EPOLLHUP | EPOLLERR;  
    if (epoll_ctl (m_epoll_fd, EPOLL_CTL_ADD, sockfd, &event) == -1)  
    {  
        fprintf(stderr, "%s %u# epoll_ctl: %s\n", __FILE__, __LINE__, strerror(errno));
        delete pSessionInstance;
        return FALSE;
    }
    
    if(m_session_list.find(sockfd) != m_session_list.end() && m_session_list[sockfd])
    {
        delete m_session_list[sockfd];
    }
    
    m_session_list[sockfd] = pSessionInstance;
    
    return TRUE;
}

BOOL Session_Group::Poll()
{
    int n, i;  
  
    n = epoll_wait (m_epoll_fd, m_events, MAX_EVENTS_NUM, 1000);

    for (i = 0; i < n; i++)  
    {
        if (m_events[i].events & EPOLLIN)
        {
            if(m_session_list.find(m_events[i].data.fd) != m_session_list.end() && m_session_list[m_events[i].data.fd])
            {
                Session_Info * pSessionInstance = m_session_list[m_events[i].data.fd];

                char szmsg[4096 + 1024 + 1];
                
                std::size_t new_line;
                int len = pSessionInstance->GetProtocol()->ProtRecv2(szmsg, 4096 + 1024);
                if( len > 0)
                {
                    szmsg[len] = '\0';
                    
                    pSessionInstance->RecvBuf() += szmsg;
                    
                    do
                    {
                        new_line = pSessionInstance->RecvBuf().find(pSessionInstance->GetProtocol()->ProtEndingChr());
                        
                        string str_left;
        
                        if(new_line != std::string::npos)
                        {
                            str_left = pSessionInstance->RecvBuf().substr(0, new_line + 1);
                            pSessionInstance->RecvBuf() = pSessionInstance->RecvBuf().substr(new_line + 1, pSessionInstance->RecvBuf().length() - 1 - new_line);
                            if(!pSessionInstance->GetProtocol()->Parse((char*)str_left.c_str()))
                            {
                                break;
                            }
                        }
                    }while(new_line != std::string::npos);
                }
                else if( len < 0)
                {
                    struct epoll_event ev;
                    ev.events = EPOLLOUT |  EPOLLIN | EPOLLHUP | EPOLLERR;
                    ev.data.fd = m_events[i].data.fd;
                    epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_events[i].data.fd, &ev);
                                    
                    delete m_session_list[m_events[i].data.fd];
                    m_session_list.erase(m_events[i].data.fd);
                }
            }
        }
        else if (m_events[i].events & EPOLLOUT)
        {
            if(m_session_list.find(m_events[i].data.fd) != m_session_list.end() && m_session_list[m_events[i].data.fd])
            {
                Session_Info * pSessionInstance = m_session_list[m_events[i].data.fd];
                pSessionInstance->GetProtocol()->ProtFlush();
            }
        }
        else if (m_events[i].events & EPOLLHUP || m_events[i].events & EPOLLERR)
        {
            struct epoll_event ev;
            ev.events = EPOLLOUT |  EPOLLIN | EPOLLHUP | EPOLLERR;
            ev.data.fd = m_events[i].data.fd;
            epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_events[i].data.fd, &ev);
                            
            if(m_session_list.find(m_events[i].data.fd) != m_session_list.end() && m_session_list[m_events[i].data.fd])
            {
                delete m_session_list[m_events[i].data.fd];
                m_session_list.erase(m_events[i].data.fd);
            }
        }
    }
    
    return TRUE;
}