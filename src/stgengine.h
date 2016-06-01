#ifndef _STORAGE_ENGINE_H_
#define _STORAGE_ENGINE_H_

#include "storage.h"
#include "util/general.h"

typedef struct
{
	MailStorage* storage;
	BOOL opened;
	BOOL inUse;
	unsigned long cTime; //Cteate Time
	unsigned long lTime; //Last using Time
	unsigned long usedCount;
	pthread_t owner;
}stStorageEngine;


class StorageEngine
{
public:
	StorageEngine(const char * host, const char* username, const char* password, const char* database, int maxConn);
	virtual ~StorageEngine();

	MailStorage* WaitEngine(int &index);
	void PostEngine(int index);

	
	string m_host;
	int m_realConn;
	
private:
	string m_username;
	string m_password;
	string m_database;
	int m_maxConn;
	unsigned long m_next;
	stStorageEngine* m_engine;
	
	sem_t m_engineSem;
	pthread_mutex_t m_engineMutex;
	
};

class StorageEngineInstance
{
public:
	StorageEngineInstance(StorageEngine* engine, MailStorage** storage)
	{
		m_engine = engine;
		m_storage = m_engine->WaitEngine(m_engineIndex);
		*storage = m_storage;
        m_isReleased = FALSE;
	}
	
	virtual ~StorageEngineInstance()
	{
		Release();
	}

	void Release()
	{
        if(!m_isReleased)
        {
            m_engine->PostEngine(m_engineIndex);
            m_isReleased = TRUE;
        }
	}
private:
	StorageEngine* m_engine;
	MailStorage* m_storage;
	int m_engineIndex;
	BOOL m_isReleased;
};
#endif /* _STORAGE_ENGINE_H_ */
