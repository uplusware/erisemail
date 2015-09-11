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
#include "cache.h"

typedef enum
{
	stSMTP = 1,
	stPOP3,
	stIMAP,
	stHTTP,
	stSMTPS,
	stPOP3S,
	stIMAPS,
	stHTTPS,
	stSPOOL
} Service_Type;

class Session
{
protected:
	int m_sockfd;
	string m_clientip;
	Service_Type m_st;

	StorageEngine* m_storageEngine;
public:
	Session(int sockfd, const char* clientip, Service_Type st, StorageEngine* storage_engine, memory_cache* ch);
	virtual ~Session();
	
	void Process();

	memory_cache* m_cache;
};
#endif /* _SESSION_H_*/

