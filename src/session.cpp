/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "session.h"

Session::Session(int sockfd, SSL *ssl, SSL_CTX * ssl_ctx, const char* clientip, unsigned short client_port, Service_Type st, BOOL is_ssl,
    StorageEngine* storage_engine, memory_cache* ch, memcached_st * memcached)
{
	m_sockfd = sockfd;
    m_ssl = ssl;
    m_ssl_ctx = ssl_ctx;

	m_clientip = clientip;
    m_client_port = client_port;
    
	m_st = st;
    m_is_ssl = is_ssl;
	m_cache = ch;

	m_storageEngine = storage_engine;
	m_memcached = memcached_clone(NULL, memcached);
    
    if(m_is_ssl && (!m_ssl || !m_ssl_ctx))
    {
        if(!create_ssl(m_sockfd, 
                CMailBase::m_ca_crt_root.c_str(),
                CMailBase::m_ca_crt_server.c_str(),
                CMailBase::m_ca_password.c_str(),
                CMailBase::m_ca_key_server.c_str(),
                CMailBase::m_enableclientcacheck,
                &m_ssl, &m_ssl_ctx))
        {
            close(m_sockfd);
            throw new string("Create SSL session error.");
        }
    }
                
}

Session::~Session()
{
    if(m_memcached)
        memcached_free(m_memcached);
    m_memcached = NULL;
}

void Session::Process()
{
    BOOL keep_alive = FALSE;

    while(1)
    {
        CMailBase * pProtocolInstance;
        try{
            if(m_st == stSMTP)
            {
                pProtocolInstance = new CMailSmtp(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_memcached, m_is_ssl);
            }
            else if(m_st == stPOP3)
            {
                pProtocolInstance = new CMailPop(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_memcached, m_is_ssl);
            }
            else if(m_st == stIMAP)
            {
                pProtocolInstance = new CMailImap(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_memcached, m_is_ssl);
            }
            else if(m_st == stHTTP)
            {
                pProtocolInstance = new CHttp(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_cache, m_memcached, m_is_ssl);
            }
            else
            {
                return;
            }
        }
        catch(string* e)
        {
            fprintf(stderr, "Exception: %s, %s:%d\n", e->c_str(), __FILE__, __LINE__);
            delete e;
            close(m_sockfd);
            m_sockfd = -1;
            return;
        }

        char szmsg[4096 + 1024 + 1];
        string str_line = "";
        std::size_t new_line;
        int result;
        
        bool bQuitSession = false;
        while(1)
        {
            try
            {
                result = pProtocolInstance->ProtRecv(szmsg, 4096 + 1024);
                if(result <= 0)
                {
                    bQuitSession = true;
                    break;
                }
                else
                {
                    szmsg[result] = '\0';
                    
                    str_line += szmsg;
                    
                    do 
                    {
                        new_line = str_line.find(pProtocolInstance->GetProtEndingChar());
                        
                        string str_left;
        
                        if(new_line != std::string::npos)
                        {
                            str_left = str_line.substr(0, new_line + 1);
                            str_line = str_line.substr(new_line + 1, str_line.length() - 1 - new_line);
                            if(!pProtocolInstance->Parse((char*)str_left.c_str()))
                            {
                                bQuitSession = true;
                                break;
                            }
                        }
                    } while(new_line != std::string::npos);
                }
            }
            catch(string* e)
            {
                fprintf(stderr, "Exception: %s, %s:%d\n", e->c_str(), __FILE__, __LINE__);
                delete e;
                break;
            }
            
            if(bQuitSession)
                break;
        }
        keep_alive = (pProtocolInstance->IsKeepAlive() && pProtocolInstance->IsEnabledKeepAlive()) ? TRUE : FALSE;
        delete pProtocolInstance;

        if(!keep_alive)
            break;
    }
}
