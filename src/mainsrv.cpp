#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <time.h>
#include "service.h"
#include "cli.h"
#include <string>
#include <iostream>
#include <syslog.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include "util/md5.h"
#include "dns.h"
#include <libgen.h>
#include <errno.h>
#include <sys/param.h> 
#include <sys/stat.h> 
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <streambuf>
#include <semaphore.h>
#include "spool.h"
#include "util/qp.h"
#include "base.h"
#include "fstring.h"
#include "util/trace.h"

using namespace std;

static void usage()
{
	printf("Usage:erisemaild start | stop | status | reload | version [CONFIG FILE]\n");
}

//set to daemon mode
static void daemon_init()
{
	setsid();
	chdir("/");
	umask(0);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
//	close(STDERR_FILENO);
	signal(SIGCHLD,SIG_IGN);
}

char PIDFILE[256] = "/tmp/erisemail/erisemaild.pid";

int Run()
{
	CUplusTrace uTrace(LOGNAME, LCKNAME);
	uTrace.Write(Trace_Msg, "%s", "Service starts");
	
	/* CMailBase::LoadConfig(); */

	int retVal = 0;
	int smtp_pid = -1, pop3_pid = -1, imap_pid = -1, smtps_pid = -1, pop3s_pid = -1, imaps_pid = -1, http_pid = -1, https_pid = -1;

	do
	{
		int pfd[2];
		pipe(pfd);
		smtp_pid = fork();
		if(smtp_pid == 0)
		{
			char szFlag[128];
			sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stSMTP]);
			if(check_single_on(szFlag)) 
			{
				printf("%s is aready runing.\n", SVR_DESP_TBL[stSMTP]);   
				exit(-1);
			}
				
			close(pfd[0]);
			daemon_init();
			Service smtp_svr(stSMTP);
			smtp_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_smtpport);

			exit(0);
			
		}
		else if(smtp_pid > 0)
		{
			unsigned int result;
			close(pfd[1]);
			read(pfd[0], &result, sizeof(unsigned int));
			if(result == 0)
				printf("Start SMTP Service OK \t\t\t[%u]\n", smtp_pid);
			else
			{
				uTrace.Write(Trace_Msg, "%s", "Start SMTP Service Failed.");
				printf("Start SMTP Service Failed. \t\t\t[Error]\n");
			}
			close(pfd[0]);
		}
		else
		{
			close(pfd[0]);
			close(pfd[1]);
			retVal = -1;
			break;
		}
		
		if(CMailBase::m_enablepop3)
		{
                pipe(pfd);
                pop3_pid = fork();
                if(pop3_pid == 0)
                {
                    char szFlag[128];
                    sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stPOP3]);
                    if(check_single_on(szFlag) )   
                    {   
                        printf("%s is aready runing.\n", SVR_DESP_TBL[stPOP3]);   
                        exit(-1);  
                    }
            
                    close(pfd[0]);
                    daemon_init();
                    Service pop3_svr(stPOP3);
                    pop3_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_pop3port);
                    exit(0);
                }
                else if(pop3_pid > 0)
                {
                    unsigned int result;
                    close(pfd[1]);
                    read(pfd[0], &result, sizeof(unsigned int));
                    if(result == 0)
                        printf("Start POP3 Service OK \t\t\t[%u]\n", pop3_pid);
                    else
                    {
                        uTrace.Write(Trace_Error, "%s", "Start POP3 Service Failed.");
                        printf("Start POP3 Service Failed. \t\t\t[Error]\n");
                    }
                    close(pfd[0]);
                }
			else
			{
				close(pfd[0]);
				close(pfd[1]);
				retVal = -1;
				break;
			}
		}

		if(CMailBase::m_enableimap)
		{
			pipe(pfd);
			imap_pid = fork();
			if(imap_pid == 0)
			{
				char szFlag[128];
				sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stIMAP]);
				if(check_single_on(szFlag)  )  
				{   
					printf("%s is aready runing.\n", SVR_DESP_TBL[stIMAP]);   
					exit(-1);
				}
				
				close(pfd[0]);
				daemon_init();
				Service imap_svr(stIMAP);
				imap_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_imapport);
				exit(0);
			}
			else if(imap_pid > 0)
			{
				unsigned int result;
				close(pfd[1]);
				read(pfd[0], &result, sizeof(unsigned int));
				if(result == 0)
					printf("Start IMAP Service OK \t\t\t[%u]\n", imap_pid);
				else
				{
					uTrace.Write(Trace_Error, "%s", "Start IMAP Service Failed.");
					printf("Start IMAP Service Failed. \t\t\t[Error]\n");
				}
				close(pfd[0]);
			}
			else
			{
				close(pfd[0]);
				close(pfd[1]);
				retVal = -1;
				break;
			}
		}

		if(CMailBase::m_enablehttp)
		{
			pipe(pfd);
			http_pid = fork();
			if(http_pid == 0)
			{
				char szFlag[128];
				sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stHTTP]);
				if(check_single_on(szFlag)  )  
				{   
					printf("%s is aready runing.\n", SVR_DESP_TBL[stHTTP]);   
					exit(-1);  
				}
				
				close(pfd[0]);
				daemon_init();
				Service http_svr(stHTTP);
				http_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_httpport);
				exit(0);
			}
			else if(http_pid > 0)
			{
				unsigned int result;
				close(pfd[1]);
				read(pfd[0], &result, sizeof(unsigned int));
				if(result == 0)
					printf("Start WebMail Service OK \t\t[%u]\n", http_pid);
				else
				{
					uTrace.Write(Trace_Error, "%s", "Start WebMail Service Failed.");
					printf("Start WebMail Service Failed. \t\t[Error]\n");
				}
				close(pfd[0]);
			}
			else
			{
				close(pfd[0]);
				close(pfd[1]);
				retVal = -1;
				break;
			}
		}
		
		//SSL Service
		if(CMailBase::m_enablesmtps)
		{
			pipe(pfd);
			smtps_pid = fork();
			if(smtps_pid == 0)
			{
				char szFlag[128];
				sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stSMTPS]);
				if(check_single_on(szFlag))    
				{   
					printf("%s is aready runing.\n", SVR_DESP_TBL[stSMTPS]);   
					exit(-1);  
				}
				
				close(pfd[0]);
				daemon_init();
				Service smtps_svr(stSMTPS);
				smtps_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_smtpsport);
				exit(0);
				
			}
			else if(smtps_pid > 0)
			{
				unsigned int result;
				close(pfd[1]);
				read(pfd[0], &result, sizeof(unsigned int));
				if(result == 0)
					printf("Start SMTP on SSL Service OK \t\t[%u]\n", smtps_pid);
				else
				{
					uTrace.Write(Trace_Error, "%s", "Start SMTP on SSL Service Failed.");
					printf("Start SMTP on SSL Service Failed. \t\t[Error]\n");
				}
				close(pfd[0]);
			}
			else
			{
				close(pfd[0]);
				close(pfd[1]);
				retVal = -1;
				break;
			}
		}

		if(CMailBase::m_enablepop3s)
		{
			pipe(pfd);
			pop3s_pid = fork();
			if(pop3s_pid == 0)
			{
				char szFlag[128];
				sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stPOP3S]);
				if(check_single_on(szFlag))    
				{   
					printf("%s is aready runing.\n", SVR_DESP_TBL[stPOP3S]);   
					exit(-1);  
				}
				
				close(pfd[0]);
				daemon_init();
				Service pop3s_svr(stPOP3S);
				pop3s_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_pop3sport);
				exit(0);
			}
			else if(pop3s_pid > 0)
			{
				unsigned int result;
				close(pfd[1]);
				read(pfd[0], &result, sizeof(unsigned int));
				if(result == 0)
					printf("Start POP3 on SSL Service OK \t\t[%u]\n", pop3s_pid);
				else
				{
					uTrace.Write(Trace_Error, "%s", "Start POP3 on SSL Service Failed.");
					printf("Start POP3 on SSL Service Failed. \t\t[Error]\n");
				}
				close(pfd[0]);
			}
			else
			{
				close(pfd[0]);
				close(pfd[1]);
				retVal = -1;
				break;
			}
		}

		if(CMailBase::m_enableimaps)
		{
			pipe(pfd);
			imaps_pid = fork();
			if(imaps_pid == 0)
			{
				char szFlag[128];
				sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stIMAPS]);
				if(check_single_on(szFlag) )   
				{   
					printf("%s is aready runing.\n", SVR_DESP_TBL[stIMAPS]);   
					exit(-1);  
				}
				
				close(pfd[0]);
				
				daemon_init();
				Service imaps_svr(stIMAPS);
				imaps_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_imapsport);
				exit(0);
			}
			else if(imaps_pid > 0)
			{
				unsigned int result;
				close(pfd[1]);
				read(pfd[0], &result, sizeof(unsigned int));
				if(result == 0)
					printf("Start IMAP on SSL Service OK \t\t[%u]\n", imaps_pid);
				else
				{
					uTrace.Write(Trace_Error, "%s", "Start IMAP on SSL Service Failed.");
					printf("Start IMAP on SSL Service Failed. \t\t[Error]\n");
				}
				close(pfd[0]);
			}
			else
			{
				close(pfd[0]);
				close(pfd[1]);

				retVal = -1;
				break;
			}
		}

		if(CMailBase::m_enablehttps)
		{
			pipe(pfd);
			https_pid = fork();
			if(https_pid == 0)
			{
				char szFlag[128];
				sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stHTTPS]);
				if(check_single_on(szFlag))    
				{   
					printf("%s is aready runing.\n", SVR_DESP_TBL[stHTTPS]);   
					exit(-1);  
				}
				
				close(pfd[0]);
				
				daemon_init();
				Service https_svr(stHTTPS);
				https_svr.Run(pfd[1], CMailBase::m_hostip.c_str(), (unsigned short)CMailBase::m_httpsport);
				exit(0);
			}
			else if(https_pid > 0)
			{
				unsigned int result;
				close(pfd[1]);
				read(pfd[0], &result, sizeof(unsigned int));
				if(result == 0)
					printf("Start WebMail on SSL Service OK \t[%u]\n", https_pid);
				else
				{
					uTrace.Write(Trace_Error, "%s", "Start WebMail on SSL Service Failed.");
					printf("Start WebMail on SSL Service Failed. \t[Error]\n");
				}
				close(pfd[0]);
			}
			else
			{
				close(pfd[0]);
				close(pfd[1]);

				retVal = -1;
				break;
			}
		}
		
		//Relay service
		int spool_pids;
		pipe(pfd);
		spool_pids = fork();
		if(spool_pids == 0)
		{
			char szFlag[128];
			sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stSPOOL]);
			if(check_single_on(szFlag) )   
			{   
				printf("%s is aready runing.\n", SVR_DESP_TBL[stSPOOL]);   
				exit(-1);  
			}
			
			close(pfd[0]);	
			daemon_init();
			Spool spool;
			spool.Run(pfd[1]);
			exit(0);
		}
		else if(spool_pids > 0)
		{
			unsigned int result;
			close(pfd[1]);
			read(pfd[0], &result, sizeof(unsigned int));
			if(result == 0)
				printf("Start Relay Service OK \t\t\t[%u]\n", spool_pids);
			else
			{
				uTrace.Write(Trace_Error, "%s", "Start Relay Service Failed.");
				printf("Start Relay Service Failed \t\t\t[Error]\n");
			}
			close(pfd[0]);
		}
		else
		{
			retVal = -1;
			break;
		}
        
		//Watch Dog
		int watchdog_pids;
		pipe(pfd);
		watchdog_pids = fork();
		if(watchdog_pids == 0)
		{
			char szFlag[128];
			sprintf(szFlag, "/tmp/erisemail/%s.pid", "WATCHDOG");
			if(check_single_on(szFlag) )   
			{   
				printf("%s is aready runing.\n", "Watch Dog");   
				exit(-1);  
			}
			
			close(pfd[0]);	
			daemon_init();
			WatchDog wdog;
			wdog.Run(pfd[1]);
			exit(0);
		}
		else if(watchdog_pids > 0)
		{
			unsigned int result;
			close(pfd[1]);
			read(pfd[0], &result, sizeof(unsigned int));
			if(result == 0)
				printf("Start Watch Dog OK \t\t\t[%u]\n", watchdog_pids);
			else
			{
				uTrace.Write(Trace_Error, "%s", "Start Watch Dog Service Failed.");
				printf("Start Watch Dog Service Failed \t\t\t[Error]\n");
			}
			close(pfd[0]);
		}
		else
		{
			retVal = -1;
			break;
		}
	}while(0);
	
	CMailBase::UnLoadConfig();
	
	return retVal;
}

