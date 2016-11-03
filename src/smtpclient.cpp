/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "base.h"
#include "util/general.h"
#include "smtpclient.h"


SmtpClient::SmtpClient(int sockfd)
{
	m_sockfd = -1;
	m_sockfd = sockfd;
	m_bStartTLS = FALSE;
	m_bInTLS = FALSE;
	m_lssl = NULL;
	m_lsockfd = new linesock(sockfd);
	char welstr[4096];
	while(1)
	{
		memset(welstr,0,4096);
		int res = m_lsockfd->lrecv(welstr,4095);
		if(res <= 0)
		{
			break;
		}
		if(welstr[3] == ' ')
		{
			break;
		}
	}
}

SmtpClient::~SmtpClient()
{
	if(m_ssl)
		SSL_shutdown(m_ssl);
	if(m_ssl)
		SSL_free(m_ssl);
	if(m_ssl_ctx)
		SSL_CTX_free(m_ssl_ctx);

	m_ssl = NULL;
	m_ssl_ctx = NULL;

	if(m_lssl)
		delete m_lssl;

	if(m_sockfd)
		close(m_sockfd);
}

BOOL SmtpClient::Do_Auth_Command(string& strmsg)
{
	char cmd[256];
	sprintf(cmd,"AUTH LOGIN\r\n");
	
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"334",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_User_Command(char* user, string& strmsg)
{
	if(strlen(user) > MAX_USERNAME_LEN)
	{
		return FALSE;
	}
	int outlen = BASE64_ENCODE_OUTPUT_MAX_LEN(strlen(user));//strlen(user)*4 + 1;
	char* cmd = (char*)malloc(outlen);
	memset(cmd, 0, outlen);
	CBase64::Encode(user,strlen(user),cmd,&outlen);
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"334",3) == 0)))
	{
		free(cmd);
		return TRUE;
	}
	else
	{
		free(cmd);
		return FALSE;
	}
}

BOOL SmtpClient::Do_Password_Command(char* password,string& strmsg)
{
	if(strlen(password) > MAX_PASSWORD_LEN)
	{
		return FALSE;
	}
	int outlen = BASE64_ENCODE_OUTPUT_MAX_LEN(strlen(password));//strlen(password)*4 + 1;
	char* cmd = (char*)malloc(outlen);
	memset(cmd, 0, outlen);
	CBase64::Encode(password,strlen(password),cmd,&outlen);

	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"235",3) == 0)))
	{
		free(cmd);
		return TRUE;
	}
	else
	{
		free(cmd);
		return FALSE;
	}
}

