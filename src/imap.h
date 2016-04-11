#ifndef _IMAP_H_
#define _IMAP_H_
#include <libmemcached/memcached.h>

#include "base.h"
#include "util/general.h"
#include "storage.h"
#include <map>
#include "mime.h"
#include "letter.h"

using namespace std;

typedef enum
{	
	atLOGIN = 1,
	atKERBEROS_V4,
	atDIGEST_MD5,
	atCRAM_MD5
} Imap_Auth_Type;

typedef enum
{
	ANDing = 0,
	ORing
}Search_Relation;

typedef struct
{
	BOOL isDeep;
	string item;
} Search_Item;

class CMailImap : public CMailBase
{
public:
	CMailImap(int sockfd, const char* clientip, StorageEngine* storage_engine, memcached_st * memcached, BOOL isSSL = FALSE);
	virtual ~CMailImap();

	virtual BOOL Parse(char* text);
	virtual int ProtRecv(char* buf, int len);
	
	void On_Unknown(char* text);
	void On_Service_Ready();

	//Any state
	void On_Capability(char* text);
	void On_Noop(char* text);
	void On_Logout(char* text);

	//Non-Authenticated state
	BOOL On_Authenticate(char* text);
	BOOL On_Login(char* text);

	//Authenticated state
	void On_Select(char* text);
	void On_Examine(char* text);
	void On_Create(char* text);
	void On_Delete(char* text);
	void On_Rename(char* text);
	void On_Subscribe(char* text);
	void On_Unsubscribe(char* text);
	void On_List(char* text);
	void On_Listsub(char* text);
	void On_Status(char* text);
	void On_Append(char* text);

	//Select state
	void On_Check(char* text);
	void On_Close(char* text);
	void On_Expunge(char* text);
	void On_Search(char* text);
	void On_Fetch(char* text);
	void On_Store(char* text);
	void On_Copy(char* text);
	void On_UID(char* text);
	void On_STARTTLS(char* text);
	void On_X_atom(char* text);
	void On_Namespace(char* text);
	void On_Service_Error();
	int ImapSend(const char* buf, int len);
	int ImapRecv(char* buf, int len);
	
protected:
	vector<Mail_Info> m_maillisttbl;
	BOOL m_isSSL;
	SSL* m_ssl;
	SSL_CTX* m_ssl_ctx;
	
	int m_sockfd;
	linesock * m_lsockfd;
	linessl* m_lssl;
	string m_clientip;
	string m_username;
	DWORD m_status;
	Imap_Auth_Type m_authType;
	unsigned int m_challenge;
	string m_strSelectedDir;

	void BodyStruct(MimeSummary* part, string & strStruct, BOOL isEND = FALSE);

	void Fetch(const char* szArg, BOOL isUID = FALSE);
	void Store(const char* szArg, BOOL isUID = FALSE);
	int  Copy(const char* szArg, BOOL isUID = FALSE);
	void Search(const char * szTag, const char* szCmd, const char* szArg);
	void SearchRecv(const char* szTag, const char* szCmd, string & strsearch);
	void SubSearch(MailStorage* mailStg, const char* szArg, map<int,int>& vSearchResult);
	void ParseFetch(const char* text, vector<string>& vDst);
	void ParseCommand(const char* text, vector<string>& vDst);
	void ParseSearch(const char* text, vector<Search_Item>& vDst);

	StorageEngine* m_storageEngine;
	memcached_st * m_memcached;
};

#endif /* _IMAP_H_ */