static int Stop()
{
	printf("Stop eRisemail service ...\n");
	
	WatchDog wdog;
	wdog.Stop();
	
	Service smtp_svr(stSMTP);
	smtp_svr.Stop();
	
	Service pop3_svr(stPOP3);
	pop3_svr.Stop();

	Service imap_svr(stIMAP);
	imap_svr.Stop();

	Service http_svr(stHTTP);
	http_svr.Stop();	

	Service smtps_svr(stSMTPS);
	smtps_svr.Stop();
		
	Service pop3s_svr(stPOP3S);
	pop3s_svr.Stop();

	Service imaps_svr(stIMAPS);
	imaps_svr.Stop();

	Service https_svr(stHTTPS);
	https_svr.Stop();
	
	Spool spool_svr;
	spool_svr.Stop();
}

static void Version()
{
	printf("v%s\n", CMailBase::m_sw_version.c_str());
}

static int Reload()
{
	printf("Reload eRisemail configuration ...\n");
	
	Service smtp_svr(stSMTP);
	smtp_svr.ReloadConfig();

	Service pop3_svr(stPOP3);
	pop3_svr.ReloadConfig();

	Service imap_svr(stIMAP);
	imap_svr.ReloadConfig();

	Service http_svr(stHTTP);
	http_svr.ReloadConfig();
	
	Service smtps_svr(stSMTPS);
	smtps_svr.ReloadConfig();

	Service pop3s_svr(stPOP3S);
	pop3s_svr.ReloadConfig();

	Service imaps_svr(stIMAPS);
	imaps_svr.ReloadConfig();

	Service https_svr(stHTTPS);
	https_svr.ReloadConfig();
	
	Spool spool_svr;
	spool_svr.ReloadConfig();
}

