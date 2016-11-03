/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _STORAGE_ENGINE_H_
#define _STORAGE_ENGINE_H_

#include "storage.h"
#include "util/general.h"

typedef struct
{
	MailStorage* storage;
	BOOL inUse;
}stStorageEngine;


class StorageEngine
{
public:
	StorageEngine(const char * host, const char* username, const char* password, const char* database, int maxConn,
        unsigned short port, const char* sock_file, const char* private_path, const char* encoding, memcached_st * memcached);
	virtual ~StorageEngine();

	MailStorage* Wait(int &index);
	void Post(int index);

	
	string m_host;
	int m_realConn;
	
private:
	string m_username;
	string m_password;
	string m_database;
	int m_maxConn;
    unsigned short m_port;
    string m_sock_file;
    string m_private_path;
    string m_encoding;
	unsigned long m_next;
	stStorageEngine* m_engine;
	
	sem_t m_engineSem;
	pthread_mutex_t m_engineMutex;
	memcached_st * m_memcached;
};

class StorageEngineInstance
{
public:
	StorageEngineInstance(StorageEngine* engine, MailStorage** storage)
	{
		m_engine = engine;
		m_storage = m_engine->Wait(m_engineIndex);
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
            m_engine->Post(m_engineIndex);
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
