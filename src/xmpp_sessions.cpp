/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "xmpp_sessions.h"

Xmpp_Session_Group::Xmpp_Session_Group()
{
    m_epoll_fd = epoll_create1(0);
    if (m_epoll_fd == -1)  
    {
        fprintf(stderr, "%s %u# epoll_create1: %s\n", __FILE__, __LINE__, strerror(errno));
        throw new string(strerror(errno));
    }
    
    m_events = new struct epoll_event[MAX_EVENTS_NUM]; 
    
}

Xmpp_Session_Group::~Xmpp_Session_Group()
{
    map<int, Xmpp_Session_Info *>::iterator iter_session;
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

BOOL Xmpp_Session_Group::Accept(int sockfd, SSL *ssl, SSL_CTX * ssl_ctx, const char* clientip, Service_Type st, BOOL is_ssl,
        StorageEngine* storage_engine, memory_cache* ch, memcached_st * memcached)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
  	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
        
    Xmpp_Session_Info * pSessionInstance = new Xmpp_Session_Info(this, st, m_epoll_fd, sockfd, ssl, ssl_ctx, clientip, storage_engine, memcached, is_ssl);
    if(!pSessionInstance)
    {
        close(sockfd);
        return FALSE;
    }
    
    struct epoll_event event;
    event.data.fd = sockfd;  
    event.events = EPOLLIN /* | EPOLLOUT */ |  EPOLLHUP | EPOLLERR;  
    if (epoll_ctl (m_epoll_fd, EPOLL_CTL_ADD, sockfd, &event) == -1)  
    {  
        fprintf(stderr, "%s %u# epoll_ctl: %s\n", __FILE__, __LINE__, strerror(errno));
        delete pSessionInstance;
        close(sockfd);
        return FALSE;
    }
    
    if(!pSessionInstance->CreateProtocol())
    {
        delete pSessionInstance;
        close(sockfd);
        return FALSE;
    }
    
    if(m_session_list.find(sockfd) != m_session_list.end() && m_session_list[sockfd])
    {
        delete m_session_list[sockfd];
    }
    
    m_session_list[sockfd] = pSessionInstance;
    
    return TRUE;
}

BOOL Xmpp_Session_Group::Connect(const char* hostname, unsigned short port, Service_Type st, StorageEngine* storage_engine, memory_cache* ch, memcached_st * memcached,
    std::stack<string>& xmpp_stanza_stack, BOOL isXmppDialBack, CXmpp* pDependency)
{
    char szPort[16];
    sprintf(szPort, "%d", port);
    
     /* Get the IP from the name */
	char realip[INET6_ADDRSTRLEN];
	struct addrinfo hints;      
    struct addrinfo *servinfo, *curr;  
    struct sockaddr_in *sa;
    struct sockaddr_in6 *sa6;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_CANONNAME; 
    
    //printf("getaddrinfo: %s\n", hostname);
    if (getaddrinfo(hostname, NULL, &hints, &servinfo) != 0)
    {
        return FALSE;
    }
    
    BOOL bFound = FALSE;
    curr = servinfo; 
    while (curr && curr->ai_canonname)
    {  
        if(servinfo->ai_family == AF_INET6)
        {
            sa6 = (struct sockaddr_in6 *)curr->ai_addr;  
            inet_ntop(AF_INET6, (void*)&sa6->sin6_addr, realip, sizeof (realip));
            bFound = TRUE;
        }
        else if(servinfo->ai_family == AF_INET)
        {
            sa = (struct sockaddr_in *)curr->ai_addr;  
            inet_ntop(AF_INET, (void*)&sa->sin_addr, realip, sizeof (realip));
            bFound = TRUE;
        }
        curr = curr->ai_next;
    }     

    freeaddrinfo(servinfo);

    if(bFound == FALSE)
    {
        fprintf(stderr, "couldn't find ip for %s\n", hostname);
        return FALSE;
    }
    
    int sockfd = -1;
	
	/* struct addrinfo hints; */
    struct addrinfo *server_addr, *rp;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
                
    int s = getaddrinfo((realip && realip[0] != '\0') ? realip : NULL, szPort, &hints, &server_addr);
    if (s != 0)
    {
       fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
       return FALSE;
    }
    
    BOOL isConnectOK = FALSE;
    for (rp = server_addr; rp != NULL; rp = rp->ai_next)
    {
       sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
       if (sockfd == -1)
           continue;
       
	    int flags = fcntl(sockfd, F_GETFL, 0); 
	    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
        
        int rtVal = connect(sockfd, rp->ai_addr, rp->ai_addrlen);
        if( rtVal == 0 || (rtVal < 0 && errno == EINPROGRESS))
        {
            isConnectOK = TRUE;
            break;
        }
    }
    
    freeaddrinfo(server_addr);  
    
    if(isConnectOK == TRUE)
    {
        Xmpp_Session_Info * pSessionInstance = new Xmpp_Session_Info(this, st, m_epoll_fd, sockfd, NULL, NULL, realip, storage_engine, memcached, FALSE, TRUE, hostname, isXmppDialBack, pDependency);
        if(!pSessionInstance)
        {
            close(sockfd);
            return FALSE;
        }
        
		pSessionInstance->SetStanzaStack(xmpp_stanza_stack);
		
        struct epoll_event event;
        event.data.fd = sockfd;  
        event.events = EPOLLIN | EPOLLOUT |  EPOLLHUP | EPOLLERR;  
        if (epoll_ctl (m_epoll_fd, EPOLL_CTL_ADD, sockfd, &event) == -1)  
        {  
            fprintf(stderr, "%s %u# epoll_ctl: %s\n", __FILE__, __LINE__, strerror(errno));
            delete pSessionInstance;
            close(sockfd);
            return FALSE;
        }
        
        if(m_session_list.find(sockfd) != m_session_list.end() && m_session_list[sockfd])
        {
            delete m_session_list[sockfd];
        }
        
        m_session_list[sockfd] = pSessionInstance;
        
        return TRUE;
    }
    
    return TRUE;
}

