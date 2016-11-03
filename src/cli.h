/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _CLI_H_
#define _CLI_H_
#include <stdlib.h>
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

typedef struct
{
	char name[32];
	mqd_t qid;
}QueueInfo;

class CLI : public CMailBase
{
public:
	CLI();
	virtual ~CLI();

	void Run();
	virtual BOOL Parse(char* text);
	vector<QueueInfo> m_queueList;
protected:

};

#endif /* _CLI_H_ */

