#include "smtp.h"
#include "dns.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "spool.h"
#include "util/md5.h"
#include "service.h"


static char CHAR_TBL[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890=/";

CMailSmtp::CMailSmtp(int sockfd, const char* clientip, StorageEngine* storage_engine, memcached_st * memcached, BOOL isSSL)
{
	m_sockfd = sockfd;
	m_clientip = clientip;

	m_lsockfd = NULL;
	m_lssl = NULL;
	
	m_storageEngine = storage_engine;
	m_memcached = memcached;
	
	m_letter_obj_vector.clear();
	
	m_ssl = NULL;
	m_ssl_ctx = NULL;
	
	m_isSSL = isSSL;
	if(isSSL)
	{	
		int flags = fcntl(m_sockfd, F_GETFL, 0); 
		fcntl(m_sockfd, F_SETFL, flags & ~O_NONBLOCK); 

		X509* client_cert;
		SSL_METHOD* meth;
		SSL_load_error_strings();
		OpenSSL_add_ssl_algorithms();
		meth = (SSL_METHOD*)SSLv23_server_method();
		m_ssl_ctx = SSL_CTX_new(meth);
		if(!m_ssl_ctx)
			return;
		SSL_CTX_set_verify(m_ssl_ctx, SSL_VERIFY_PEER, NULL);
		
		SSL_CTX_load_verify_locations(m_ssl_ctx, m_ca_crt_root.c_str(), NULL);
		if(SSL_CTX_use_certificate_file(m_ssl_ctx, m_ca_crt_server.c_str(), SSL_FILETYPE_PEM) <= 0)
		{
			printf("SSL_CTX_use_certificate_file: %s\n", ERR_error_string(ERR_get_error(),NULL));
			throw new string(ERR_error_string(ERR_get_error(), NULL));
			return;
		}
		SSL_CTX_set_default_passwd_cb_userdata(m_ssl_ctx, (char*)m_ca_password.c_str());

		if(SSL_CTX_use_PrivateKey_file(m_ssl_ctx, m_ca_key_server.c_str(), SSL_FILETYPE_PEM) <= 0)
		{
			printf("SSL_CTX_use_PrivateKey_file: %s\n", ERR_error_string(ERR_get_error(),NULL));
			throw new string(ERR_error_string(ERR_get_error(), NULL));
			return;
		}
		if(!SSL_CTX_check_private_key(m_ssl_ctx))
		{
			printf("SSL_CTX_check_private_key: %s\n", ERR_error_string(ERR_get_error(),NULL));
			throw new string(ERR_error_string(ERR_get_error(), NULL));
			return;
		}
		SSL_CTX_set_cipher_list(m_ssl_ctx,"RC4-MD5");
		SSL_CTX_set_mode(m_ssl_ctx, SSL_MODE_AUTO_RETRY);

		m_ssl = SSL_new(m_ssl_ctx);
		if(!m_ssl)
		{
			throw new string(ERR_error_string(ERR_get_error(), NULL));
			return;
		}
		SSL_set_fd(m_ssl, m_sockfd);
		int err = SSL_accept(m_ssl);
		if(err == -1)
		{
			printf("SSL_accept: %s\n", ERR_error_string(ERR_get_error(),NULL));
			throw new string(ERR_error_string(ERR_get_error(), NULL));
			return;
		}

		if(m_enableclientcacheck)
		{
			X509* client_cert;
			client_cert = SSL_get_peer_certificate (m_ssl);
			if (client_cert != NULL)
			{
				X509_free (client_cert);
			}
			else
			{	
				if(m_ssl)
					SSL_shutdown(m_ssl);
				if(m_ssl)
					SSL_free(m_ssl);
				if(m_ssl_ctx)
					SSL_CTX_free(m_ssl_ctx);
				m_ssl = NULL;
				m_ssl_ctx = NULL;
				throw new string(ERR_error_string(ERR_get_error(), NULL));
				return;
			}
		}

		flags = fcntl(m_sockfd, F_GETFL, 0); 
		fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK); 

		m_lssl = new linessl(m_sockfd, m_ssl);
	}
	else
	{
		int flags = fcntl(m_sockfd, F_GETFL, 0); 
		fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK); 

		m_lsockfd = new linesock(m_sockfd);
	}

	m_status = STATUS_ORIGINAL;
			
	struct sockaddr_in peer_name;
	int peer_name_len = sizeof(struct sockaddr_in);
	getpeername(m_sockfd, (struct sockaddr*)&peer_name,(socklen_t*)&peer_name_len);
	struct hostent* remoteHost;
	int slen = sizeof(peer_name.sin_addr);
	remoteHost = gethostbyaddr((char *)&peer_name.sin_addr, slen, AF_INET);
	if(remoteHost)
	{
		m_clientdomain = remoteHost->h_name;
	}

	TiXmlDocument xmlFileterDoc;
	xmlFileterDoc.LoadFile(CONFIG_FILTER_PATH);
	TiXmlElement * pRootElement = xmlFileterDoc.RootElement();
	if(pRootElement)
	{
		TiXmlNode* pChildNode = pRootElement->FirstChild("filter");
		while(pChildNode)
		{
			if(pChildNode && pChildNode->ToElement())
			{
				FilterHandle mfilter;
				mfilter.dhandle = NULL;
				mfilter.lhandle = NULL;
				mfilter.action = jaTag;
				
				mfilter.lhandle = dlopen(pChildNode->ToElement()->Attribute("libso"), RTLD_NOW);
				
				if(mfilter.lhandle)
				{
					if(strcasecmp(pChildNode->ToElement()->Attribute("action") == NULL ? "" : pChildNode->ToElement()->Attribute("action"), "drop") == 0)
					{
						mfilter.action = jaDrop;	
					}
					else
					{
						mfilter.action = jaTag;
					}
					
					void* (*mfilter_init)();
					mfilter_init = (void*(*)())dlsym(mfilter.lhandle, "mfilter_init");
					const char* errmsg;
					if((errmsg = dlerror()) == NULL)
					{
						mfilter.dhandle = mfilter_init();
						m_filterHandle.push_back(mfilter);
					}
					else
					{
						printf("%s\n", errmsg);
					}
				}
			}
			pChildNode = pChildNode->NextSibling("filter");
		}
	}
	
	for(int x = 0; x < m_filterHandle.size(); x++)
	{
		if(m_filterHandle[x].lhandle)
		{
			void (*mfilter_emaildomain) (void*, const char*, unsigned int);
			mfilter_emaildomain = (void (*)(void*, const char*, unsigned int))dlsym(m_filterHandle[x].lhandle, "mfilter_emaildomain");
			const char* errmsg;
			if((errmsg = dlerror()) == NULL)
			{
				mfilter_emaildomain(m_filterHandle[x].dhandle, m_email_domain.c_str(), m_email_domain.length());
			}
			else
			{
				printf("%s\n", errmsg);
			}
			
			void (*mfilter_clientip) (void*, const char*, unsigned int);
			mfilter_clientip = (void (*)(void*, const char*, unsigned int))dlsym(m_filterHandle[x].lhandle, "mfilter_clientip");
			if((errmsg = dlerror()) == NULL)
			{	
				mfilter_clientip(m_filterHandle[x].dhandle, m_clientip.c_str(), m_clientip.length());
			}
			else
			{
				printf("%s\n", errmsg);
			}
			
			void (*mfilter_clientdomain) (void*, const char*, unsigned int);
			mfilter_clientdomain = (void (*)(void*, const char*, unsigned int))dlsym(m_filterHandle[x].lhandle, "mfilter_clientdomain");
			if((errmsg = dlerror()) == NULL)
			{		
				mfilter_clientdomain(m_filterHandle[x].dhandle, m_clientdomain.c_str(), m_clientdomain.length());
			}
			else
			{
				printf("%s", errmsg);
			}
		}
	}

	On_Service_Ready_Handler();
}

