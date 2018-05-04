/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _POP_H_
#define _POP_H_

#include <libmemcached/memcached.h>
#include "base.h"
#include "letter.h"
#include <string>
using namespace std;

typedef enum
{
	POP_AUTH_USER = 1,
	POP_AUTH_PLAIN,
	POP_AUTH_CRAM_MD5,
	POP_AUTH_DIGEST_MD5,
	POP_AUTH_NTLM,
	POP_AUTH_GSSAPI,
    POP_AUTH_EXTERNAL
}Pop3_Auth_Type;

class CMailPop  : public CMailBase
{
public:
	CMailPop(int sockfd, SSL * ssl, SSL_CTX * ssl_ctx, const char* clientip,
        StorageEngine* storage_engine, memcached_st * memcached, BOOL isSSL = FALSE);
	virtual ~CMailPop();
	virtual BOOL Parse(char* text, int len);
	virtual int ProtRecv(char* buf, int len);
	virtual int ProtSend(const char* buf, int len) { return PopSend(buf, len); };
    
	void On_Service_Ready_Handler();
	void On_Service_Error_Handler();
	void On_Unrecognized_Command_Handler();
	void On_Stat_Handler(char* text);
	void On_Apop_Handler(char* text);
	void On_List_Handler(char* text);
	void On_Retr_Handler(char* text);
	void On_Uidl_Handler(char* text);
	void On_Delete_Handler(char* text);
	BOOL On_Supose_Username_Handler(char* text);
	BOOL On_Supose_Password_Handler(char* text);
	BOOL On_Supose_Checking_Handler();
	void On_Not_Valid_State_Handler();
	void On_Quit_Handler();
	void On_Rset_Handler(char* text);
	void On_Capa_Handler(char* text);
	void On_Top_Handler(char* text);
	void On_STLS_Handler();
    void On_Auth_Handler(char* text);
	int PopSend(const char* buf, int len);
	
	virtual BOOL IsEnabledKeepAlive() { return FALSE; }
    
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
	
	vector<Mail_Info> m_mailTbl;
	unsigned int m_lettersCount;
	unsigned int m_lettersTotalSize;
	string m_client;
	string m_username;
	string m_password;
	string m_strDigest;
	string m_strToken;
    string m_strTokenVerify;
	StorageEngine* m_storageEngine;
	memcached_st * m_memcached;
    
    Pop3_Auth_Type m_authType;

};
#endif /* _POP_H_ */

