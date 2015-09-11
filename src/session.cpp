#include "session.h"

Session::Session(int sockfd, const char* clientip, Service_Type st, StorageEngine* storage_engine, memory_cache* ch)
{
	m_sockfd = sockfd;
	m_clientip = clientip;
	m_st = st;

	m_cache = ch;

	m_storageEngine = storage_engine;
}

Session::~Session()
{

}

void Session::Process()
{
	CMailBase * pProtocol;
	try{
	if(m_st == stSMTP)
	{
		pProtocol = new CMailSmtp(m_sockfd, m_clientip.c_str(), m_storageEngine);
	}
	else if(m_st == stPOP3)
	{
		pProtocol = new CMailPop(m_sockfd, m_clientip.c_str(), m_storageEngine);
	}
	else if(m_st == stIMAP)
	{
		pProtocol = new CMailImap(m_sockfd, m_clientip.c_str(), m_storageEngine);
	}
	else if(m_st == stSMTPS)
	{
		pProtocol = new CMailSmtp(m_sockfd, m_clientip.c_str(), m_storageEngine, TRUE);
	}
	else if(m_st == stPOP3S)
	{
		pProtocol = new CMailPop(m_sockfd, m_clientip.c_str(), m_storageEngine, TRUE);
	}
	else if(m_st == stIMAPS)
	{
		pProtocol = new CMailImap(m_sockfd, m_clientip.c_str(), m_storageEngine, TRUE);
	}
	else if(m_st == stHTTP)
	{
		pProtocol = new CHttp(m_sockfd, m_clientip.c_str(), m_storageEngine, m_cache);
	}
	else if(m_st == stHTTPS)
	{
		pProtocol = new CHttp(m_sockfd, m_clientip.c_str(), m_storageEngine, m_cache, TRUE);
	}
	else
	{
		return;
	}
	}
	catch(string* e){
		//printf("Exception: %s\n", e->c_str());
		return;
	}
	char szmsg[4096];
	int result;
	while(1)
	{
		result = pProtocol->ProtRecv(szmsg, 4095);
		if(result <= 0)
		{
			break;
		}
		else
		{
			szmsg[result] = '\0';
			if(!pProtocol->Parse(szmsg))
				break;
		}
	}
	delete pProtocol;
}

