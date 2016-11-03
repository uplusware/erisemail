/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _SMTP_H_
#define _SMTP_H_
#include <libmemcached/memcached.h>
#include <vector>
#include "letter.h"
#include "base.h"
#include "util/general.h"
#include "storage.h"
#include <string.h>
#include <string>
#include <dlfcn.h>

typedef enum
{
	jaTag = 0,
	jaDrop
} JunkAction;

typedef struct
{
	void* lhandle;
	void* dhandle;
	JunkAction action;
}FilterHandle;

using namespace std;

#define EXCHANGE_INTERVAL	5
#define EXCHANGE_MAXTIMES	10

typedef struct 
{
	MailLetter* letter;

	Letter_Info letter_info;
} LETTER_OBJ;

typedef enum
{
	SMTP_AUTH_LOGIN = 1,
	SMTP_AUTH_PLAIN,
	SMTP_AUTH_CRAM_MD5,
	SMTP_AUTH_DIGEST_MD5,
	SMTP_AUTH_NTLM,
	SMTP_AUTH_GSSAPI
}Smtp_Auth_Type;

class CMailSmtp : public CMailBase
{
public:
	CMailSmtp(int socket, SSL * ssl, SSL_CTX * ssl_ctx, const char* clientip,
        StorageEngine* storage_engine, memcached_st * memcached, BOOL isSSL = FALSE);
	virtual ~CMailSmtp();
	virtual BOOL Parse(char* text);
	virtual int ProtRecv(char* buf, int len);
	
	void On_Cancel_Auth_Handler(char* text);
	
	void On_Auth_Handler(char* text);
	
	BOOL On_Supose_Username_Handler(char* text);
	BOOL On_Supose_Password_Handler(char* text);
	BOOL On_Supose_Checking_Handler();
	BOOL On_Mail_Handler(char* text);
	BOOL On_Rcpt_Handler(char* text);
	void On_Data_Handler(char* text);
	BOOL On_Supose_Dataing_Handler(char* text);
	void On_Finish_Data_Handler();
	void On_Vrfy_Handler(char* text);
	void On_Expn_Handler(char* text);
	void On_Send_Handler(char* text);
	void On_Soml_Handler(char* text);
	void On_Saml_Handler(char* text);
	BOOL On_Helo_Handler(char* text);
	void On_Ehlo_Handler(char* text);
	void On_Quit_Handler(char* text);
	void On_Rset_Handler(char* text);
	void On_Help_Handler(char* text);
	void On_Noop_Handler(char* text);
	void On_Turn_Handler(char* text);
	void On_Unrecognized_Command_Handler();
	void On_Service_Unavailable_Handler();
	void On_Mailbox_Unavailable_Handler();
	void On_Local_Error_Handler();
	void On_Insufficient_Storage_Handler();
	void On_Unimplemented_Command_Handler();
	void On_Unimplemented_parameter_Handler();
	void On_Exceeded_Storage_Handler();
	void On_Transaction_Failed_Handler();
	void On_Service_Ready_Handler();
	void On_Service_Error_Handler();
	void On_Service_Close_Handler();
	void On_Request_Okay_Handler();
	void On_Command_Bad_Sequence_Handler();
	void On_Not_Local_User_Handler();
	void On_Exchange_Mail_Handler();
	void On_STARTTLS_Handler();
	
	int SmtpSend(const char* buf, int len);

	
private:
	vector<FilterHandle> m_filterHandle;
	BOOL m_isSSL;
    BOOL m_bSTARTTLS;
	SSL* m_ssl;
	SSL_CTX* m_ssl_ctx;
	
	int m_sockfd;
	DWORD m_status;
	string m_clientip;
	string m_clientdomain;
	string m_username;
	string m_password;
	string m_mailfrom;
	vector<LETTER_OBJ*> m_letter_obj_vector;
	
	string m_strDigest;
	Smtp_Auth_Type m_authType;
	string m_strToken;
	string m_strTokenVerify;

	linesock* m_lsockfd;
	linessl * m_lssl;

	
protected:
	BOOL Check_Helo_Domain(const char* domain);
	StorageEngine* m_storageEngine;
	memcached_st * m_memcached;

};

#endif /* _SMTP_H_ */

