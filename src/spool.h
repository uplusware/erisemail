#ifndef _SPOOL_H_
#define _SPOOL_H_
#include <mqueue.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "util/general.h"
#include "base.h"
#include "storage.h"

#define SPOOL_SERVICE	"SPOOL"

class Spool
{
public:
	Spool();
	virtual ~Spool();

	int Run(int fd);
	void Stop();
	void ReloadConfig();
	
	StorageEngine* GetStorageEngine() { return m_storageEngine; }
	
private:
	mqd_t m_spool_qid;
	sem_t* m_spool_sid;
	string m_spool_name;
	
	StorageEngine* m_storageEngine;
};

#endif /* _SPOOL_H_ */