CMailSmtp::~CMailSmtp()
{	
	int vlen = m_letter_obj_vector.size();
	for(int x =0; x< vlen; x++)
	{
		if(m_letter_obj_vector[x]->letter)
		{
			delete m_letter_obj_vector[x]->letter;
		}
		delete m_letter_obj_vector[x];
	}
	m_letter_obj_vector.clear();
	

	for(int x = 0; x < m_filterHandle.size(); x++)
	{
		if(m_filterHandle[x].lhandle)
		{
			void (*mfilter_exit) (void*);
			mfilter_exit = (void (*)(void*))dlsym(m_filterHandle[x].lhandle, "mfilter_exit");
			const char* errmsg;
			if((errmsg = dlerror()) == NULL)
			{
				mfilter_exit(m_filterHandle[x].dhandle);
			}
			else
			{
				printf("%s\n", errmsg);
			}
			dlclose(m_filterHandle[x].lhandle);
		}
	}

	if(m_ssl)
		SSL_shutdown(m_ssl);
	if(m_ssl)
		SSL_free(m_ssl);
	if(m_ssl_ctx)
		SSL_CTX_free(m_ssl_ctx);

	m_ssl = NULL;
	m_ssl_ctx = NULL;

	if(m_sockfd > 0)
	{
		close(m_sockfd);
		m_sockfd = -1;
	}
	
	if(m_lsockfd)
		delete m_lsockfd;

	if(m_lssl)
		delete m_lssl;
	
}

int CMailSmtp::SmtpSend(const char* buf, int len)
{
	//printf("%s", buf);
	if(m_isSSL)
		if(m_ssl)
			return SSLWrite(m_sockfd, m_ssl, buf, len);
		else
			return -1;
	else
		return Send(m_sockfd, buf, len);
}

int CMailSmtp::ProtRecv(char* buf, int len)
{
	if(m_isSSL)
	{
		if(m_ssl)
			return m_lssl->lrecv(buf, len);
		else
			return -1;
	}
	else
	{
		return m_lsockfd->lrecv(buf, len);
	}
}

BOOL CMailSmtp::Parse(char* text)
{
	//printf("%s",text);
	BOOL result = TRUE;
	if((m_status&STATUS_AUTH_STEP1) == STATUS_AUTH_STEP1)
	{
	
		result = On_Supose_Username_Handler(text);

		
	}
	else if( (m_status&STATUS_AUTH_STEP2) == STATUS_AUTH_STEP2)
	{
	
		result = On_Supose_Password_Handler(text);
		
	}
	else if( (m_status&STATUS_SUPOSE_DATAING) == STATUS_SUPOSE_DATAING)
	{
		if(strcmp(text, ".\r\n") == 0)
		{
	
			On_Finish_Data_Handler();

			
		}
		else
		{
			result = On_Supose_Dataing_Handler(text);
		}
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
			if(strncmp(text, "*", 1) == 0)
			{
				On_Cancel_Auth_Handler(text);
			}
			else if(strncasecmp(text,"DATA", 4) == 0)
			{
				if((m_status & STATUS_RCPTED) == STATUS_RCPTED)
				{
					On_Data_Handler(text);

					
				}
				else
					On_Command_Bad_Sequence_Handler();
			}
			else if(strncasecmp(text,"AUTH", 4) == 0)
			{
				if((m_status & STATUS_HELOED) == STATUS_HELOED)
				{
					
					On_Auth_Handler(text);
					
				}
				else
				{
					On_Command_Bad_Sequence_Handler();
				}
			}
			else if(strncasecmp(text,"EHLO", 4) == 0)
			{
				On_Ehlo_Handler(text);
			}
			else if(strncasecmp(text,"EXPN", 4) == 0)
			{
				On_Expn_Handler(text);
			}
			else if(strncasecmp(text,"HELO", 4) == 0)
			{
				if(!On_Helo_Handler(text))
					result = FALSE; 
			}
			else if(strncasecmp(text,"HELP", 4) == 0)
			{
				On_Help_Handler(text);
			}
			else if(strncasecmp(text,"MAIL", 4) == 0)
			{
				if((m_status&STATUS_HELOED) == STATUS_HELOED)
					result = On_Mail_Handler(text);
				else
					On_Command_Bad_Sequence_Handler();
			}
			else if(strncasecmp(text,"NOOP", 4) == 0)
			{
				On_Noop_Handler(text);
			}
			else if(strncasecmp(text,"QUIT", 4) == 0)
			{
				On_Quit_Handler(text);
				result = FALSE; 
			}
			else if(strncasecmp(text,"RCPT", 4) == 0)
			{
				if((m_status & STATUS_MAILED) == STATUS_MAILED)
				{
					result = On_Rcpt_Handler(text);

					
				}
				else
					On_Command_Bad_Sequence_Handler();
			}
			else if(strncasecmp(text,"RSET", 4) == 0)
			{
				On_Rset_Handler(text);
			}
			else if(strncasecmp(text,"SAML", 4) == 0)
			{
				On_Saml_Handler(text);
			}
			else if(strncasecmp(text,"SEND", 4) == 0)
			{
				On_Send_Handler(text);
			}
			else if(strncasecmp(text,"SOML", 4) == 0)
			{
				On_Soml_Handler(text);
			}
			else if(strncasecmp(text,"TURN", 4) == 0)
			{
				On_Turn_Handler(text);
			}
			else if(strncasecmp(text,"VRFY", 4) == 0)
			{
					
				On_Vrfy_Handler(text);

				
			}
			else if(strncasecmp(text,"STARTTLS", 4) == 0)
			{
				On_STARTTLS_Handler();
			}
			else
			{
				On_Unrecognized_Command_Handler();
			}
		}
	}
	return result;
}

BOOL CMailSmtp::On_Mail_Handler(char* text)
{
	strcut(text, "<" , ">", m_mailfrom);
	strtrim(m_mailfrom, "<> ");

	string fromdomain;
	strcut(m_mailfrom.c_str(), "@", NULL, fromdomain);
		
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		if(CMailBase::Is_Local_Domain(fromdomain.c_str()))
		{
			m_status |= STATUS_MAILED;
			On_Request_Okay_Handler();
			return TRUE;
		}
		else
		{
			char cmd[256];
			sprintf(cmd,"553 sorry, that domain is not in my domain list of allowed mailhosts\r\n");
			SmtpSend(cmd,strlen(cmd));
			return FALSE;
		}
	}
	else
	{
		if(CMailBase::Is_Local_Domain(fromdomain.c_str()))
		{
			char cmd[256];
			sprintf(cmd,"553 sorry, that domain is not in my domain list of allowed mailhosts\r\n");
			SmtpSend(cmd,strlen(cmd));
			return FALSE;
		}
		else
		{
			for(int x = 0; x < m_filterHandle.size(); x++)
			{
				if(m_filterHandle[x].lhandle)
				{
					void (*mfilter_mailfrom) (void*, const char*, unsigned int);
					mfilter_mailfrom = (void (*)(void*, const char*, unsigned int))dlsym(m_filterHandle[x].lhandle, "mfilter_mailfrom");
					const char* errmsg;
					if((errmsg = dlerror()) == NULL)
					{
						mfilter_mailfrom(m_filterHandle[x].dhandle, m_mailfrom.c_str(), m_mailfrom.length());
					}
					else
					{
						printf("%s\n", errmsg);
					}
				}
			}
			
			m_status |= STATUS_MAILED;
			On_Request_Okay_Handler();
			return TRUE;
		}
	}

}

void CMailSmtp::On_Cancel_Auth_Handler(char* text)
{
	m_status = STATUS_HELOED;
}

