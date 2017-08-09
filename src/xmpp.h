/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _XMPP_H_
#define _XMPP_H_

#include <libmemcached/memcached.h>
#include "base.h"
#include "util/general.h"
#include "util/md5.h"
#include "service.h"
#include <string>

using namespace std;

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

class xmpp_stanza
{
public:
    xmpp_stanza()
    {
      xmpp_stanza("<?xml version='1.0' ?>");
    }

    xmpp_stanza(const char* declare)
    {
      m_xml_text = declare;
    }

    virtual ~xmpp_stanza()
    {

    }

    bool Parse(const char* text);
    
    void Append(const char* text) { m_xml_text += text; }

    const char* GetTag()
    {
      TiXmlElement * pStanzaElement = m_xml.RootElement();
      return pStanzaElement->Value();
    }

    const char* GetXmlText()
    {
      return m_xml_text.c_str();
    }

    const char* GetFrom()
    {
      return m_from.c_str();
    }

    void SetFrom(const char* from)
    {
      TiXmlElement * pStanzaElement = m_xml.RootElement();
      pStanzaElement->SetAttribute("from", from);
    }

    const char* GetTo()
    {
      return m_to.c_str();
    }

    TiXmlDocument* GetXml() { return &m_xml; }
private:
    string m_id;

    string m_from;
    string m_to;

    string m_lang;
    string m_xmlns; /* namespace */

    TiXmlDocument m_xml;
    string m_xml_text;
};

class xmpp_iq : public xmpp_stanza
{
public:
    xmpp_iq()
    {

    }
    virtual ~xmpp_iq()
    {

    }
private:
    iq_type m_type;
};

class xmpp_presence : public xmpp_stanza
{
public:
    xmpp_presence()
    {

    }
    virtual ~xmpp_presence()
    {

    }
private:
    presence_type m_type;
};

class xmpp_message : public xmpp_stanza
{
public:
    xmpp_message()
    {

    }
    virtual ~xmpp_message()
    {

    }
private:

};

class xmpp_auth : public xmpp_stanza
{
public:
    xmpp_auth()
    {

    }
    virtual ~xmpp_auth()
    {

    }
private:

};


class xmpp_stream_stream : public xmpp_stanza
{
public:
    xmpp_stream_stream()
    {

    }
    virtual ~xmpp_stream_stream()
    {

    }
private:

};

class xmpp_stream_features : public xmpp_stanza
{
public:
    xmpp_stream_features()
    {

    }
    virtual ~xmpp_stream_features()
    {

    }
private:

};

class xmpp_stream_error : public xmpp_stanza
{
public:
    xmpp_stream_error()
    {

    }
    virtual ~xmpp_stream_error()
    {

    }
private:

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
    BOOL PresenceTag(TiXmlDocument* xmlDoc);
    BOOL IqTag(TiXmlDocument* xmlDoc);
    BOOL StreamTag(TiXmlDocument* xmlDoc);
    BOOL MessageTag(TiXmlDocument* xmlDoc);
    BOOL AuthTag(TiXmlDocument* xmlDoc);

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
    xmpp_stanza * m_xmpp_stanza;
    string m_xml_declare;
    string m_xml_stream;
    char m_stream_id[33];
    unsigned int m_stream_count;
    int m_indent;
        
    string m_resource;
    BOOL m_auth_success;

    static map<string, CXmpp* > m_online_list;
    static pthread_rwlock_t m_online_list_lock;
    static BOOL m_online_list_inited;

    pthread_mutex_t m_send_lock;
};
#endif /* _XMPP_H_ */
