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
#include "util/security.h"
#include "smtpclient.h"


SmtpClient::SmtpClient(int sockfd, const char* mx_server_name)
{
	m_mx_server_name = mx_server_name;
	m_sockfd = sockfd;
	m_bStartTLS = FALSE;
	m_bInTLS = FALSE;
    m_ssl = NULL;
    m_ssl_ctx = NULL;
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
    close_ssl(m_ssl, m_ssl_ctx);

	if(m_lssl)
		delete m_lssl;
    
    if(m_lsockfd)
        delete m_lsockfd;
    
	if(m_sockfd)
		close(m_sockfd);
    
  	m_ssl = NULL;
	m_ssl_ctx = NULL;
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
        string ca_crt_root;
        ca_crt_root = CMailBase::m_ca_client_base_dir;
        ca_crt_root += "/";
        ca_crt_root += m_mx_server_name;
        ca_crt_root += "/ca.crt";
        
        string ca_crt_client;
        ca_crt_client = CMailBase::m_ca_client_base_dir;
        ca_crt_client += "/";
        ca_crt_client += m_mx_server_name;
        ca_crt_client += "/client.crt";
        
        string ca_key_client;
        ca_key_client = CMailBase::m_ca_client_base_dir;
        ca_key_client += "/";
        ca_key_client += m_mx_server_name;
        ca_key_client += "/client.key";
        
        string ca_password_file;
        ca_password_file = CMailBase::m_ca_client_base_dir;
        ca_password_file += "/";
        ca_password_file += m_mx_server_name;
        ca_password_file += "/client.pwd";
        
        struct stat file_stat;
        if(stat(ca_crt_root.c_str(), &file_stat) == 0
            && stat(ca_crt_client.c_str(), &file_stat) == 0
            && stat(ca_key_client.c_str(), &file_stat) == 0
            && stat(ca_password_file.c_str(), &file_stat) == 0)
        {
            string ca_password;
            string strPwdEncoded;
            ifstream ca_password_fd(ca_password_file.c_str(), ios_base::binary);
            if(ca_password_fd.is_open())
            {
                getline(ca_password_fd, strPwdEncoded);
                ca_password_fd.close();
                strtrim(strPwdEncoded);
            }
            
            Security::Decrypt(strPwdEncoded.c_str(), strPwdEncoded.length(), ca_password);
                
            if(connect_ssl(m_sockfd,
                ca_crt_root.c_str(),
                ca_crt_client.c_str(),
                ca_password.c_str(),
                ca_key_client.c_str(),
                &m_ssl, &m_ssl_ctx) == FALSE)
            {
                return FALSE;
            }
        }
        else
        {
            if(connect_ssl(m_sockfd, NULL, NULL, NULL, NULL, &m_ssl, &m_ssl_ctx) == FALSE)
            {
                return FALSE;
            }
        }

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

