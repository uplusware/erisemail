/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _TRACE_H_
#define _TRACE_H_

#include <stdio.h>
#include <string.h>
#include <fcntl.h>			 /* For O_* constants */
#include <sys/stat.h> 	   /* For mode constants */
#include <semaphore.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

static char LOGNAME[256] = "/var/log/erisemail/SERVICE.log";
static char LCKNAME[256] = "/.ERISEMAIL_SYS.LOG";

enum Trace_Level
{
	Trace_Msg = 0,
	Trace_Warning,
	Trace_Error
};

class CUplusTrace
{
public:
	CUplusTrace(const char* filepath, const char* lockname);
	virtual ~CUplusTrace();
	void Write(Trace_Level level, const char* fmt, ...);
private:
	FILE* m_file;
	sem_t* m_ayslock1; //Process
	sem_t  m_ayslock2; //Thread
	char m_filepath[256];
};

#endif /* _TRACE_H_ */
