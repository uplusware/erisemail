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
#include <sys/epoll.h>  

using namespace std;

void __inline__ _strtrim_dquote_(string &src) /* icnluding double quote mark*/
{
	string::size_type ops1, ops2, ops3, ops4, ops5;
	bool bExit = false;
	while(!bExit)
	{
		bExit = true;
		ops1 = src.find_first_not_of(" ");
		if((ops1 != string::npos)&&(ops1 != 0))
		{
			src.erase(0, ops1);
			bExit = false;
		}
		else if(ops1 == string::npos)
		{
			src.erase(0, src.length());
		}
		ops2 = src.find_first_not_of("\t");
		if((ops2 != string::npos)&&(ops2 != 0))
		{
			src.erase(0, ops2);
		}
		else if(ops2 == string::npos)
		{
			src.erase(0, src.length());
		}
		ops3 = src.find_first_not_of("\r");
		if((ops3 != string::npos)&&(ops3 != 0))
		{
			src.erase(0, ops3);
		}
		else if(ops3 == string::npos)
		{
			src.erase(0, src.length());
		}
			
		ops4 = src.find_first_not_of("\n");
		if((ops4 != string::npos)&&(ops4 != 0))
		{
			src.erase(0, ops4);
		}
		else if(ops4 == string::npos)
		{
			src.erase(0, src.length());
		}
		
		ops5 = src.find_first_not_of("\"");
		if((ops5 != string::npos)&&(ops5 != 0))
		{
			src.erase(0, ops5);
		}
		else if(ops5 == string::npos)
		{
			src.erase(0, src.length());
		}
	}
	bExit = false;
	while(!bExit)
	{
		bExit = true;
		ops1 = src.find_last_not_of(" ");
		if((ops1 != string::npos)&&(ops1 != src.length() - 1 ))
		{
			src.erase(ops1 + 1, src.length() - ops1 - 1);
			bExit = false;
		}
		
		ops2 = src.find_last_not_of("\t");
		if((ops2 != string::npos)&&(ops2 != src.length() - 1 ))
		{
			src.erase(ops2 + 1, src.length() - ops2 - 1);
			bExit = false;
		}
		ops3 = src.find_last_not_of("\r");
		if((ops3 != string::npos)&&(ops3 != src.length() - 1 ))
		{
			src.erase(ops3 + 1, src.length() - ops3 - 1);
			bExit = false;
		}
			
		ops4 = src.find_last_not_of("\n");
		if((ops4 != string::npos)&&(ops4 != src.length() - 1 ))
		{
			src.erase(ops4 + 1, src.length() - ops4 - 1);
			bExit = false;
		}
		ops5 = src.find_last_not_of("\"");
		if((ops5 != string::npos)&&(ops5 != src.length() - 1 ))
		{
			src.erase(ops5 + 1, src.length() - ops5 - 1);
			bExit = false;
		}
	}
}

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

enum auth_method
{
  AUTH_METHOD_PLAIN = 0,
  AUTH_METHOD_DIGEST_MD5,
  AUTH_METHOD_GSSAPI,
  AUTH_METHOD_EXTERNAL,
  AUTH_METHOD_UNKNOWN
};

enum auth_steps
{
    AUTH_STEP_INIT = 0,
    AUTH_STEP_1,
    AUTH_STEP_2, /* DIGEST-MD5 */
    AUTH_STEP_3,
    AUTH_STEP_SUCCESS = 99
};