void CMailSmtp::On_Auth_Handler(char* text)
{	
	char cmd[256];
	if(strncasecmp(&text[5],"LOGIN",5) == 0)
	{
		m_authType = AUTH_LOGIN;
		m_status = m_status|STATUS_AUTH_STEP1;

		char cmd[256];
		sprintf(cmd,"334 VXNlcm5hbWU6\r\n");
		SmtpSend(cmd,strlen(cmd));
		
	}
	else if(strncasecmp(&text[5],"PLAIN",5) == 0)
	{
		m_authType = AUTH_PLAIN;
		string strEnoded;
		strcut(&text[11], NULL, "\r\n",strEnoded);		
		int outlen = BASE64_DECODE_OUTPUT_MAX_LEN(strEnoded.length());//strEnoded.length()*4+1;
		char* tmp1 = (char*)malloc(outlen);
		memset(tmp1, 0, outlen);
		
		CBase64::Decode((char*)strEnoded.c_str(), strEnoded.length(), tmp1, &outlen);
		char tmp2[65], tmp3[65];
		memset(tmp2, 0, 65);
		memset(tmp3, 0, 65);
		int x = 0, y = 0;
		for(x = 1; x < (outlen < 65 ? outlen : 65); x++)
		{
			if(tmp1[x] == '\0')
			{
				break;
			}
			else
			{
				tmp2[y] = tmp1[x];
				y++;
			}
		}
		y = 0;
		for(x = x + 1; x < (outlen < 130 ? outlen : 130); x++)
		{
			if(tmp1[x] == '\0')
			{
				break;
			}
			else
			{
				tmp3[y] = tmp1[x];
				y++;
			}
		}
		
		m_username = tmp2;
		m_password = tmp3;
		free(tmp1);

		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		
		if(mailStg->CheckLogin((char*)m_username.c_str(), (char*)m_password.c_str())== 0)
		{
			m_status = m_status|STATUS_AUTHED;
			sprintf(cmd, "235 Authentication successful\r\n");
		}
		else
		{
			push_reject_list(m_isSSL ? stSMTPS : stSMTP, m_clientip.c_str());
			sprintf(cmd, "535 Error: authentication failed\r\n");
		}
		
		SmtpSend(cmd,strlen(cmd));
	}
	else if(strncasecmp(&text[5],"DIGEST-MD5", 10) == 0)
	{
		m_authType = AUTH_DIGEST_MD5;
		
		string strChallenge;
		char cmd[512];
		char nonce[15];
		srand(time(NULL));
		sprintf(nonce, "%c%c%c%08x%c%c%c",
			CHAR_TBL[rand()%(sizeof(CHAR_TBL)-1)],
			CHAR_TBL[rand()%(sizeof(CHAR_TBL)-1)],
			CHAR_TBL[rand()%(sizeof(CHAR_TBL)-1)],
			(unsigned int)time(NULL),
			CHAR_TBL[rand()%(sizeof(CHAR_TBL)-1)],
			CHAR_TBL[rand()%(sizeof(CHAR_TBL)-1)],
			CHAR_TBL[rand()%(sizeof(CHAR_TBL)-1)]);
		sprintf(cmd, "realm=\"%s\",nonce=\"%s\",qop=\"auth\",charset=utf-8,algorithm=md5-sess\n", m_localhostname.c_str(), nonce);
		int outlen  = BASE64_ENCODE_OUTPUT_MAX_LEN(strlen(cmd));//strlen(cmd) *4 + 1;
		char* szEncoded = (char*)malloc(outlen);
		memset(szEncoded, 0, outlen);
		CBase64::Encode(cmd, strlen(cmd), szEncoded, &outlen);
		sprintf(cmd, "334 ");
		SmtpSend(cmd,strlen(cmd));
		SmtpSend(szEncoded, outlen);
		SmtpSend((char*)"\r\n", 2);

		free(szEncoded);
		m_status = m_status|STATUS_AUTH_STEP1;

	}
	else if(strncasecmp(&text[5],"CRAM-MD5", 8) == 0)
	{
		m_authType = AUTH_CRAM_MD5;
		
		char cmd[512];
		srand(time(NULL));
		sprintf(cmd,"<%u%u.%u@%s>", rand()%10000, getpid(), (unsigned int)time(NULL), m_localhostname.c_str());
		m_strDigest = cmd;
		int outlen  = BASE64_ENCODE_OUTPUT_MAX_LEN(strlen(cmd));//strlen(cmd) *4 + 1;
		char* szEncoded = (char*)malloc(outlen);
		memset(szEncoded, 0, outlen);
		CBase64::Encode(cmd, strlen(cmd), szEncoded, &outlen);
		
		sprintf(cmd, "334 %s\r\n", szEncoded);
		free(szEncoded);
		
		SmtpSend(cmd,strlen(cmd));
		m_status = m_status|STATUS_AUTH_STEP2;
	}
	else
	{
		
		sprintf(cmd, "504 Unrecognized Authentication type\r\n");
		SmtpSend(cmd,strlen(cmd));
	}
	
}

