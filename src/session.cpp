#include "session.h"

Session::Session(int sockfd, SSL *ssl, SSL_CTX * ssl_ctx,
    const char* clientip, Service_Type st, StorageEngine* storage_engine, memory_cache* ch, memcached_st * memcached)
{
	m_sockfd = sockfd;
    m_ssl = ssl;
    m_ssl_ctx = ssl_ctx;

	m_clientip = clientip;
	m_st = st;

	m_cache = ch;

	m_storageEngine = storage_engine;
	m_memcached = memcached;
}

Session::~Session()
{

}

void Session::Process()
{
	CMailBase * pProtocol;
	try{
	    if(m_st == stSMTP)
	    {
		    pProtocol = new CMailSmtp(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_memcached);
	    }
	    else if(m_st == stPOP3)
	    {
		    pProtocol = new CMailPop(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_memcached);
	    }
	    else if(m_st == stIMAP)
	    {
		    pProtocol = new CMailImap(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_memcached);
	    }
        else if(m_st == stHTTP)
	    {
		    pProtocol = new CHttp(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_cache, m_memcached);
	    }
	    else if(m_st == stSMTPS)
	    {
		    pProtocol = new CMailSmtp(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_memcached, TRUE);
	    }
	    else if(m_st == stPOP3S)
	    {
		    pProtocol = new CMailPop(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_memcached, TRUE);
	    }
	    else if(m_st == stIMAPS)
	    {
		    pProtocol = new CMailImap(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_memcached, TRUE);
	    }	    
	    else if(m_st == stHTTPS)
	    {
		    pProtocol = new CHttp(m_sockfd, m_ssl, m_ssl_ctx, m_clientip.c_str(), m_storageEngine, m_cache, m_memcached, TRUE);
	    }
	    else
	    {
		    return;
	    }
	}
	catch(string* e)
	{
		printf("Exception1: %s\n", e->c_str());
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
                new_line = str_line.find('\n');
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
	        printf("Exception2: %s\n", e->c_str());
            delete e;
	        break;
        }

	}
	delete pProtocol;
}

