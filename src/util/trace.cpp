/************************************************************************/
/* 声明:                                                                */
/* 作者:Brian   uplusware@gmail.com                                     */
/* 禁止用于任何商业用途,复制请保持本声明的完整性                        */
/* 最后更新:2011.11.14                                                  */
/************************************************************************/

#include "trace.h"

static char* format_time(unsigned long tv,char* sztv)
{
	time_t t;
	tm *now;
	t = tv;
	now = localtime(&t);
	sprintf(sztv,"%04d-%02d-%02d %02d:%02d:%02d",1900+now->tm_year,now->tm_mon+1,now->tm_mday,now->tm_hour,now->tm_min,now->tm_sec);
	return sztv;
}

CUplusTrace::CUplusTrace(const char* filepath, const char* lockname)
{
	strcpy(m_filepath, filepath);
	m_ayslock1 = sem_open(lockname, O_CREAT | O_RDWR, 0644, 1);
	sem_init(&m_ayslock2, 0, 1);
}

CUplusTrace::~CUplusTrace()
{
	if(m_ayslock1 != SEM_FAILED)
	{
		sem_close(m_ayslock1);
	}
	sem_destroy(&m_ayslock2);
}

void CUplusTrace::Write(Trace_Level level, const char* fmt, ...)
{
	if(m_ayslock1 != SEM_FAILED)
		return;
	
	char sv[64];
	va_list vlist ;
	va_start(vlist, fmt) ;
	
	sem_wait(m_ayslock1);
	sem_wait(&m_ayslock2);

	m_file = fopen(m_filepath, "a+");
	if(m_file)
	{
		if(level == Trace_Msg)
		{
			fprintf(m_file, "<MESSAGE>-[%s]: ", format_time(time(NULL), sv));
			vfprintf(m_file, fmt, vlist);
			fprintf(m_file, "\n");
			fflush(m_file);
		}
		else if(level == Trace_Warning)
		{
			fprintf(m_file, "<WARNING>-[%s]: ", format_time(time(NULL), sv));
			vfprintf(m_file, fmt, vlist);
			fprintf(m_file, "\n");
			fflush(m_file);
		}
		else if(level == Trace_Error)
		{
			fprintf(m_file, "< ERROR >-[%s]: ", format_time(time(NULL), sv));
			vfprintf(m_file, fmt, vlist);
			fprintf(m_file, "\n");
			fflush(m_file);
		}
		fclose(m_file);
	}
		
	sem_post(m_ayslock1);
	sem_post(&m_ayslock2);

	va_end(vlist) ; 
}

