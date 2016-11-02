#include "stgengine.h"
#include "base.h"

StorageEngine::StorageEngine(const char * host, const char* username, const char* password, const char* database, int maxConn,
    unsigned short port, const char* sock_file, const char* private_path, const char* encoding, memcached_st * memcached)
{
    m_host = host;
	m_username = username;
	m_password = password;
	m_database = database;
	m_maxConn = maxConn;
    m_port = port;
    m_sock_file = sock_file;
    m_private_path = private_path;
    m_encoding = encoding;
    m_memcached = memcached;
    m_realConn = 0;
	m_next = 0;
	m_engine = new stStorageEngine[m_maxConn];
	sem_init(&m_engineSem, 0, m_maxConn);
	for(int i = 0; i < m_maxConn; i++)
	{
		m_engine[i].storage = new MailStorage(m_encoding.c_str(), m_private_path.c_str(), m_memcached);
        m_engine[i].inUse = FALSE;
	}
	pthread_mutex_init(&m_engineMutex, NULL);
    
}

StorageEngine::~StorageEngine()
{
	pthread_mutex_lock(&m_engineMutex);
	for(int i = 0; i < m_maxConn; i++)
	{
		if(m_engine[i].storage)
		{
			m_engine[i].storage->Close();
			delete m_engine[i].storage;
		}
	}
	if(m_engine)
		delete[] m_engine;
	pthread_mutex_unlock(&m_engineMutex);
	pthread_mutex_destroy(&m_engineMutex);
	
	sem_close(&m_engineSem);
}

MailStorage* StorageEngine::Wait(int &index)
{
    MailStorage* freeOne = NULL;
    sem_wait(&m_engineSem);
    pthread_mutex_lock(&m_engineMutex);
    for(int i = 0; i < m_maxConn; i++)
	{
        if(m_engine[i].inUse == FALSE)
        {
            index = i;
            m_engine[index].inUse = TRUE;
            break;
        }
    }
    pthread_mutex_unlock(&m_engineMutex);
    
    unsigned int connect_timeout = 0;
    while(!freeOne && connect_timeout < 1000 * 1000 * 20) //20 seconds
    {
        if(m_engine[index].storage && m_engine[index].storage->Ping() != 0)
        {
            m_engine[index].storage->Close();
            
            if(m_engine[index].storage->Connect(m_host.c_str(),
                m_username.c_str(), m_password.c_str(),  
                m_database.c_str(), m_port, m_sock_file.c_str()) == 0)
            {
                freeOne = m_engine[index].storage;
            }
            else
            {
               freeOne = NULL;
               usleep(1000);
               connect_timeout += 1000;
            }
        }
        else
        {
            freeOne = m_engine[index].storage;
        }
    }
	return freeOne;
}

void StorageEngine::Post(int index)
{
    pthread_mutex_lock(&m_engineMutex);
    m_engine[index].inUse = FALSE;
    pthread_mutex_unlock(&m_engineMutex);
    
    sem_post(&m_engineSem);
}
	
