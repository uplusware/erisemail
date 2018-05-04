/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _SESSION_H_
#define _SESSION_H_
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include "base.h"
#include "smtp.h"
#include "pop.h"
#include "imap.h"
#include "http.h"
#include "xmpp.h"
#include "simple_ldap.h"
#include "cache.h"

class Session
{
protected:
	int m_sockfd;
	string m_clientip;
    unsigned short m_client_port;
	Service_Type m_st;
    BOOL m_is_ssl;
    SSL * m_ssl;
    SSL_CTX * m_ssl_ctx;
	StorageEngine* m_storageEngine;
	memcached_st * m_memcached;
public:
	Session(int sockfd, SSL *ssl, SSL_CTX * ssl_ctx, const char* clientip, unsigned short client_port, Service_Type st, BOOL is_ssl,
        StorageEngine* storage_engine, memory_cache* ch, memcached_st * memcached);
	virtual ~Session();
	
	void Process();

	memory_cache* m_cache;
};
#endif /* _SESSION_H_*/