BOOL CMailSmtp::On_Supose_Username_Handler(char* text)
{
	if(m_authType == AUTH_LOGIN)
	{
		string username;
		strcut(text, NULL, "\r\n", username);

		int outlen = BASE64_DECODE_OUTPUT_MAX_LEN(username.length());//username.length()*4+1;
		char* tmp = (char*)malloc(outlen);
		memset(tmp,0,outlen);
		CBase64::Decode((char*)username.c_str(), username.length(), tmp, &outlen);
		m_username = tmp;
		free(tmp);
		
		char cmd[64];
		sprintf(cmd,"334 UGFzc3dvcmQ6\r\n");
		SmtpSend(cmd,strlen(cmd));

		m_status = m_status&(~STATUS_AUTH_STEP1);
		m_status = m_status|STATUS_AUTH_STEP2;
		
	}
	else if(m_authType == AUTH_DIGEST_MD5)
	{
		string strEncoded;
		strcut(text, NULL, "\r\n",strEncoded);
		
		int outlen = BASE64_DECODE_OUTPUT_MAX_LEN(strEncoded.length());//strEncoded.length()*4+1;
		char* tmp = (char*)malloc(outlen);
		memset(tmp, 0, outlen);
		CBase64::Decode((char*)strEncoded.c_str(), strEncoded.length(), tmp, &outlen);
		
		string strRealm, strNonce, strCNonce, strDigestUri, strQop, strNc;

		string strRight;
		strcut(tmp, "username=\"", "\"", m_username);

		strcut(tmp, "realm=\"", "\"", strRealm);

		strcut(tmp, "response=", "\n", strRight);
		strcut(strRight.c_str(), NULL, ",", m_strToken);
		
		strcut(tmp, "nonce=\"", "\"", strNonce);
		
		strcut(tmp, "cnonce=\"", "\"", strCNonce);

		strcut(tmp, "digest-uri=\"", "\"", strDigestUri);

		strcut(tmp, "qop=", "\n", strRight);
		strcut(strRight.c_str(), NULL, ",", strQop);

		strcut(tmp, "nc=", "\n", strRight);
		strcut(strRight.c_str(), NULL, ",", strNc);
		free(tmp);

		string strpwd;
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return FALSE;
		}
		
		mailStg->GetPassword(m_username.c_str(), strpwd);
		
		
		unsigned char digest1[16];
		string strMD5src1;
		strMD5src1 = m_username + ":" + strRealm + ":" + strpwd;
		MD5_CTX md5;
		md5.MD5Update((unsigned char*)strMD5src1.c_str(), strMD5src1.length());
		md5.MD5Final(digest1);

		unsigned char digest2[16];
		string strMD5src2_0 = ":";
		strMD5src2_0 += strNonce + ":" + strCNonce;
		
		char* szMD5src2_1 = (char*)malloc(16 + strMD5src2_0.length() + 1);
		memcpy(szMD5src2_1, digest1, 16);
		memcpy(&szMD5src2_1[16], strMD5src2_0.c_str(), strMD5src2_0.length() + 1);

		md5.MD5Update((unsigned char*)szMD5src2_1, 16 + strMD5src2_0.length());
		md5.MD5Final(digest2);
		char szMD5Dst2[33];
		memset(szMD5Dst2, 0, 33);
		sprintf(szMD5Dst2,
				"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				digest2[0], digest2[1], digest2[2], digest2[3], digest2[4], digest2[5], digest2[6], digest2[7],
				digest2[8], digest2[9], digest2[10], digest2[11], digest2[12], digest2[13], digest2[14], digest2[15]);

		////////////////////////////////////////////////////////////////////////////////////////////////////
		//Client
		unsigned char digest3_1[16];
		string strMD5src3_1 = "AUTHENTICATE:";
		strMD5src3_1 += strDigestUri;
		if((strQop == "auth-int") || (strQop == "auth-conf"))
			strMD5src3_1 += ":00000000000000000000000000000000";
		
		md5.MD5Update((unsigned char*)strMD5src3_1.c_str(), strMD5src3_1.length());
		md5.MD5Final(digest3_1);
		char szMD5Dst3_1[33];
		memset(szMD5Dst3_1, 0, 33);
		sprintf(szMD5Dst3_1,
				"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				digest3_1[0], digest3_1[1], digest3_1[2], digest3_1[3], digest3_1[4], digest3_1[5], digest3_1[6], digest3_1[7],
				digest3_1[8], digest3_1[9], digest3_1[10], digest3_1[11], digest3_1[12], digest3_1[13], digest3_1[14], digest3_1[15]);

		unsigned char digest4_1[16];
		string strMD5src4_1;
		strMD5src4_1 = szMD5Dst2;
		strMD5src4_1 += ":";
		strMD5src4_1 += strNonce;
		strMD5src4_1 += ":";
		strMD5src4_1 += strNc;
		strMD5src4_1 += ":";
		strMD5src4_1 += strCNonce;
		strMD5src4_1 += ":";
		strMD5src4_1 += strQop;
		strMD5src4_1 += ":";
		strMD5src4_1 += szMD5Dst3_1;
		
		md5.MD5Update((unsigned char*)strMD5src4_1.c_str(), strMD5src4_1.length());
		md5.MD5Final(digest4_1);
		
		char szMD5Dst4_1[33];
		memset(szMD5Dst4_1, 0, 33);
		sprintf(szMD5Dst4_1,
				"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				digest4_1[0], digest4_1[1], digest4_1[2], digest4_1[3], digest4_1[4], digest4_1[5], digest4_1[6], digest4_1[7],
				digest4_1[8], digest4_1[9], digest4_1[10], digest4_1[11], digest4_1[12], digest4_1[13], digest4_1[14], digest4_1[15]);
		m_strTokenVerify = szMD5Dst4_1;

		if(m_strTokenVerify == m_strToken)
		{
			////////////////////////////////////////////////////////////////////////////////////
			// server
			unsigned char digest3_2[16];
			//at server site, no need "AUTHENTICATE"
			string strMD5src3_2 = ":";
			strMD5src3_2 += strDigestUri;
			if((strQop == "auth-int") || (strQop == "auth-conf"))
				strMD5src3_2 += ":00000000000000000000000000000000";
			
			md5.MD5Update((unsigned char*)strMD5src3_2.c_str(), strMD5src3_2.length());
			md5.MD5Final(digest3_2);
			char szMD5Dst3_2[33];
			memset(szMD5Dst3_2, 0, 33);
			sprintf(szMD5Dst3_2,
					"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
					digest3_2[0], digest3_2[1], digest3_2[2], digest3_2[3], digest3_2[4], digest3_2[5], digest3_2[6], digest3_2[7],
					digest3_2[8], digest3_2[9], digest3_2[10], digest3_2[11], digest3_2[12], digest3_2[13], digest3_2[14], digest3_2[15]);

			unsigned char digest4_2[16];
			string strMD5src4_2;
			strMD5src4_2 = szMD5Dst2;
			strMD5src4_2 += ":";
			strMD5src4_2 += strNonce;
			strMD5src4_2 += ":";
			strMD5src4_2 += strNc;
			strMD5src4_2 += ":";
			strMD5src4_2 += strCNonce;
			strMD5src4_2 += ":";
			strMD5src4_2 += strQop;
			strMD5src4_2 += ":";
			strMD5src4_2 += szMD5Dst3_2;
			
			md5.MD5Update((unsigned char*)strMD5src4_2.c_str(), strMD5src4_2.length());
			md5.MD5Final(digest4_2);
			
			char szMD5Dst4_2[33];
			memset(szMD5Dst4_2, 0, 33);
			sprintf(szMD5Dst4_2,
					"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
					digest4_2[0], digest4_2[1], digest4_2[2], digest4_2[3], digest4_2[4], digest4_2[5], digest4_2[6], digest4_2[7],
					digest4_2[8], digest4_2[9], digest4_2[10], digest4_2[11], digest4_2[12], digest4_2[13], digest4_2[14], digest4_2[15]);
			
			char cmd[512];
			sprintf(cmd, "rspauth=%s", szMD5Dst4_2);
			outlen = BASE64_ENCODE_OUTPUT_MAX_LEN(strlen(cmd));//strlen(cmd)*4+1;
			tmp = (char*)malloc(outlen);
			memset(tmp, 0, outlen);
			CBase64::Encode((char*)cmd, strlen(cmd), tmp, &outlen);

			sprintf(cmd,"235 ");
			SmtpSend(cmd, strlen(cmd));
			SmtpSend(tmp, outlen);
			SmtpSend((char*)"\r\n", 2);

			free(tmp);
		}
		m_status = m_status&(~STATUS_AUTH_STEP1);
		m_status = m_status|STATUS_AUTH_STEP2;
		
	}
	return TRUE;
}

BOOL CMailSmtp::On_Supose_Password_Handler(char* text)
{
	if(m_authType == AUTH_LOGIN)
	{
		string password;
		strcut(text, NULL, "\r\n",password);
		//printf("password:%s\n", password.c_str());
		
		int outlen = BASE64_DECODE_OUTPUT_MAX_LEN(password.length());//password.length()*4+1;
		char* tmp = (char*)malloc(outlen);
		memset(tmp, 0, outlen);
		CBase64::Decode((char*)password.c_str(), password.length(), tmp, &outlen);
		m_password = tmp;
		free(tmp);
	}
	else if(m_authType == AUTH_DIGEST_MD5)
	{
		
	}
	else if(m_authType == AUTH_CRAM_MD5)
	{
		string strEncoded;
		strcut(text, NULL, "\r\n",strEncoded);
		
		int outlen = BASE64_DECODE_OUTPUT_MAX_LEN(strEncoded.length());//strEncoded.length()*4+1;
		char* tmp = (char*)malloc(outlen);
		memset(tmp, 0, outlen);
		CBase64::Decode((char*)strEncoded.c_str(), strEncoded.length(), tmp, &outlen);
		strcut(tmp, NULL, " ", m_username);
		strcut(tmp, " ", NULL, m_strToken);
		free(tmp);
	}
	m_status = m_status&(~STATUS_AUTH_STEP2);

	return On_Supose_Checking_Handler();
}