#define XMPP_MAX_STANZA_LENGTH  (1024*8)

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
    
    void Append(const char* text)
    {
        m_xml_text += text;
    }

    const char* GetTag()
    {
      TiXmlElement * pStanzaElement = m_xml.RootElement();
      return pStanzaElement->Value();
    }

    const char* GetXmlText()
    {
      return m_xml_text.c_str();
    }
    
    int GetXmlTextLength()
    {
      return m_xml_text.length();
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

class Xmpp_Session_Info;

enum TLSState
{
    TLSNone = 0,
    TLSWantConnect,
    TLSWantAccept,
    TLSEstablished
};

class CXmpp : public CMailBase
{
public:
	CXmpp(Xmpp_Session_Info* sess_inf, int epoll_fd, int sockfd, SSL * ssl, SSL_CTX * ssl_ctx, const char* clientip, unsigned short client_port, StorageEngine* storage_engine, memcached_st * memcached,
        BOOL isSSL = FALSE, BOOL isConnectionInitiator= FALSE, const char* pdn = "", BOOL isDialBackStream = FALSE, CXmpp* pDependency = NULL);
	virtual ~CXmpp();

    //virtual function
    virtual BOOL IsEnabledKeepAlive() { return FALSE; }
	virtual BOOL Parse(char* text, int len);
	virtual int ProtRecv(char* buf, int len);
    virtual int ProtSend(const char* buf, int len) { return XmppSend(buf, len); };
    virtual int ProtRecvNoWait(char* buf, int len);
    virtual int ProtSendNoWait(const char* buf, int len);
    virtual int ProtTryFlush();
    virtual BOOL TLSContinue();
    virtual BOOL ProtSendBufEmpty() { return m_send_buf.length() == 0 ? TRUE : FALSE; }
    virtual char GetProtEndingChar() { return '>'; };
    
	int XmppSend(const char* buf, int len);

    StorageEngine* GetStg() { return m_storageEngine; }

    const char* GetUsername() { return m_username.c_str(); }

    const char* GetResource() { return m_resource.c_str(); }

    const char* GetStreamID() { return m_stream_id; }
    
    Xmpp_Session_Info* GetSessionInfo() { return m_sess_inf; }
    
protected:
    BOOL ResponseTag(TiXmlDocument* xmlDoc);
    BOOL StartTLSTag(TiXmlDocument* xmlDoc);
    BOOL PresenceTag(TiXmlDocument* xmlDoc);
    BOOL IqTag(TiXmlDocument* xmlDoc);
    BOOL Stream_StreamTag(TiXmlDocument* xmlDoc);
    BOOL Stream_FeaturesTag(TiXmlDocument* xmlDoc);
    BOOL MessageTag(TiXmlDocument* xmlDoc);
    BOOL AuthTag(TiXmlDocument* xmlDoc);
    BOOL SuccessTag(TiXmlDocument* xmlDoc);
    BOOL DbResultTag(TiXmlDocument* xmlDoc);
    BOOL DbVerifyTag(TiXmlDocument* xmlDoc);
	BOOL ProceedTag(TiXmlDocument* xmlDoc);
    BOOL UnknownTag(TiXmlDocument* xmlDoc);

    BOOL SendOfflineMessage();
    
    BOOL TLSAccept(const char* ca_crt_root,
        const char* ca_crt_server,
        const char* ca_password,
        const char* ca_key_server);
    
    BOOL TLSConnect(const char* ca_crt_root,
        const char* ca_crt_client,
        const char* ca_password,
        const char* ca_key_client);
    
protected:
    Xmpp_Session_Info* m_sess_inf;
    int m_epoll_fd;
	int m_sockfd;
	linesock* m_lsockfd;
	linessl * m_lssl;
    
    string m_send_buf;
    
	string m_clientip;
    unsigned short m_client_port;
	DWORD m_status;

	BOOL m_isSSL;
    BOOL m_bSTARTTLS;
	SSL* m_ssl;
	SSL_CTX* m_ssl_ctx;
    
    TLSState m_TLSState;
    
    auth_method m_auth_method;
    auth_steps m_auth_step;
    
	string m_client;
	string m_username;
	string m_password;
    
    //DIGEST-MD5
	string m_strDigitalMD5Nonce;
    string m_strDigitalMD5Response;
    
    //server to server session
    BOOL m_is_connection_initiator;
    BOOL m_is_srv2srv_conn;
    string m_peer_srv_domain_name;
    BOOL m_is_dialback_stream;
    CXmpp * m_dependency;
    
#ifdef _WITH_GSSAPI_
    OM_uint32 m_maj_stat;
    OM_uint32 m_min_stat;        
    gss_cred_id_t m_server_creds; //default GSS_C_NO_CREDENTIAL;
    gss_name_t m_server_name; //default GSS_C_NO_NAME;
    gss_ctx_id_t m_context_hdl; // default GSS_C_NO_CONTEXT;
    gss_name_t m_client_name; // default GSS_C_NO_NAME;
#endif /* _WITH_GSSAPI_ */

	StorageEngine* m_storageEngine;
	memcached_st * m_memcached;

    //xmpp
    xmpp_stanza * m_xmpp_stanza;
    string m_xml_declare;
    string m_xml_stream;
    char m_stream_id[512];
    unsigned int m_stream_count;
    int m_indent;
        
    string m_resource;

    static map<string, CXmpp* > m_online_list;
	static map<string, CXmpp* > m_srv2srv_list;
    static pthread_rwlock_t m_online_list_lock;
	static pthread_rwlock_t m_srv2srv_list_lock;
    static BOOL m_online_list_inited;
	static BOOL m_srv2srv_list_inited;

    pthread_mutex_t m_send_lock;
};
#endif /* _XMPP_H_ */
