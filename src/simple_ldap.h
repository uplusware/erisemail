/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _SIMPLE_LDAP_H_
#define _SIMPLE_LDAP_H_

#include <libmemcached/memcached.h>
#include "base.h"
#include "letter.h"
#include "asn_1.h"
#include <string>
using namespace std;

class CLdap  : public CMailBase
{
public:
	CLdap(int sockfd, SSL * ssl, SSL_CTX * ssl_ctx, const char* clientip,
        StorageEngine* storage_engine, memcached_st * memcached, BOOL isSSL = FALSE);
	virtual ~CLdap();
	virtual BOOL Parse(char* text, int len);
	virtual int ProtRecv(char* buf, int len);
	virtual int ProtSend(const char* buf, int len) { return LdapSend(buf, len); };
	virtual char GetProtEndingChar() { return -1; };
    int LdapSend(const char* buf, int len);
    int LdapDecodedBuffer(const char* buf, int len);
    int SendDecodedBuffer();
    
protected:
	int m_sockfd;
	linesock* m_lsockfd;
	linessl * m_lssl;
	
	string m_clientip;
	DWORD m_status;

	BOOL m_isSSL;
    BOOL m_bSTARTTLS;
	SSL* m_ssl;
	SSL_CTX* m_ssl_ctx;
	
	string m_client;
	string m_username;
	string m_password;

	StorageEngine* m_storageEngine;
	memcached_st * m_memcached;
	
	unsigned char* m_buf;
	unsigned long long m_buf_len;
	unsigned long long m_data_len;
    
    unsigned char* m_decoded_buffer;
    unsigned int m_decoded_buffer_len;
    unsigned int m_decoded_data_len;
};
#endif /* _SIMPLE_LDAP_H_ */