BOOL CMailSmtp::On_Supose_Checking_Handler()
{
	MailStorage* mailStg;
	StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
	if(!mailStg)
	{
		printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
		return FALSE;
	}
	char cmd[256];
	if(m_authType == AUTH_LOGIN)
	{
		if(mailStg->CheckLogin(m_username.c_str(),m_password.c_str()) == 0)
		{
			sprintf(cmd,"235 Authentication successful.\r\n");
			SmtpSend(cmd,strlen(cmd));
			m_status = m_status|STATUS_AUTHED;
			return TRUE;
		}
		else
		{
			push_reject_list(m_isSSL ? stSMTPS : stSMTP, m_clientip.c_str());
			sprintf(cmd,"535 Authentication Failed.\r\n");
			SmtpSend(cmd,strlen(cmd));
			return FALSE;
		}
	}
	else if(m_authType == AUTH_CRAM_MD5)
	{
		string strpwd;
		if(mailStg->GetPassword(m_username.c_str(), strpwd) == 0)
		{
			unsigned char digest[16];
			HMAC_MD5((unsigned char*)m_strDigest.c_str(), m_strDigest.length(), (unsigned char*)strpwd.c_str(), strpwd.length(), digest);
			char szMD5dst[33];
			memset(szMD5dst, 0, 33);
			sprintf(szMD5dst,
				"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7],
				digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15]);
			
			if(strcasecmp(szMD5dst, m_strToken.c_str()) == 0)
			{
				sprintf(cmd,"235 Authentication successful.\r\n");
				SmtpSend(cmd,strlen(cmd));
				m_status = m_status|STATUS_AUTHED;
				return TRUE;
			}
			else
			{
				push_reject_list(m_isSSL ? stSMTPS : stSMTP, m_clientip.c_str());
				sprintf(cmd,"535 Authentication Failed.\r\n");
				SmtpSend(cmd,strlen(cmd));
				return FALSE;
			}
		}
		else
		{
			push_reject_list(m_isSSL ? stSMTPS : stSMTP, m_clientip.c_str());
			sprintf(cmd,"535 Authentication Failed.\r\n");
			SmtpSend(cmd,strlen(cmd));
			return FALSE;
		}
	}
	else if(m_authType == AUTH_DIGEST_MD5)
	{
		if(m_strToken == m_strTokenVerify)
		{
			sprintf(cmd,"235 Authentication successful.\r\n");
			SmtpSend(cmd,strlen(cmd));
			m_status = m_status|STATUS_AUTHED;
			return TRUE;
		}
		else
		{
			push_reject_list(m_isSSL ? stSMTPS : stSMTP, m_clientip.c_str());
			char cmd[128];
			sprintf(cmd,"535 Authentication Failed.\r\n");
			SmtpSend(cmd,strlen(cmd));
			return FALSE;
		}
	}
	else
	{
		push_reject_list(m_isSSL ? stSMTPS : stSMTP, m_clientip.c_str());
		return FALSE;
	}
	
}
	
BOOL CMailSmtp::On_Rcpt_Handler(char* text)
{
	
	string rcpt_to;
	strcut(text, ":", "\r\n", rcpt_to);
	strtrim(rcpt_to, "<> ");

	string rcptto_domain;
	strcut(rcpt_to.c_str(), "@", NULL, rcptto_domain);
	
	//filter the rcpt address
	for(int x = 0; x < m_filterHandle.size(); x++)
	{
		if(m_filterHandle[x].lhandle)
		{
			void (*mfilter_rcptto) (void*, const char*, unsigned int);
			mfilter_rcptto = (void (*)(void*, const char*, unsigned int))dlsym(m_filterHandle[x].lhandle, "mfilter_rcptto");
			const char* errmsg;
			if((errmsg = dlerror()) == NULL)
			{
				mfilter_rcptto(m_filterHandle[x].dhandle, rcpt_to.c_str(), rcpt_to.length());
			}
			else
			{
				printf("%s\n", errmsg);
			}
		}
	}

	
	
	string user_or_group;
	strcut(rcpt_to.c_str(), NULL, "@", user_or_group);

	MailStorage* mailStg;
	StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);

	if(!mailStg)
	{
		printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
		return FALSE;
	}
	
	//To Local domain
	if(CMailBase::Is_Local_Domain(rcptto_domain.c_str()))
	{
		if((mailStg->VerifyUser(user_or_group.c_str()) != 0)&&(mailStg->VerifyGroup(user_or_group.c_str()) != 0))
		{
			push_reject_list(m_isSSL ? stSMTPS : stSMTP, m_clientip.c_str());
			
			char cmd[256];
			sprintf(cmd,"553 sorry, that domain is not in my deliver domain list\r\n");
			SmtpSend(cmd,strlen(cmd));
			return FALSE;
		}
		else
		{
			if(mailStg->VerifyGroup(user_or_group.c_str()) == 0)
			{
				char name[256];
				vector<User_Info> uitbl;
				mailStg->ListMemberOfGroup(user_or_group.c_str(), uitbl);
				int vlen = uitbl.size();

				for(int x = 0; x < vlen; x++)
				{
					string member_mail("");
					member_mail = member_mail + uitbl[x].username + "@" + rcptto_domain;
					
					LETTER_OBJ*  letter_obj = new LETTER_OBJ;
					letter_obj->letter = NULL;
					letter_obj->letter_info.mail_to = member_mail.c_str();//new string(member_mail.c_str());
					m_letter_obj_vector.push_back(letter_obj);
				}
			}
			else
			{
				LETTER_OBJ* lette_obj = new LETTER_OBJ;
				lette_obj->letter = NULL;
				lette_obj->letter_info.mail_to = rcpt_to.c_str();//rcpt_to = new string(rcpt_to.c_str());
				m_letter_obj_vector.push_back(lette_obj);
			}
		}
	}
	//To Extern domain
	else
	{
		if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
		{
			//Relay the mail
			LETTER_OBJ* lette_obj = new LETTER_OBJ;
			lette_obj->letter = NULL;
			lette_obj->letter_info.mail_to = rcpt_to.c_str();//new string(rcpt_to.c_str());
			m_letter_obj_vector.push_back(lette_obj);
		}
		else
		{
			if(!m_enablerelay)
			{
				push_reject_list(m_isSSL ? stSMTPS : stSMTP, m_clientip.c_str());
				
				char cmd[256];
		        sprintf(cmd,"553 sorry, that domain is not in my deliver domain list\r\n");
		        SmtpSend(cmd,strlen(cmd));
		        return FALSE;
			}
			else
			{
				//Relay the mail
				LETTER_OBJ* lette_obj = new LETTER_OBJ;
				lette_obj->letter = NULL;
				lette_obj->letter_info.mail_to = rcpt_to.c_str(); //new string(rcpt_to.c_str());
				m_letter_obj_vector.push_back(lette_obj);
			}
		}
	}
	
	m_status |= STATUS_RCPTED;
	On_Request_Okay_Handler();
	return TRUE;

}

