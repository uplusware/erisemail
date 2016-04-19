#include "util/general.h"
#include "pop.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "util/md5.h"
#include "service.h"

CMailPop::CMailPop(int sockfd, SSL * ssl, SSL_CTX * ssl_ctx, const char* clientip,
    StorageEngine* storage_engine, memcached_st * memcached, BOOL isSSL)
{		
	m_status = STATUS_ORIGINAL;
	m_sockfd = sockfd;
	m_clientip = clientip;

    m_bSTARTTLS = FALSE;

	m_lsockfd = NULL;
	m_lssl = NULL;
	
	m_storageEngine = storage_engine;
	m_memcached = memcached;
	
    m_ssl = ssl;
    m_ssl_ctx = ssl_ctx;

	m_isSSL = isSSL;
	if(m_isSSL && m_ssl)
	{	
		int flags = fcntl(m_sockfd, F_GETFL, 0); 
		fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK); 

		m_lssl = new linessl(m_sockfd, m_ssl);
		
	}
	else
	{
		int flags = fcntl(m_sockfd, F_GETFL, 0); 
		fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK); 
		m_lsockfd = new linesock(m_sockfd);
	}

	On_Service_Ready_Handler();
}

CMailPop::~CMailPop()
{
	
    if(m_bSTARTTLS)
	    close_ssl(m_ssl, m_ssl_ctx);
	if(m_lsockfd)
		delete m_lsockfd;

	if(m_lssl)
		delete m_lssl;

	
}

int CMailPop::PopSend(const char* buf, int len)
{
	//printf("%s", buf);
		if(m_ssl)
			return SSLWrite(m_sockfd, m_ssl, buf, len);
		else
			return Send(m_sockfd, buf, len);	
}

int CMailPop::ProtRecv(char* buf, int len)
{
		if(m_ssl)
			return m_lssl->lrecv(buf, len);
		else
			return m_lsockfd->lrecv(buf, len);
}

void CMailPop::On_Service_Ready_Handler()
{
	char cmd[1024];
	srand(time(NULL));
	sprintf(cmd,"<%lu%lu%lu.%lu@%s>", time(NULL), getpid(), pthread_self(), m_global_uid, m_localhostname.c_str());
	
	m_global_uid++;
	m_strDigest = cmd;
	sprintf(cmd,"+OK %s POP3 service is ready by eRisemail-%s powered by Uplusware %s\r\n", m_localhostname.c_str(), m_sw_version.c_str(), m_strDigest.c_str());
	
	PopSend(cmd, strlen(cmd));
}


void CMailPop::On_Service_Error_Handler()
{
	
	char cmd[256];
	sprintf(cmd,"-ERR eRiseMail Sever POP3 server has some error\r\n");
	PopSend(cmd,strlen(cmd));
}

void CMailPop::On_Unrecognized_Command_Handler()
{
	char cmd[256];
	sprintf(cmd,"-ERR unrecognized command line\r\n");
	PopSend(cmd,strlen(cmd));
}


void CMailPop::On_Capa_Handler(char* text)
{
	char cmd[128];
	sprintf(cmd,"+OK Capability list follows\r\n");
	PopSend(cmd,strlen(cmd));
	sprintf(cmd,"UIDL\r\n");
	PopSend(cmd,strlen(cmd));
	sprintf(cmd,"APOP\r\n");
	PopSend(cmd,strlen(cmd));
	sprintf(cmd,"USER\r\n");
	PopSend(cmd,strlen(cmd));
	sprintf(cmd,"TOP\r\n");
	PopSend(cmd,strlen(cmd));
	sprintf(cmd,"STLS\r\n");
	PopSend(cmd,strlen(cmd));
	sprintf(cmd,".\r\n");
	PopSend(cmd,strlen(cmd));
}
				
