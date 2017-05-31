/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "cli.h"
#include <errno.h>

#define WELCOME_MSG	"**********************************************************\r\n"    \
                    "*     Welcome to Use eRisemail Server On Linux/Unix      *\r\n"    \
                    "*              Powered by uplusware                      *\r\n"    \
                    "**********************************************************\r\n"

#define BYE_MSG     "\r\n"  \
                    "------------------------- Bye ----------------------------\r\n"

CLI::CLI()
{

}

CLI::~CLI()
{

}

void CLI::Run()
{
	if(!CMailBase::LoadConfig((char*)"/etc/erisemail/erisemail.conf"))
	{
		printf("Load Configuration Failed\n");
		return;
	}
	string strline;
	int vlen = m_queueList.size();
	for(int x = 0; x < vlen; x++)
	{
		m_queueList[x].qid = mq_open(m_queueList[x].name,  O_RDWR);
		printf("%s %d\n",m_queueList[x].name, m_queueList[x].qid);
		if(m_queueList[x].qid == -1)
		{
			goto END;	
		}
		mq_attr attr;
		mq_getattr(m_queueList[x].qid, &attr);
	}
	printf(WELCOME_MSG);
    
	while(1)
	{
		printf("CLI# ");
		getline(cin, strline);
		if(!Parse((char*)strline.c_str()))
			break;
	}
END:
	for(int x = 0; x < vlen; x++)
	{
		if(m_queueList[x].qid > 0)
			mq_close(m_queueList[x].qid);
	}
}

BOOL CLI::Parse(char* text)
{
	BOOL ret = TRUE;
	stQueueMsg qMsg;
	string strline = text;
	strtrim(strline);
	if((strline == "exit")||(strline == "quit"))
	{
		printf("Bye.\n");
		ret = FALSE;
	}
	else if(strline == "stop")
	{
		qMsg.cmd = MSG_EXIT;
		int vlen = m_queueList.size();
		for(int x = 0; x < vlen; x++)
		{
			int rc = mq_send(m_queueList[x].qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
			if(rc < 0)
			{
				if(errno != EINTR)
				{
					perror("error");
				}
			}
		}
		ret = FALSE;
	}
	else if(strncasecmp(strline.c_str(), "adduser", sizeof("adduser") - 1) == 0)
	{
		
	}
	else if(strncasecmp(strline.c_str(), "addgroup", sizeof("addgroup") - 1) == 0)
	{
				
	}
	else if(strncasecmp(strline.c_str(), "deluser", sizeof("deluser") - 1) == 0)
	{
				
	}
	else if(strncasecmp(strline.c_str(), "delgroup", sizeof("delgroup") - 1) == 0)
	{
				
	}
	else
		printf("unknown command.\n");
	return ret;
}