void CMailSmtp::On_Data_Handler(char* text)
{
	MailStorage* mailStg;
	StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);

	if(!mailStg)
	{
		printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
		return;
	}
	
	char cmd[256];
	sprintf(cmd,"354 send the mail data, end with . \r\n");
	SmtpSend(cmd, strlen(cmd));
	
	m_status = m_status|STATUS_SUPOSE_DATAING;

	char uid[256];
	int vlen = m_letter_obj_vector.size();
	for(int x =0; x< vlen; x++)
	{
		//string local_domain;
		string rcptto_domain;

		int DirID;
		
		strcut(m_letter_obj_vector[x]->letter_info.mail_to.c_str(),"@", NULL, rcptto_domain);
		unsigned int mstatus = MSG_ATTR_RECENT;

		srand(time(NULL));
		sprintf(uid, "%08x_%08x_%016lx_%08x", time(NULL), getpid(), pthread_self(), CMailBase::m_global_uid);
		CMailBase::m_global_uid++;
		
		if(CMailBase::Is_Local_Domain(rcptto_domain.c_str()))
		{
			if(m_status & STATUS_AUTHED == STATUS_AUTHED)
			{
				string mailfromid, rcpttoid;		
				strcut(m_mailfrom.c_str(), NULL, "@", mailfromid);
				strcut(m_letter_obj_vector[x]->letter_info.mail_to.c_str(), NULL, "@", rcpttoid);
				mailStg->GetInboxID(rcpttoid.c_str(), DirID);
				
				unsigned long long usermaxsize;
				if(mailStg->GetUserSize(rcpttoid.c_str(), usermaxsize) == -1)
				{
					usermaxsize = 5000*1024;
				}
						
				m_letter_obj_vector[x]->letter = new MailLetter(m_memcached, uid, usermaxsize /*mailStg, m_mailfrom.c_str(),
					m_letter_obj_vector[x].rcpt_to->c_str(), mtLocal, uid, DirID, mstatus, (unsigned int)time(NULL), usermaxsize*/);

				m_letter_obj_vector[x]->letter_info.mail_from = m_mailfrom.c_str();
				m_letter_obj_vector[x]->letter_info.mail_type = mtLocal;
				m_letter_obj_vector[x]->letter_info.mail_uniqueid = uid;
				m_letter_obj_vector[x]->letter_info.mail_dirid = DirID;
				m_letter_obj_vector[x]->letter_info.mail_status = mstatus;
				m_letter_obj_vector[x]->letter_info.mail_time = time(NULL);
				m_letter_obj_vector[x]->letter_info.user_maxsize = usermaxsize;
				m_letter_obj_vector[x]->letter_info.mail_id = -1;
			}
			else
			{
				string rcpttoid;
				strcut(m_letter_obj_vector[x]->letter_info.mail_to.c_str(), NULL, "@", rcpttoid);
				mailStg->GetInboxID(rcpttoid.c_str(), DirID);

				unsigned long long usermaxsize;
				if(mailStg->GetUserSize(rcpttoid.c_str(), usermaxsize) == -1)
				{
					usermaxsize = 5000*1024;
				}

				
				m_letter_obj_vector[x]->letter = new MailLetter(m_memcached, uid, usermaxsize /*mailStg, m_mailfrom.c_str(), 
					m_letter_obj_vector[x].rcpt_to->c_str(), mtLocal, uid, DirID, mstatus, (unsigned int)time(NULL), usermaxsize*/);

				m_letter_obj_vector[x]->letter_info.mail_from = m_mailfrom.c_str();
				m_letter_obj_vector[x]->letter_info.mail_type = mtLocal;
				m_letter_obj_vector[x]->letter_info.mail_uniqueid = uid;
				m_letter_obj_vector[x]->letter_info.mail_dirid = DirID;
				m_letter_obj_vector[x]->letter_info.mail_status = mstatus;
				m_letter_obj_vector[x]->letter_info.mail_time = time(NULL);
				m_letter_obj_vector[x]->letter_info.user_maxsize = usermaxsize;
				m_letter_obj_vector[x]->letter_info.mail_id = -1;
				
			}
		}
		else
		{
			if(m_status & STATUS_AUTHED == STATUS_AUTHED)
			{
				string mailfrom_id;	
				strcut(m_mailfrom.c_str(), NULL, "@", mailfrom_id);

				unsigned long long usermaxsize;
				if(mailStg->GetUserSize(mailfrom_id.c_str(), usermaxsize) == -1)
				{
					usermaxsize = 5000*1024;
				}
				
				m_letter_obj_vector[x]->letter = new MailLetter(m_memcached, uid, usermaxsize /*mailStg, 
				m_mailfrom.c_str(), m_letter_obj_vector[x].rcpt_to->c_str(), mtExtern, uid, -1, mstatus, time(NULL), usermaxsize*/);

				m_letter_obj_vector[x]->letter_info.mail_from = m_mailfrom.c_str();
				m_letter_obj_vector[x]->letter_info.mail_type = mtExtern;
				m_letter_obj_vector[x]->letter_info.mail_uniqueid = uid;
				m_letter_obj_vector[x]->letter_info.mail_dirid = -1;
				m_letter_obj_vector[x]->letter_info.mail_status = mstatus;
				m_letter_obj_vector[x]->letter_info.mail_time = time(NULL);
				m_letter_obj_vector[x]->letter_info.user_maxsize = usermaxsize;
				m_letter_obj_vector[x]->letter_info.mail_id = -1;
			}
			else
			{
				//middle transport
				unsigned long long usermaxsize;
				if(mailStg->GetUserSize(m_mailfrom.c_str(), usermaxsize) == -1)
				{
					usermaxsize = 5000*1024;
				}
				m_letter_obj_vector[x]->letter = new MailLetter(m_memcached, uid, usermaxsize /*mailStg, 
				m_mailfrom.c_str(), m_letter_obj_vector[x].rcpt_to->c_str(), mtExtern, uid, -1, mstatus, time(NULL), usermaxsize*/);

				m_letter_obj_vector[x]->letter_info.mail_from = m_mailfrom.c_str();
				m_letter_obj_vector[x]->letter_info.mail_type = mtExtern;
				m_letter_obj_vector[x]->letter_info.mail_uniqueid = uid;
				m_letter_obj_vector[x]->letter_info.mail_dirid = -1;
				m_letter_obj_vector[x]->letter_info.mail_status = mstatus;
				m_letter_obj_vector[x]->letter_info.mail_time = time(NULL);
				m_letter_obj_vector[x]->letter_info.user_maxsize = usermaxsize;
				m_letter_obj_vector[x]->letter_info.mail_id = -1;
				
			}
		}

		string strTime;
		OutTimeString(time(NULL), strTime);
		char header[1024];
		sprintf(header,"Received: from %s ([%s]) by %s (eRisemail) with SMTP id %s for <%s>;"
			" %s\r\n",
			m_clientdomain == "" ? "unknown" : m_clientdomain.c_str(),
			m_clientip.c_str(),
			m_localhostname.c_str(),
			m_letter_obj_vector[x]->letter_info.mail_uniqueid.c_str(),
			m_letter_obj_vector[x]->letter_info.mail_to.c_str(),
			strTime.c_str());
		m_letter_obj_vector[x]->letter->Write(header, strlen(header));
	}
}

BOOL CMailSmtp::On_Supose_Dataing_Handler(char* text)
{
	for(int x = 0; x < m_filterHandle.size(); x++)
	{
		if(m_filterHandle[x].lhandle)
		{
			void (*mfilter_data) (void*, const char*, unsigned int);
			mfilter_data = (void (*)(void*, const char*, unsigned int))dlsym(m_filterHandle[x].lhandle, "mfilter_data");
			const char* errmsg;
			if((errmsg = dlerror()) == NULL)
			{
				mfilter_data(m_filterHandle[x].dhandle, text, strlen(text));
			}
			else
			{
				printf("%s\n", errmsg);
			}
		}
	}
		
	int vlen = m_letter_obj_vector.size();
	for(int x = 0; x < vlen; x++)
	{
		if(m_letter_obj_vector[x]->letter->Write(text, strlen(text)) < 0)
		{
			return FALSE;
		}
	}
	return TRUE;
}