void CMailPop::On_Apop_Handler(char* text)
{
	MailStorage* mailStg;
	StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
	if(!mailStg)
	{
		printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
		return;
	}
	char cmd[256];
	string strArg;
	strcut(text, " ", "\r\n", strArg);
	strcut(strArg.c_str(), NULL, " ", m_username);
	strcut(strArg.c_str(), " ", NULL, m_strToken);

	string strpwd;
	
	if(mailStg->GetPassword(m_username.c_str(), strpwd) == 0)
	{
		string strMD5src = m_strDigest;
		strMD5src += strpwd;

		MD5_CTX context;
		context.MD5Update ((unsigned char*)strMD5src.c_str(), strMD5src.length());
		unsigned char digest[16];
		context.MD5Final (digest);
		char szMD5dst[33];
		memset(szMD5dst, 0, 33);
		sprintf(szMD5dst,
			"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7],
			digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15]);
		
		if(strcasecmp(szMD5dst, m_strToken.c_str()) == 0)
		{
			string recv_user;
			recv_user = m_username;
			m_mailTbl.clear();
			mailStg->ListMailByDir(recv_user.c_str(), m_mailTbl, "INBOX");
			m_lettersCount = m_mailTbl.size();
			m_lettersTotalSize = 0;
			for(int x = 0; x< m_lettersCount; x++)
			{
				MailLetter* Letter;
							
							
				string emlfile;
				mailStg->GetMailIndex(m_mailTbl[x].mid, emlfile);
							
				Letter = new MailLetter(m_memcached, emlfile.c_str());
				m_lettersTotalSize += Letter->GetSize();
				delete Letter;
			}
		
			sprintf(cmd,"+OK %u message<s> [%u byte(s)]\r\n", m_lettersCount, m_lettersTotalSize);
			PopSend(cmd,strlen(cmd));
			m_status = m_status|STATUS_AUTHED;
		}
		else
		{
			push_reject_list(m_isSSL ? stPOP3S : stPOP3, m_clientip.c_str());
			
			sprintf(cmd,"-ERR Unable to log on.\r\n");
			PopSend(cmd,strlen(cmd));
		}
	}
	else
	{
		push_reject_list(m_isSSL ? stPOP3S : stPOP3, m_clientip.c_str());
		
		sprintf(cmd,"-ERR Unable to log on.\r\n");
		PopSend(cmd,strlen(cmd));
	}
	
}

void CMailPop::On_Stat_Handler(char* text)
{
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		string recv_user;
		recv_user = m_username;
		char cmd[256];
		m_mailTbl.clear();
		mailStg->ListMailByDir(recv_user.c_str(), m_mailTbl, "INBOX");
		m_lettersCount = m_mailTbl.size();
		m_lettersTotalSize = 0;
		for(int x = 0; x< m_lettersCount; x++)
		{
			MailLetter* Letter;

			string emlfile;
			mailStg->GetMailIndex(m_mailTbl[x].mid, emlfile);

			Letter = new MailLetter(m_memcached, emlfile.c_str());
			m_lettersTotalSize += Letter->GetSize();
			delete Letter;
		}
		sprintf(cmd,"+OK %u message(s) [%u byte(s)]\r\n",m_lettersCount,m_lettersTotalSize);
		PopSend(cmd,strlen(cmd));
	}
	else
	{
		On_Not_Valid_State_Handler();
	}
}

void CMailPop::On_Not_Valid_State_Handler()
{
	char cmd[256];
	sprintf(cmd,"-ERR Command not valid in this state\r\n");
	PopSend(cmd,strlen(cmd));
}

BOOL CMailPop::On_Supose_Username_Handler(char* text)
{
	strcut(text, " ", "\r\n", m_username);

	m_status = m_status&(~STATUS_AUTH_STEP1);
	
	char cmd[256];
	sprintf(cmd,"+OK core mail\r\n");
	PopSend(cmd,strlen(cmd));
	return TRUE;
}

BOOL CMailPop::On_Supose_Password_Handler(char* text)
{
	strcut(text, " ", "\r\n", m_password);
	
	m_status = m_status&(~STATUS_AUTH_STEP2);

	return On_Supose_Checking_Handler();
}

