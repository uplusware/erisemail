#ifndef _POP_H_
#define _POP_H_
#include "base.h"
#include "letter.h"
#include <string>
using namespace std;

class CMailPop  : public CMailBase
{
public:
	CMailPop(int sockfd, const char* clientip, StorageEngine* storage_engine, BOOL isSSL = FALSE);
	virtual ~CMailPop();
	virtual BOOL Parse(char* text);
	virtual int ProtRecv(char* buf, int len);
	
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
	int PopSend(const char* buf, int len);
	
	
protected:
	int m_sockfd;
	linesock* m_lsockfd;
	linessl * m_lssl;
	
	string m_clientip;
	DWORD m_status;

	BOOL m_isSSL;
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

	StorageEngine* m_storageEngine;
};
#endif /* _POP_H_ */