void CMailSmtp::On_Finish_Data_Handler()
{
	MailStorage* mailStg;
	StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);

	if(!mailStg)
	{
		printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
		return;
	}
	
	m_status = m_status&(~STATUS_SUPOSE_DATAING);
	m_status = m_status|STATUS_DATAED;

	int isjunk = -1;
	JunkAction action = jaTag;
	
	for(int x = 0; x < m_filterHandle.size(); x++)
	{
		if(m_filterHandle[x].lhandle)
		{
			void (*mfilter_result) (void*, int*);
			mfilter_result = (void (*)(void*, int*))dlsym(m_filterHandle[x].lhandle, "mfilter_result");
			const char* errmsg;
			if((errmsg = dlerror()) == NULL)
			{
				mfilter_result(m_filterHandle[x].dhandle, &isjunk);
				if(isjunk != -1)
				{
					action = m_filterHandle[x].action;
					break;
				}
			}
			else
			{
				printf("%s\n", errmsg);
			}
		}
	}
	
	int vlen = m_letter_obj_vector.size();
	for(int x=0; x< vlen; x++)
	{
		if(isjunk != -1 && m_letter_obj_vector[x]->letter_info.mail_type == mtLocal)
		{
			if(action == jaTag)
			{
				int DirID;
				string rcpttoid;
				strcut(m_letter_obj_vector[x]->letter_info.mail_to.c_str(), NULL, "@", rcpttoid);
				
				mailStg->GetJunkID(rcpttoid.c_str(), DirID);
				
				unsigned long mail_sumsize = m_letter_obj_vector[x]->letter->GetSize();
				m_letter_obj_vector[x]->letter_info.mail_dirid = DirID;
				if(mail_sumsize > 0)
				{
					m_letter_obj_vector[x]->letter->SetOK();
				}
			}
		}
		else
		{
			Level_Info linfo;
			linfo.attachsizethreshold = 5000*1024;
			linfo.boxmaxsize = 5000*1024*1024*1024;
			linfo.enableaudit = eaFalse;
			linfo.ldefault = ldFalse;
			linfo.mailmaxsize = 5000*1024;
			linfo.mailsizethreshold = 5000*1024;
			
			if(mailStg->GetUserLevel(m_username.c_str(), linfo) == -1)
			{
				mailStg->GetDefaultLevel(linfo);
				
			}

			unsigned long mail_sumsize = m_letter_obj_vector[x]->letter->GetSize();
			
			if(m_letter_obj_vector[x]->letter_info.mail_type == mtExtern && linfo.enableaudit == eaTrue)
			{
				m_letter_obj_vector[x]->letter->Flush();
				
				unsigned long attach_count, attach_sumsize;
				m_letter_obj_vector[x]->letter->GetAttachSummary(attach_count, attach_sumsize);
				
				if(mail_sumsize >= linfo.mailsizethreshold|| attach_sumsize >= linfo.attachsizethreshold)
				{
					m_letter_obj_vector[x]->letter_info.mail_status |= MSG_ATTR_UNAUDITED;
				}
			}
			
			if(mail_sumsize > 0)
			{
				m_letter_obj_vector[x]->letter->SetOK();
			}
		}
		m_letter_obj_vector[x]->letter->Close();
		if(m_letter_obj_vector[x]->letter->isOK())
		{
			mailStg->InsertMailIndex(m_letter_obj_vector[x]->letter_info.mail_from.c_str(), m_letter_obj_vector[x]->letter_info.mail_to.c_str(), 
						m_letter_obj_vector[x]->letter_info.mail_time,
						m_letter_obj_vector[x]->letter_info.mail_type, m_letter_obj_vector[x]->letter_info.mail_uniqueid.c_str(), 
						m_letter_obj_vector[x]->letter_info.mail_dirid, m_letter_obj_vector[x]->letter_info.mail_status,
						m_letter_obj_vector[x]->letter->GetEmlName(), m_letter_obj_vector[x]->letter->GetSize(), m_letter_obj_vector[x]->letter_info.mail_id);
		}
		
	}
	
	On_Request_Okay_Handler();
}

void CMailSmtp::On_Vrfy_Handler(char* text)
{
	MailStorage* mailStg;
	StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);

	if(!mailStg)
	{
		printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
		return;
	}
	
	string tmp;
	strcut(text, " ", "\r\n", tmp);
		
	char cmd[1024];
	if(mailStg->VerifyUser(tmp.c_str()) == 0)
	{
		sprintf(cmd,"250 <%s@%s>.\r\n",tmp.c_str(),m_email_domain.c_str());
	}
	else
	{
		sprintf(cmd,"550 String does not match anything.\r\n");
	}
	SmtpSend(cmd,strlen(cmd));
}



void CMailSmtp::On_Expn_Handler(char* text)
{
	On_Unimplemented_Command_Handler();
}



void CMailSmtp::On_Send_Handler(char* text)
{
	On_Unimplemented_Command_Handler();
}



void CMailSmtp::On_Soml_Handler(char* text)
{
	On_Unimplemented_Command_Handler();
}



void CMailSmtp::On_Saml_Handler(char* text)
{
	On_Unimplemented_Command_Handler();
}



BOOL CMailSmtp::On_Helo_Handler(char* text)
{
	string client;
	strcut(text, " ", "\r\n", client);

	m_status = m_status|STATUS_HELOED;

	if(Check_Helo_Domain((char*)client.c_str()))
	{
		//m_status = m_status|STATUS_AUTHED;
		On_Request_Okay_Handler();
		return TRUE;
	}
	else
		return FALSE;
	
}



void CMailSmtp::On_Ehlo_Handler(char* text)
{
	m_status = m_status|STATUS_HELOED;
	
	char cmd[256];
	if(m_enablesmtptls)
		sprintf(cmd,"250-%s\r\n250-STARTTLS\r\n250-PIPELINING\r\n250-AUTH LOGIN PLAIN CRAM-MD5 DIGEST-MD5\r\n250-8BITMIME\r\n250 OK\r\n", m_email_domain.c_str());
	else
		sprintf(cmd,"250-%s\r\n250-PIPELINING\r\n250-AUTH LOGIN PLAIN CRAM-MD5 DIGEST-MD5\r\n250-8BITMIME\r\n250 OK\r\n", m_email_domain.c_str());

	SmtpSend(cmd,strlen(cmd));
}



void CMailSmtp::On_Quit_Handler(char* text)
{
	On_Service_Close_Handler();
}



void CMailSmtp::On_Rset_Handler(char* text)
{
	m_status = m_status&(~STATUS_RCPTED);
	m_status = m_status&(~STATUS_MAILED);
	
	int vlen =  m_letter_obj_vector.size();

	for(int x = 0; x < vlen; x++)
	{
		if(m_letter_obj_vector[x]->letter)
		{
			delete m_letter_obj_vector[x]->letter;
		}
		delete m_letter_obj_vector[x];
	}
	m_letter_obj_vector.clear();
	On_Request_Okay_Handler();
}



void CMailSmtp::On_Help_Handler(char* text)
{
	char reply[256];
	sprintf(reply,"211 System is Ready\r\n");
	SmtpSend(reply,strlen(reply));
}



void CMailSmtp::On_Noop_Handler(char* text)
{
	On_Request_Okay_Handler();
}



void CMailSmtp::On_Turn_Handler(char* text)
{
	On_Unimplemented_Command_Handler();
}