BOOL CMailPop::On_Supose_Checking_Handler()
{	
	char cmd[256];
	MailStorage* mailStg;
	StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
	if(!mailStg)
	{
		printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
		return FALSE;
	}
	if(mailStg->CheckLogin(m_username.c_str(),m_password.c_str()) == 0)
	{
		string recv_user;
		recv_user = m_username;

		m_mailTbl.clear();
		mailStg->ListMailByDir(recv_user.c_str(), m_mailTbl, "INBOX");
		m_lettersCount = m_mailTbl.size();
		m_lettersTotalSize = 0;
		for(int x = 0; x< m_lettersCount; x++)
		{
			MailLetter* Letter;
			string emlfile;
			mailStg->GetMailIndex(m_mailTbl[x].mid, emlfile);
			
			Letter = new MailLetter(m_memcached, emlfile.c_str());
			m_lettersTotalSize += Letter->GetSize();
			delete Letter;
		}
		sprintf(cmd,"+OK %u message<s> [%u byte(s)]\r\n",m_lettersCount,m_lettersTotalSize);
		PopSend(cmd,strlen(cmd));
		m_status = m_status|STATUS_AUTHED;
		return TRUE;
	}
	else
	{
		push_reject_list(m_isSSL ? stPOP3S : stPOP3, m_clientip.c_str());
		
		sprintf(cmd,"-ERR Unable to log on.\r\n");
		PopSend(cmd,strlen(cmd));
		return FALSE;
	}

}

void CMailPop::On_List_Handler(char* text)
{
	
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		char cmd[256];
		string strIndex;
		strcut(text, " ", "\r\n", strIndex);

		if(strcmp(strIndex.c_str(),"") == 0)
		{
			sprintf(cmd,"+OK %u message<s> [%u byte(s)]\r\n",m_lettersCount,m_lettersTotalSize);
			PopSend(cmd,strlen(cmd));

			vector<Mail_Info> mitbl;
			mailStg->ListMailByDir(m_username.c_str(), mitbl, "INBOX");
			int vlen = mitbl.size();
			for(int i = 0; i < vlen; i++)
			{
				MailLetter* Letter;
				string emlfile;
				mailStg->GetMailIndex(m_mailTbl[i].mid, emlfile);
			
				Letter = new MailLetter(m_memcached, emlfile.c_str());
				sprintf(cmd, "%d %u\r\n", i + 1, Letter->GetSize());
				delete Letter;
				PopSend(cmd,strlen(cmd));
			}
			sprintf(cmd, ".\r\n");
			PopSend(cmd,strlen(cmd));
		}
		else
		{
			int index = atoi(strIndex.c_str());
			if((m_lettersCount < (unsigned int)index)||(index <= 0))
			{
				sprintf(cmd,"-ERR no such message\r\n");
			}
			else
			{
				MailLetter* Letter;
				string emlfile;
				mailStg->GetMailIndex(m_mailTbl[index - 1].mid, emlfile);
			
				Letter = new MailLetter(m_memcached, emlfile.c_str());
				sprintf(cmd,"+OK %d %u\r\n",index, Letter->GetSize());
				delete Letter;
			}
			PopSend(cmd,strlen(cmd));
		}
	}
	else
	{
		On_Not_Valid_State_Handler();
	}
}

void CMailPop::On_Uidl_Handler(char* text)
{
	
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		char cmd[256];
		string strIndex;
		strcut(text, " ", "\r\n", strIndex);

		if(strcmp(strIndex.c_str(),"") == 0)
		{
			sprintf(cmd, "+OK\r\n");
			PopSend(cmd,strlen(cmd));
			vector<Mail_Info> mitbl;
			mailStg->ListMailByDir(m_username.c_str(), mitbl, "INBOX");
			int vlen = mitbl.size();
			for(int i = 0; i < vlen; i++)
			{
				sprintf(cmd, "%d %s\r\n", i + 1, mitbl[i].uniqid);
				PopSend(cmd,strlen(cmd));
			}
			sprintf(cmd, ".\r\n");
			PopSend(cmd,strlen(cmd));
		}
		else
		{
			int index = atoi(strIndex.c_str());
			if((m_lettersCount < (unsigned int)index)||(index <= 0))
			{
				sprintf(cmd,"-ERR no such message\r\n");
			}
			else
			{
				
				sprintf(cmd,"+OK %d %s\r\n",index, m_mailTbl[index-1].uniqid);
			}
			PopSend(cmd,strlen(cmd));
		}
	}
	else
	{
		On_Not_Valid_State_Handler();
	}
}

void CMailPop::On_Rset_Handler(char* text)
{
	char cmd[256];
	for(int x =0; x< m_lettersCount; x++)
	{
		m_mailTbl[x].reserve = 0;
	}
	sprintf(cmd,"+OK Reset all.\r\n");
	PopSend(cmd,strlen(cmd));
}

