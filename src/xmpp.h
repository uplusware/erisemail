/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _XMPP_H_
#define _XMPP_H_

#include <libmemcached/memcached.h>
#include "base.h"
#include <string>
using namespace std;

enum xmpp_state_machine
{
   S_XMPP_INIT = 0,
   S_XMPP_AUTHING,
   S_XMPP_AUTHED,
   S_XMPP_IQING,
   S_XMPP_IQED,
   S_XMPP_PRESENCEING,
   S_XMPP_PRESENCED,
   S_XMPP_MESSAGEING,
   S_XMPP_MESSAGED,
   S_XMPP_TOP
};

enum iq_type
{
  IQ_GET = 0,
  IQ_SET,
  IQ_RESULT,
  IQ_UNKNOW
};

enum presence_type
{
  PRESENCE_AVAILABLE = 0,
  PRESENCE_SBSCRIBE,
  PRESENCE_SBSCRIBED,
  PRESENCE_UNSBSCRIBE,
  PRESENCE_UNSBSCRIBED,
  PRESENCE_UNAVAILABLE,
  PRESENCE_PROBE,
  PRESENCE_UNKNOW
};

class CXmpp : public CMailBase
{
public:
	CXmpp(int sockfd, SSL * ssl, SSL_CTX * ssl_ctx, const char* clientip,
        StorageEngine* storage_engine, memcached_st * memcached, BOOL isSSL = FALSE);
	virtual ~CXmpp();
    
    //virtual function
    virtual BOOL IsEnabledKeepAlive() { return FALSE; }
	virtual BOOL Parse(char* text);
	virtual int ProtRecv(char* buf, int len);
	
	int XmppSend(const char* buf, int len);
	    
    StorageEngine* GetStg() { return m_storageEngine; }
    
    const char* GetUsername() { return m_username.c_str(); }
    
    const char* GetResource() { return m_resource.c_str(); }
    
    const char* GetStreamID() { return m_stream_id; }
    
protected:
    BOOL PresenceTag(const char* text);
    BOOL IqTag(const char* text);
    BOOL StreamTag(const char* text);
    BOOL MessageTag(const char* text);
    BOOL AuthTag(const char* text);
    
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
	string m_strDigest;
	string m_strToken;
    string m_strTokenVerify;
	StorageEngine* m_storageEngine;
	memcached_st * m_memcached;
    
    //xmpp
    string m_xml_declare;
    string m_xml_stream;
    string m_xml_stanza;
    char m_stream_id[33];
    unsigned int m_stream_count;
    xmpp_state_machine m_state_machine;
    
    string m_resource;
    BOOL m_auth_success;
    
    static map<string, CXmpp* > m_online_list;
    static pthread_rwlock_t m_online_list_lock;
    static BOOL m_online_list_inited;
    
    pthread_mutex_t m_send_lock;
};
#endif /* _XMPP_H_ */