void CMailSmtp::On_Unrecognized_Command_Handler()
{
	char reply[256];
	sprintf(reply,"500 Syntax error, command unrecognized.\r\n");
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Unimplemented_Command_Handler()
{
	char reply[256];
	sprintf(reply,"502 Command not implemented.\r\n");
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Service_Unavailable_Handler()
{
	char reply[256];
	sprintf(reply,"421 %s Service not available, closing transmission channel\r\n",m_localhostname.c_str());
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Mailbox_Unavailable_Handler()
{
	char reply[256];
	sprintf(reply,"450 Mailbox unavailable.\r\n");
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Local_Error_Handler()
{
	char reply[256];
	sprintf(reply,"451 local error in processing.\r\n");
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Insufficient_Storage_Handler()
{
	char reply[256];
	sprintf(reply,"452 Insufficient system storage.\r\n");
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Unimplemented_parameter_Handler()
{
	char reply[256];
	sprintf(reply,"504 Command parameter not implemented.\r\n");
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Exceeded_Storage_Handler()
{
	char reply[256];
	sprintf(reply,"552 Exceeded storage allocation.\r\n");
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Transaction_Failed_Handler()
{
	char reply[256];
	sprintf(reply,"554 Transaction Failed.\r\n");
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Service_Ready_Handler()
{
	char reply[256];
	sprintf(reply,"220 %s SMTP service is ready by eRisemail-%s powered by Uplusware\r\n",m_localhostname.c_str(), m_sw_version.c_str());
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Service_Error_Handler()
{
	char reply[256];
	sprintf(reply,"421 %s Service has error\r\n",m_localhostname.c_str());
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Service_Close_Handler()
{
	char reply[256];
	sprintf(reply,"221 %s Service closing transmission channel\r\n",m_localhostname.c_str());
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Request_Okay_Handler()
{
	char reply[256];
	sprintf(reply,"250 Requested mail action okay, completed\r\n");
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Command_Bad_Sequence_Handler()
{
	char reply[256];
	sprintf(reply,"503 Bad sequence of commands\r\n");
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Not_Local_User_Handler()
{
	char forward_path[256];
	char reply[256];
	sprintf(reply,"551 User not local; please try %s\r\n",forward_path);
	SmtpSend(reply,strlen(reply));
}

void CMailSmtp::On_Exchange_Mail_Handler()
{
	return;
}

void CMailSmtp::On_STARTTLS_Handler()
{
	char cmd[256];
	if(!m_enablesmtptls)
	{
		sprintf(cmd,"500 TLS is disabled\r\n");
		SmtpSend(cmd,strlen(cmd));
		return;
	}
	
	if(m_isSSL == TRUE)
	{
		sprintf(cmd,"550 Command not permitted when TLS active\r\n");
		SmtpSend(cmd,strlen(cmd));
		return;
	}
	else
	{
		sprintf(cmd,"220 Ready to start TLS\r\n");
		SmtpSend(cmd,strlen(cmd));
	}
	m_isSSL = TRUE;
	if(m_isSSL)
	{	
		int flags = fcntl(m_sockfd, F_GETFL, 0); 
		fcntl(m_sockfd, F_SETFL, flags & (~O_NONBLOCK)); 
		
		SSL_METHOD* meth;
		SSL_load_error_strings();
		OpenSSL_add_ssl_algorithms();
		meth = (SSL_METHOD*)TLSv1_server_method();
		m_ssl_ctx = SSL_CTX_new(meth);
		if(!m_ssl_ctx)
		{
			m_isSSL = FALSE;
			return;
		}
		SSL_CTX_set_verify(m_ssl_ctx, SSL_VERIFY_PEER, NULL);
		
		SSL_CTX_load_verify_locations(m_ssl_ctx, m_ca_crt_root.c_str(), NULL);
		if(SSL_CTX_use_certificate_file(m_ssl_ctx, m_ca_crt_server.c_str(), SSL_FILETYPE_PEM) <= 0)
		{
			printf("SSL_CTX_use_certificate_file: %s\n", ERR_error_string(ERR_get_error(),NULL));
			m_isSSL = FALSE;
			if(m_ssl)
				SSL_shutdown(m_ssl);
			if(m_ssl)
				SSL_free(m_ssl);
			if(m_ssl_ctx)
				SSL_CTX_free(m_ssl_ctx);
			m_ssl = NULL;
			m_ssl_ctx = NULL;
			return;
		}
		m_ssl_ctx->default_passwd_callback_userdata = (char*)m_ca_password.c_str();
		if(SSL_CTX_use_PrivateKey_file(m_ssl_ctx, m_ca_key_server.c_str(), SSL_FILETYPE_PEM) <= 0)
		{
			printf("SSL_CTX_use_PrivateKey_file: %s\n", ERR_error_string(ERR_get_error(),NULL));
			m_isSSL = FALSE;
			if(m_ssl)
				SSL_shutdown(m_ssl);
			if(m_ssl)
				SSL_free(m_ssl);
			if(m_ssl_ctx)
				SSL_CTX_free(m_ssl_ctx);
			m_ssl = NULL;
			m_ssl_ctx = NULL;
			return;
		}
		if(!SSL_CTX_check_private_key(m_ssl_ctx))
		{
			printf("SSL_CTX_check_private_key: %s\n", ERR_error_string(ERR_get_error(),NULL));
			m_isSSL = FALSE;
			if(m_ssl)
				SSL_shutdown(m_ssl);
			if(m_ssl)
				SSL_free(m_ssl);
			if(m_ssl_ctx)
				SSL_CTX_free(m_ssl_ctx);
			m_ssl = NULL;
			m_ssl_ctx = NULL;
			return;
		}
		SSL_CTX_set_cipher_list(m_ssl_ctx,"RC4-MD5");
		SSL_CTX_set_mode(m_ssl_ctx, SSL_MODE_AUTO_RETRY);

		m_ssl = SSL_new(m_ssl_ctx);
		if(!m_ssl)
		{
			printf("m_ssl invaild\n");
			m_isSSL = FALSE;
			if(m_ssl)
				SSL_shutdown(m_ssl);
			if(m_ssl)
				SSL_free(m_ssl);
			if(m_ssl_ctx)
				SSL_CTX_free(m_ssl_ctx);
			m_ssl = NULL;
			m_ssl_ctx = NULL;
			return;
		}
		if(SSL_set_fd(m_ssl, m_sockfd) <= 0)
		{
			printf("SSL_set_fd: %s\n", ERR_error_string(ERR_get_error(),NULL));
		}
		
		if(SSL_accept(m_ssl) <= 0)
		{
			printf("SSL_accept: %s\n", ERR_error_string(ERR_get_error(),NULL));
			m_isSSL = FALSE;
			if(m_ssl)
				SSL_shutdown(m_ssl);
			if(m_ssl)
				SSL_free(m_ssl);
			if(m_ssl_ctx)
				SSL_CTX_free(m_ssl_ctx);
			m_ssl = NULL;
			m_ssl_ctx = NULL;
			return;
		}
		
		if(m_enableclientcacheck)
		{
			X509* client_cert;
			client_cert = SSL_get_peer_certificate (m_ssl);
			if (client_cert != NULL)
			{
				X509_free (client_cert);
			}
			else
			{
				printf("SSL_get_peer_certificate: %s\n", ERR_error_string(ERR_get_error(),NULL));
				
				m_isSSL = FALSE;
				if(m_ssl)
					SSL_shutdown(m_ssl);
				if(m_ssl)
					SSL_free(m_ssl);
				if(m_ssl_ctx)
					SSL_CTX_free(m_ssl_ctx);
				m_ssl = NULL;
				m_ssl_ctx = NULL;
				return;
			}
		}
		flags = fcntl(m_sockfd, F_GETFL, 0); 
		fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK); 

		m_lssl = new linessl(m_sockfd, m_ssl);

	}
}

BOOL CMailSmtp::Check_Helo_Domain(const char* domain)
{
	if(m_enablesmtphostnamecheck == FALSE)
		return TRUE;
	else
	{
		char realip[32];
		struct hostent *hostEnt = NULL;
		hostEnt = gethostbyname(domain);
		if(hostEnt != NULL)
		{
			strcpy(realip, (char*)inet_ntoa(*(struct in_addr *)(hostEnt->h_addr_list[0])));
			if( strcmp(realip, m_clientip.c_str()) == 0)
				return TRUE;
			else
				return FALSE;
		}
		else
		{
			return FALSE;
		}
	}
}



