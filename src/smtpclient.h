/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _SMTP_CLIENT_H
#define _SMTP_CLIENT_H

#define MAX_EMAIL_ADDRESS_LEN	128

class SmtpClient
{
public:
	SmtpClient(int sockfd, const char* mx_server_name);
	virtual~ SmtpClient();
	
	BOOL Do_Auth_Command(string& strmsg);
	BOOL Do_User_Command(char* user, string& strmsg);
	BOOL Do_Password_Command(char* password, string& strmsg);
	BOOL Do_Mail_Command(const char* mail_from, string& strmsg);
	BOOL Do_Rcpt_Command(const char* forward_path, string& strmsg);
	BOOL Do_Finish_Data_Command(string& strmsg);
	BOOL Do_Vrfy_Command(char* user_name, string& strmsg);
	BOOL Do_Expn_Command(char* mail_list, string& strmsg);
	BOOL Do_Send_Command(char* reverse_path, string& strmsg);
	BOOL Do_Soml_Command(char* reverse_path, string& strmsg);
	BOOL Do_Saml_Command(char* reverse_path, string& strmsg);
	BOOL Do_Helo_Command(char* hostname, string& strmsg);
	BOOL Do_Ehlo_Command(char* hostname, string& strmsg);
	BOOL Do_Quit_Command(string& strmsg);
	BOOL Do_Rset_Command(string& strmsg);
	BOOL Do_Noop_Command(string& strmsg);
	BOOL Do_Help_Command(string& strmsg);
	BOOL Do_Turn_Command(string& strmsg);
	BOOL Do_Data_Command(string& strmsg);
	BOOL Do_Dataing_Command(const char* text, int len, string& strmsg);
	BOOL Do_StartTLS_Command(string& strmsg);
	BOOL StartTLS();

protected:
	BOOL RecvReply(char* reply_code, string &strmsg);
	int SendCmd(int sockfd, const char * buf, unsigned int buf_len);
	int m_sockfd;
	linesock* m_lsockfd;
	linessl * m_lssl;
	SSL* m_ssl;
	SSL_CTX* m_ssl_ctx;
	BOOL m_bStartTLS;
	BOOL m_bInTLS;
    string m_mx_server_name;
};
#endif /* _SMTP_CLIENT_H */