BOOL Xmpp_Session_Group::Poll()
{
    int n, i;  
  
    n = epoll_wait (m_epoll_fd, m_events, MAX_EVENTS_NUM, 1000);

    for (i = 0; i < n; i++)  
    {
        if (m_events[i].events & EPOLLIN)
        {
            if(m_session_list.find(m_events[i].data.fd) != m_session_list.end() && m_session_list[m_events[i].data.fd])
            {
                Xmpp_Session_Info * pSessionInstance = m_session_list[m_events[i].data.fd];
                
                pSessionInstance->GetProtocol()->TLSContinue();

                char szmsg[4096 + 1024 + 1];
                szmsg[0] = '\0';
                std::size_t new_line;
                int len = pSessionInstance->GetProtocol()->ProtRecvNoWait(szmsg, 4096 + 1024);
                if( len > 0)
                {
                    szmsg[len] = '\0';
                    
                    pSessionInstance->RecvBuf() += szmsg;
                    
                    do
                    {
                        new_line = pSessionInstance->RecvBuf().find(pSessionInstance->GetProtocol()->GetProtEndingChar());
                        
                        string str_left;
        
                        if(new_line != std::string::npos)
                        {
                            str_left = pSessionInstance->RecvBuf().substr(0, new_line + 1);
                            pSessionInstance->RecvBuf() = pSessionInstance->RecvBuf().substr(new_line + 1, pSessionInstance->RecvBuf().length() - 1 - new_line);
                            if(!pSessionInstance->GetProtocol()->Parse((char*)str_left.c_str()))
                            {
                                // destory the xmpp session
                                struct epoll_event ev;
                                ev.events = EPOLLOUT |  EPOLLIN | EPOLLHUP | EPOLLERR;
                                ev.data.fd = m_events[i].data.fd;
                                epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_events[i].data.fd, &ev);
                                                
                                if(m_session_list.find(m_events[i].data.fd) != m_session_list.end() && m_session_list[m_events[i].data.fd])
                                {
                                    delete m_session_list[m_events[i].data.fd];
                                    m_session_list.erase(m_events[i].data.fd);
                                }
            
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
                Xmpp_Session_Info * pSessionInstance = m_session_list[m_events[i].data.fd];
                
                if(!pSessionInstance->GetProtocol())
                {
                    if(!pSessionInstance->CreateProtocol())
                        return FALSE;
                }
                
                pSessionInstance->GetProtocol()->TLSContinue();
                
                pSessionInstance->GetProtocol()->ProtTryFlush();
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