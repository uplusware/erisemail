/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "session.h"

Session::Session(int sockfd, SSL *ssl, SSL_CTX * ssl_ctx, const char* clientip, Service_Type st, BOOL is_ssl,
    StorageEngine* storage_engine, memory_cache* ch, memcached_st * memcached)
{
	m_sockfd = sockfd;
    m_ssl = ssl;
    m_ssl_ctx = ssl_ctx;

	m_clientip = clientip;
	m_st = st;
    m_is_ssl = is_ssl;
	m_cache = ch;

	m_storageEngine = storage_engine;
	m_memcached = memcached_clone(NULL, memcached);
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
        CMailBase * pProtocol;
        try{
            if(m_st == stSMTP)
            {
                pProtocol = new CMailSmtp(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_memcached, m_is_ssl);
            }
            else if(m_st == stPOP3)
            {
                pProtocol = new CMailPop(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_memcached, m_is_ssl);
            }
            else if(m_st == stIMAP)
            {
                pProtocol = new CMailImap(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_memcached, m_is_ssl);
            }
            else if(m_st == stHTTP)
            {
                pProtocol = new CHttp(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_cache, m_memcached, m_is_ssl);
            }
            else if(m_st == stXMPP)
            {
                pProtocol = new CXmpp(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_memcached, m_is_ssl);
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

        char szmsg[4096];
        string str_line = "";
        std::size_t new_line;
        int result;
        while(1)
        {
            try
            {
                result = pProtocol->ProtRecv(szmsg, 4095);
                if(result <= 0)
                {
                    break;
                }
                else
                {
                    szmsg[result] = '\0';

                    str_line += szmsg;

                    if(m_st == stXMPP)
                    {
                        new_line = str_line.find(">");
                    }
                    else
                    {
                        new_line = str_line.find('\n');
                    }

                    if(new_line != std::string::npos)
                    {
                        if(!pProtocol->Parse((char*)str_line.c_str()))
                            break;
                        str_line = "";
                    }
                }
            }
            catch(string* e)
            {
                fprintf(stderr, "Exception: %s, %s:%d\n", e->c_str(), __FILE__, __LINE__);
                delete e;
                break;
            }

        }
        keep_alive = (pProtocol->IsKeepAlive() && pProtocol->IsEnabledKeepAlive()) ? TRUE : FALSE;
        delete pProtocol;

        if(!keep_alive)
            break;
    }
}