static int processcmd(const char* cmd, const char* conf, const char* permit, const char* reject, const char* domain, const char* webadmin)
{
	CMailBase::SetConfigFile(conf, permit, reject, domain, webadmin);
	if(!CMailBase::LoadConfig())
	{
		printf("Load Configure File Failed.\n");
		return -1;
	}
	
	if(strcasecmp(cmd, "stop") == 0)
	{
		Stop();
	}
	else if(strcasecmp(cmd, "start") == 0)
	{
		Run();
	}
	else if(strcasecmp(cmd, "reload") == 0)
	{
		Reload();
	}
	else if(strcasecmp(cmd, "status") == 0)
	{
		char szFlag[128];
		sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stSMTP]);
		if(check_single_on(szFlag) )   
		{   
			printf("%s is runing.\n", SVR_DESP_TBL[stSMTP]);   
		}
		else
		{
			printf("%s stopped.\n", SVR_DESP_TBL[stSMTP]);   
		}
		
		sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stPOP3]);
		if(check_single_on(szFlag) )   
		{   
			printf("%s is runing.\n", SVR_DESP_TBL[stPOP3]);   
		}
		else
		{
			printf("%s stopped.\n", SVR_DESP_TBL[stPOP3]);   
		}

		sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stIMAP]);
		if(check_single_on(szFlag))    
		{   
			printf("%s is runing.\n", SVR_DESP_TBL[stIMAP]);   
		}
		else
		{
			printf("%s stopped.\n", SVR_DESP_TBL[stIMAP]);   
		}
		sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stHTTP]);
		if(check_single_on(szFlag))    
		{   
			printf("%s is runing.\n", SVR_DESP_TBL[stHTTP]);   
		}
		else
		{
			printf("%s stopped.\n", SVR_DESP_TBL[stHTTP]);   
		}


		sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stSMTPS]);
		if(check_single_on(szFlag))    
		{   
			printf("%s is runing.\n", SVR_DESP_TBL[stSMTPS]);   
		}
		else
		{
			printf("%s stopped.\n", SVR_DESP_TBL[stSMTPS]);   
		}


		sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stPOP3S]);
		if(check_single_on(szFlag) )   
		{   
			printf("%s is runing.\n", SVR_DESP_TBL[stPOP3S]);   
		}
		else
		{
			printf("%s stopped.\n", SVR_DESP_TBL[stPOP3S]);   
		}

		sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stIMAPS]);
		if(check_single_on(szFlag))    
		{   
			printf("%s is runing.\n", SVR_DESP_TBL[stIMAPS]);   
		}
		else
		{
			printf("%s stopped.\n", SVR_DESP_TBL[stIMAPS]);   
		}
		sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stHTTPS]);
		if(check_single_on(szFlag))    
		{   
			printf("%s is runing.\n", SVR_DESP_TBL[stHTTPS]);   
		}
		else
		{
			printf("%s stopped.\n", SVR_DESP_TBL[stHTTPS]);   
		}
		sprintf(szFlag, "/tmp/erisemail/%s.pid", SVR_NAME_TBL[stSPOOL]);
		if(check_single_on(szFlag) )   
		{   
			printf("%s is runing.\n", SVR_DESP_TBL[stSPOOL]);   
		}
		else
		{
			printf("%s stopped.\n", SVR_DESP_TBL[stSPOOL]);   
		}
		
	}
	else if(strcasecmp(cmd, "version") == 0)
	{
		Version();
	}
	else
	{
		usage();
	}
	CMailBase::UnLoadConfig();
	return 0;	

}

static void handle_signal(int sid) 
{ 
	signal(SIGPIPE, handle_signal);
}

int main(int argc, char* argv[])
{	
	if(getgid() != 0)
	{
		printf("You need root privileges to run this program\n");
		return -1;
	}
	else
	{
		mkdir("/tmp/erisemail", 0777);
		chmod("/tmp/erisemail", 0777);

		mkdir("/var/log/erisemail/", 0744);
		
		// Set up the signal handler
		signal(SIGPIPE, SIG_IGN);
		sigset_t signals;
		sigemptyset(&signals);
		sigaddset(&signals, SIGPIPE);
		sigprocmask(SIG_BLOCK, &signals, NULL);

		if(argc == 2)
		{
			processcmd(argv[1], CONFIG_FILE_PATH, PERMIT_FILE_PATH, REJECT_FILE_PATH, DOMAIN_FILE_PATH, WEBADMIN_FILE_PATH);
		}
		else
		{
			usage();
			return -1;
		}
		return 0;
	}
}