void CMailPop::On_Delete_Handler(char* text)
{
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		char cmd[256];
		
		string strIndex;
		strcut(text, " ", "\r\n", strIndex);
		int index = atoi(strIndex.c_str());
		if((m_lettersCount < (unsigned int)index)||(index <= 0))
		{
			sprintf(cmd,"-ERR no such message\r\n");
		}
		else
		{
			if(m_mailTbl[index-1].reserve == 0)
			{
				sprintf(cmd,"+OK message %d deleted\r\n",index);
				m_mailTbl[index-1].reserve = 1;
			}
			else
			{
				sprintf(cmd,"-ERR message %d already deleted\r\n",index);
			}
		}
		PopSend(cmd,strlen(cmd));
	}
	else
	{
		On_Not_Valid_State_Handler();
	}
}

void CMailPop::On_Retr_Handler(char* text)
{
	
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		char cmd[256];
		string strIndex;
		strcut(text, " ", "\r\n", strIndex);
		int index = atoi(strIndex.c_str());
		if((m_lettersCount < (unsigned int)index)||(index <=0))
		{
			sprintf(cmd,"-ERR no such message\r\n");
			PopSend(cmd,strlen(cmd));
		}
		else
		{
			MailLetter* Letter;
			
			string emlfile;
			mailStg->GetMailIndex(m_mailTbl[index - 1].mid, emlfile);
				
			Letter = new MailLetter(m_memcached, emlfile.c_str());
			if(Letter->GetSize() > 0)
			{
				sprintf(cmd,"+OK %u byte(s)\r\n",Letter->GetSize());
				PopSend(cmd,strlen(cmd));
				
				int llen;
				char* lbuf = Letter->Body(llen);

				int wlen = 0;
				while(1)
				{
					if(PopSend(lbuf + wlen, (llen - wlen) > 1448 ? 1448 : (llen - wlen)) < 0)
						break;
					wlen += ((llen - wlen) > 1448 ? 1448 : (llen - wlen));
					if(wlen >= llen)
						break;
				}
				PopSend("\r\n.\r\n",strlen("\r\n.\r\n"));
			}
			else
			{		
				string badmail = "From: ";
				badmail += "postmaster<postermaster@";
				badmail += CMailBase::m_email_domain;
				badmail += ">\r\n";
				badmail += "Subject: NOTE: Unable to read this destroyed mail\r\n"
					"Content-Type: text/plain; charset=ISO-8859-1\r\n"
					"Content-Transfer-Encoding: 7bit\r\n\r\n"
					"NOTE: This is a mail from ";
				badmail += CMailBase::m_email_domain;
				badmail += ".\r\nThis mail have been destroyed for some reason. Unable to read this destroyed mail.";

				sprintf(cmd, "+OK %u byte(s)\r\n",badmail.length());
				PopSend(cmd, strlen(cmd));
				PopSend(badmail.c_str(), badmail.length());
				PopSend("\r\n.\r\n",strlen("\r\n.\r\n"));
			}
			
			delete Letter;
						
		}
	}
	else
	{
		On_Not_Valid_State_Handler();
	}
}

void CMailPop::On_Top_Handler(char* text)
{	
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		char cmd[256];
		string strtmp;
		strcut(text, " ", "\r\n", strtmp);
		
		string strIndex, strLineNum;
		strcut(strtmp.c_str(), NULL, " ", strIndex);
		strcut(strtmp.c_str(), " ", NULL, strLineNum);
		
		int index = atoi(strIndex.c_str());
		int lineNum = atoi(strLineNum.c_str());
		
		if((m_lettersCount < (unsigned int)index)||(index <=0))
		{
			sprintf(cmd,"-ERR no such message\r\n");
			PopSend(cmd,strlen(cmd));
		}
		else
		{
			MailLetter* Letter;

			string emlfile;
			mailStg->GetMailIndex(m_mailTbl[index - 1].mid, emlfile);
			
			Letter = new MailLetter(m_memcached, emlfile.c_str());
			sprintf(cmd,"+OK %u byte(s)\r\n",Letter->GetSize());
			
			PopSend(cmd,strlen(cmd));
			if(Letter->GetSize() > 0)
			{
				int sc = 0;
				int llen;
				char* lbuf = Letter->Body(llen);
				for(int i = 0; i < lineNum; i++)
				{
					while(sc < llen)
					{
						PopSend(lbuf + sc, 1);
						sc++;
						if(lbuf[sc] == '\n')
							break;
					}
				}
			}
			delete Letter;
			PopSend("\r\n.\r\n",strlen("\r\n.\r\n"));
		}
	}
	else
	{
		On_Not_Valid_State_Handler();
	}
}