BOOL SmtpClient::Do_Mail_Command(const char* mail_from, string& strmsg)
{
	if(strlen(mail_from) > MAX_EMAIL_ADDRESS_LEN)
		return FALSE;
	char cmd[256];
	sprintf(cmd,"MAIL FROM: <%s>\r\n", mail_from);
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd,strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Rcpt_Command(const char* forward_path, string& strmsg)
{
	if(strlen(forward_path) > MAX_EMAIL_ADDRESS_LEN)
		return FALSE;
	char cmd[256];
	
	sprintf(cmd,"RCPT TO: <%s>\r\n", forward_path);
	if((SendCmd(m_sockfd, cmd, strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Data_Command(string& strmsg)
{
	char cmd[256];
	sprintf(cmd,"DATA\r\n");
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"354",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Dataing_Command(const char* text, int len, string& strmsg)
{
	if(SendCmd(m_sockfd,text, len) == 0)
		return TRUE;
	else
	{
		strmsg = "Connection is broken when sending the mail.";
		return FALSE;
	}
}

BOOL SmtpClient::Do_Finish_Data_Command(string& strmsg)
{
	char cmd[256];
	sprintf(cmd,"\r\n.\r\n");
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Vrfy_Command(char* user_name, string& strmsg)
{
	char cmd[256];
	sprintf(cmd,"VRFY %s\r\n",user_name);
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Expn_Command(char* mail_list, string& strmsg)
{
	char cmd[256];
	sprintf(cmd,"EXPN %s\r\n",mail_list);
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Send_Command(char* reverse_path, string& strmsg)
{
	char cmd[256];
	sprintf(cmd,"SEND FROM:<%s>\r\n",reverse_path);
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Soml_Command(char* reverse_path, string& strmsg)
{
	char cmd[256];
	sprintf(cmd,"SOML FROM:<%s>\r\n",reverse_path);
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Saml_Command(char* reverse_path, string& strmsg)
{
	char cmd[256];
	sprintf(cmd,"SAML FROM:<%s>\r\n",reverse_path);
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Helo_Command(char* hostname, string& strmsg)
{
	char cmd[256];
	if((hostname == NULL)||(strcmp(hostname,"") == 0))
	{
		char temphostname[256];
		gethostname(temphostname,255);
		
		sprintf(cmd,"HELO %s\r\n",temphostname);
	}
	else
	{
		sprintf(cmd,"HELO %s\r\n",hostname);
	}
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Ehlo_Command(char* hostname, string& strmsg)
{
	char cmd[256];
	if((hostname == NULL)||(strcmp(hostname,"") == 0))
	{
		char temphostname[256];
		gethostname(temphostname,255);
		sprintf(cmd,"EHLO %s\r\n",temphostname);
	}
	else
	{
		sprintf(cmd,"EHLO %s\r\n",hostname);
	}
	
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		if(strmsg.find("STARTTLS") != string::npos)
		{
			m_bStartTLS = TRUE;
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_StartTLS_Command(string& strmsg)
{
	char cmd[256];
	sprintf(cmd,"STARTTLS\r\n");
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"220",3) == 0)))
	{
		//Rollback to block mode
		int flags = fcntl(m_sockfd, F_GETFL, 0); 
		fcntl(m_sockfd, F_SETFL, flags & (~O_NONBLOCK));

		SSL_METHOD* meth;
		SSL_load_error_strings();
		OpenSSL_add_ssl_algorithms();
		meth = (SSL_METHOD*)TLSv1_client_method();
		m_ssl_ctx = SSL_CTX_new(meth);

		if(!m_ssl_ctx)
		{
			return FALSE;
		}
		m_ssl = SSL_new(m_ssl_ctx);
		if(!m_ssl)
		{
			printf("m_ssl invaild\n");
			if(m_ssl)
				SSL_shutdown(m_ssl);
			if(m_ssl)
				SSL_free(m_ssl);
			if(m_ssl_ctx)
				SSL_CTX_free(m_ssl_ctx);
			m_ssl = NULL;
			m_ssl_ctx = NULL;
			return FALSE;
		}
		
		SSL_set_fd(m_ssl, m_sockfd);
		
		if(SSL_connect(m_ssl) <= 0)
		{
			printf("SSL_connect: %s\n", ERR_error_string(ERR_get_error(),NULL));
			if(m_ssl)
				SSL_shutdown(m_ssl);
			if(m_ssl)
				SSL_free(m_ssl);
			if(m_ssl_ctx)
				SSL_CTX_free(m_ssl_ctx);
			m_ssl = NULL;
			m_ssl_ctx = NULL;
			return FALSE;
		}

		flags = fcntl(m_sockfd, F_GETFL, 0); 
		fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK); 

		m_lssl = new linessl(m_sockfd, m_ssl);

		m_bInTLS = TRUE;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Rset_Command(string& strmsg)
{
	char cmd[256];
	sprintf(cmd,"RSET\r\n");
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Help_Command(string& strmsg)
{
	char cmd[256];
	sprintf(cmd,"HELP\r\n");
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Noop_Command(string& strmsg)
{
	char cmd[256];
	sprintf(cmd,"NOOP\r\n");
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Turn_Command(string& strmsg)
{
	char cmd[256];
	sprintf(cmd,"TURN\r\n");
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL SmtpClient::Do_Quit_Command(string& strmsg)
{
	char cmd[256];
	sprintf(cmd,"QUIT\r\n");
	if((SendCmd(m_sockfd,cmd,strlen(cmd)) == 0)&&(RecvReply(cmd, strmsg))&&((strncmp(cmd,"250",3) == 0)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

int SmtpClient::SendCmd(int sockfd, const char * buf, unsigned int buf_len)
{
	//printf(buf);
	if(m_bInTLS)
		if(m_ssl)
			return SSLWrite(m_sockfd, m_ssl, buf, buf_len);
		else
			return -1;
	else
		return Send(m_sockfd, buf, buf_len);
}

BOOL SmtpClient::RecvReply(char* reply_code, string &strmsg)
{
	int n_recv = 0;
	strmsg = "";
	char str[4096];
	while(1)
	{
		memset(str, 0, 4096);
		
		if(m_bInTLS)
		{
			n_recv = m_lssl->lrecv(str, 4095);
		}
		else
		{
			n_recv = m_lsockfd->lrecv(str, 4095);
		}
		if(n_recv < 0)
		{
			return FALSE;
		}
		else
		{
			strmsg += str;
			if(str[3] == ' ')
			{
				reply_code[0] = str[0];
				reply_code[1] = str[1];
				reply_code[2] = str[2];
				reply_code[3] = '\0';
				return TRUE;
			}
		}
	}
}

BOOL SmtpClient::StartTLS()
{
	return m_bStartTLS;
}