void CMailPop::On_Quit_Handler()
{

	MailStorage* mailStg;
	StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
	if(!mailStg)
	{
		printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
		return;
	}
	for(unsigned int i = 0; i < m_lettersCount; i++)
	{
		if(m_mailTbl[i].reserve == 1)
		{
			mailStg->DelMail(m_username.c_str(), m_mailTbl[i].mid);
		}
	}
	char cmd[256];
	sprintf(cmd,"+OK eRiseMail Server POP3 server.\r\n");
	PopSend(cmd,strlen(cmd));
}

void CMailPop::On_STLS_Handler()
{
	char cmd[256];
	if(!m_enablepop3tls)
	{
		sprintf(cmd,"-ERR TLS si disabled\r\n");
		PopSend(cmd,strlen(cmd));
		return;
	}
	if(m_bSTARTTLS == TRUE)
	{
		sprintf(cmd,"-ERR Command not permitted when TLS active\r\n");
		PopSend(cmd,strlen(cmd));
		return;
	}
	else
	{
		sprintf(cmd,"+OK Begin TLS negotiation\r\n");
		PopSend(cmd,strlen(cmd));
	}

    m_bSTARTTLS = TRUE;

	int flags = fcntl(m_sockfd, F_GETFL, 0); 
	fcntl(m_sockfd, F_SETFL, flags & (~O_NONBLOCK)); 
	
	if(!create_ssl(m_sockfd, 
        m_ca_crt_root.c_str(),
        m_ca_crt_server.c_str(),
        m_ca_password.c_str(),
        m_ca_key_server.c_str(),
        m_enableclientcacheck,
        &m_ssl, &m_ssl_ctx))
    {
        throw new string(ERR_error_string(ERR_get_error(), NULL));
        return;
    }

	flags = fcntl(m_sockfd, F_GETFL, 0); 
	fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK); 
	m_lssl = new linessl(m_sockfd, m_ssl);
}

BOOL CMailPop::Parse(char* text)
{
	//printf(text);
	BOOL result = TRUE;
	if((m_status&STATUS_AUTH_STEP1) == STATUS_AUTH_STEP1)
	{
		On_Supose_Username_Handler(text);	
	}
	else if( (m_status&STATUS_AUTH_STEP2) == STATUS_AUTH_STEP2)
	{
		On_Supose_Password_Handler(text);
	}
	else
	{
		unsigned int len = strlen(text);
		if(len < 4)
		{
			On_Unrecognized_Command_Handler();
		}
		else
		{
			if(strncasecmp(text,"USER",4) == 0)
			{
				return On_Supose_Username_Handler(text);
			}
			else if(strncasecmp(text,"PASS",4) == 0)
			{
			
				result = On_Supose_Password_Handler(text);
				
			}
			else if(strncasecmp(text, "APOP", 4) == 0)
			{
				
				On_Apop_Handler(text);

				
			}
			else if(strncasecmp(text, "CAPA", 4) == 0)
			{
				On_Capa_Handler(text);
			}
			else if(strncasecmp(text, "TOP", 3) == 0)
			{
				
				On_Top_Handler(text);
				
			}
			else if(strncasecmp(text,"STAT",4) == 0)
			{
				On_Stat_Handler(text);

			}
			else if(strncasecmp(text,"LIST",4) == 0)
			{
				
				On_List_Handler(text);

			}
			else if(strncasecmp(text,"RETR",4) == 0)
			{
				
				On_Retr_Handler(text);

			}
			else if(strncasecmp(text,"RSET",4) == 0)
			{
				On_Rset_Handler(text);
			}
			else if(strncasecmp(text,"UIDL",4) == 0)
			{
				On_Uidl_Handler(text);
	
			}
			else if(strncasecmp(text,"DELE",4) == 0)
			{
				On_Delete_Handler(text);
			}
			else if(strncasecmp(text,"QUIT",4) == 0)
			{
				
				On_Quit_Handler();
				
				result = FALSE;
			}
			else if(strncasecmp(text,"STLS",4) == 0)
			{
				On_STLS_Handler();
			}
			else
			{
				On_Unrecognized_Command_Handler();
			}
		}
	}
	return result;
}

