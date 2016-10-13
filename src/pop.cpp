#include "util/general.h"
#include "pop.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef _WITH_GSSAPI_ 
    #include <gss.h>
#endif /* _WITH_GSSAPI_ */
#include "util/md5.h"
#include "service.h"

static char CHAR_TBL[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890=/";

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
	/* printf("%s", buf); */
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
	srandom(time(NULL));
	sprintf(cmd,"<%lu%lu%lu.%lu@%s>", time(NULL), getpid(), pthread_self(), random(), m_localhostname.c_str());
	
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
#ifdef _WITH_GSSAPI_     
    sprintf(cmd,"SASL PLAIN CRAM-MD5 DIGEST-MD5 GSSAPI\r\n");
#else
    sprintf(cmd,"SASL PLAIN CRAM-MD5 DIGEST-MD5\r\n");
#endif /* _WITH_GSSAPI_ */    
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
    
    strtrim(m_username);
    strtrim(m_strToken);
    
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
							
				Letter = new MailLetter(CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
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

			Letter = new MailLetter(CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
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
    strtrim(m_username);
        
    if(m_authType == POP_AUTH_USER)
    {
        m_status = m_status&(~STATUS_AUTH_STEP1);
        
        char cmd[256];
        sprintf(cmd,"+OK core mail\r\n");
        PopSend(cmd,strlen(cmd));
    }
    else if(m_authType == POP_AUTH_DIGEST_MD5)
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
        strtrim(m_username);
		strcut(tmp, "realm=\"", "\"", strRealm);
        strtrim(strRealm);
		strcut(tmp, "response=", "\n", strRight);
		strcut(strRight.c_str(), NULL, ",", m_strToken);
		strtrim(m_strToken);
		strcut(tmp, "nonce=\"", "\"", strNonce);
		strtrim(strNonce);
		strcut(tmp, "cnonce=\"", "\"", strCNonce);
        strtrim(strCNonce);
		strcut(tmp, "digest-uri=\"", "\"", strDigestUri);
        strtrim(strDigestUri);
		strcut(tmp, "qop=", "\n", strRight);
		strcut(strRight.c_str(), NULL, ",", strQop);
        strtrim(strQop);
		strcut(tmp, "nc=", "\n", strRight);
		strcut(strRight.c_str(), NULL, ",", strNc);
        strtrim(strNc);
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

			sprintf(cmd,"+OK ");
			PopSend(cmd, strlen(cmd));
			PopSend(tmp, outlen);
			PopSend((char*)"\r\n", 2);

			free(tmp);
		}
		m_status = m_status&(~STATUS_AUTH_STEP1);
		m_status = m_status|STATUS_AUTH_STEP2;
		
	}
	return TRUE;
}

BOOL CMailPop::On_Supose_Password_Handler(char* text)
{
    strcut(text, " ", "\r\n", m_password);
    strtrim(m_password);
    
    if(m_authType == POP_AUTH_USER)
	{
        m_status = m_status&(~STATUS_AUTH_STEP2);
        return On_Supose_Checking_Handler();
    }
/*    
    else if(m_authType == POP_AUTH_DIGEST_MD5)
	{
		
	}
*/
	else if(m_authType == POP_AUTH_CRAM_MD5)
	{
		string strEncoded;
		strcut(text, NULL, "\r\n",strEncoded);
		strtrim(strEncoded);
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
    if(m_authType == POP_AUTH_USER)
	{
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
                
                Letter = new MailLetter(CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
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
    else if(m_authType == POP_AUTH_CRAM_MD5)
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
				sprintf(cmd,"+OK Authentication successful.\r\n");
				PopSend(cmd,strlen(cmd));
				m_status = m_status|STATUS_AUTHED;
				return TRUE;
			}
			else
			{
				push_reject_list(m_isSSL ? stSMTPS : stSMTP, m_clientip.c_str());
				sprintf(cmd,"-ERR Authentication Failed.\r\n");
				PopSend(cmd,strlen(cmd));
				return FALSE;
			}
		}
		else
		{
			push_reject_list(m_isSSL ? stSMTPS : stSMTP, m_clientip.c_str());
			sprintf(cmd,"-ERR Authentication Failed.\r\n");
			PopSend(cmd,strlen(cmd));
			return FALSE;
		}
	}
	else if(m_authType == POP_AUTH_DIGEST_MD5)
	{
		if(m_strToken == m_strTokenVerify)
		{
			sprintf(cmd,"+OK Authentication successful.\r\n");
			PopSend(cmd,strlen(cmd));
			m_status = m_status|STATUS_AUTHED;
			return TRUE;
		}
		else
		{
			push_reject_list(m_isSSL ? stSMTPS : stSMTP, m_clientip.c_str());
			char cmd[128];
			sprintf(cmd,"-ERR Authentication Failed.\r\n");
			PopSend(cmd,strlen(cmd));
			return FALSE;
		}
	}
	else
	{
		push_reject_list(m_isSSL ? stSMTPS : stSMTP, m_clientip.c_str());
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
        strtrim(strIndex);
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
			
				Letter = new MailLetter(CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
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
			
				Letter = new MailLetter(CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
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
        strtrim(strIndex);
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
        strtrim(strIndex);
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
        strtrim(strIndex);
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
				
			Letter = new MailLetter(CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
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
		strtrim(strIndex);
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
			
			Letter = new MailLetter(CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
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

void CMailPop::On_Auth_Handler(char* text)
{	
	char cmd[256];
	if(strncasecmp(&text[5],"PLAIN",5) == 0)
	{
		m_authType = POP_AUTH_PLAIN;
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
			sprintf(cmd, "+OK Authentication successful\r\n");
		}
		else
		{
			push_reject_list(m_isSSL ? stSMTPS : stSMTP, m_clientip.c_str());
			sprintf(cmd, "-ERR Error: authentication failed\r\n");
		}
		
		PopSend(cmd,strlen(cmd));
	}
	else if(strncasecmp(&text[5],"DIGEST-MD5", 10) == 0)
	{
		m_authType = POP_AUTH_DIGEST_MD5;
		
		string strChallenge;
		char cmd[512];
		char nonce[15];
		
		sprintf(nonce, "%c%c%c%08x%c%c%c",
			CHAR_TBL[random()%(sizeof(CHAR_TBL)-1)],
			CHAR_TBL[random()%(sizeof(CHAR_TBL)-1)],
			CHAR_TBL[random()%(sizeof(CHAR_TBL)-1)],
			(unsigned int)time(NULL),
			CHAR_TBL[random()%(sizeof(CHAR_TBL)-1)],
			CHAR_TBL[random()%(sizeof(CHAR_TBL)-1)],
			CHAR_TBL[random()%(sizeof(CHAR_TBL)-1)]);
		sprintf(cmd, "realm=\"%s\",nonce=\"%s\",qop=\"auth\",charset=utf-8,algorithm=md5-sess\n", m_localhostname.c_str(), nonce);
		int outlen  = BASE64_ENCODE_OUTPUT_MAX_LEN(strlen(cmd));//strlen(cmd) *4 + 1;
		char* szEncoded = (char*)malloc(outlen);
		memset(szEncoded, 0, outlen);
		CBase64::Encode(cmd, strlen(cmd), szEncoded, &outlen);
		sprintf(cmd, "+OK ");
		PopSend(cmd,strlen(cmd));
		PopSend(szEncoded, outlen);
		PopSend((char*)"\r\n", 2);

		free(szEncoded);
		m_status = m_status|STATUS_AUTH_STEP1;

	}
	else if(strncasecmp(&text[5],"CRAM-MD5", 8) == 0)
	{
		m_authType = POP_AUTH_CRAM_MD5;
		
		char cmd[512];
		
		sprintf(cmd,"<%u%u.%u@%s>", random()%10000, getpid(), (unsigned int)time(NULL), m_localhostname.c_str());
		m_strDigest = cmd;
		int outlen  = BASE64_ENCODE_OUTPUT_MAX_LEN(strlen(cmd));//strlen(cmd) *4 + 1;
		char* szEncoded = (char*)malloc(outlen);
		memset(szEncoded, 0, outlen);
		CBase64::Encode(cmd, strlen(cmd), szEncoded, &outlen);
		
		sprintf(cmd, "+OK %s\r\n", szEncoded);
		free(szEncoded);
		
		PopSend(cmd,strlen(cmd));
		m_status = m_status|STATUS_AUTH_STEP2;
	}
#ifdef _WITH_GSSAPI_ 
    else if(strncasecmp(&text[5],"GSSAPI", 6) == 0)
    {
        m_authType = POP_AUTH_GSSAPI;
        PopSend("+OK\r\n", strlen("+OK\r\n"));
        
        OM_uint32 maj_stat, min_stat;
        
        gss_name_t server_name = GSS_C_NO_NAME;
        gss_cred_id_t server_creds;
        gss_buffer_desc buf_desc;
        string str_buf_desc = "pop@";
        str_buf_desc += CMailBase::m_localhostname;
        buf_desc.value = (char *) str_buf_desc.c_str();
        buf_desc.length = str_buf_desc.length();
  
        maj_stat = gss_import_name (&min_stat, &buf_desc,
			      GSS_C_NT_HOSTBASED_SERVICE, &server_name);
        if (GSS_ERROR (maj_stat))
        {
            sprintf(cmd,"-ERR NO User Logged Failed\r\n");
			PopSend(cmd, strlen(cmd));
            return;
        }
        
        gss_OID_set oid_set;
        maj_stat = gss_create_empty_oid_set(&min_stat, &oid_set);
        if (GSS_ERROR (maj_stat))
        {
            sprintf(cmd,"-ERR NO User Logged Failed\r\n");
			PopSend(cmd, strlen(cmd));
            return;
        }
        
        maj_stat = gss_add_oid_set_member (&min_stat, GSS_KRB5, &oid_set);
        if (GSS_ERROR (maj_stat))
        {
            maj_stat = gss_release_oid_set(&min_stat, &oid_set);
            sprintf(cmd,"-ERR NO User Logged Failed\r\n");
			PopSend(cmd, strlen(cmd));
            return ;
        }
        maj_stat = gss_acquire_cred (&min_stat, server_name, 0,
			       oid_set, GSS_C_ACCEPT,
			       &server_creds, NULL, NULL);
        if (GSS_ERROR (maj_stat))
        {
            maj_stat = gss_release_oid_set(&min_stat, &oid_set);
            sprintf(cmd,"-ERR NO User Logged Failed\r\n");
			PopSend(cmd, strlen(cmd));
            return;
        }
        maj_stat = gss_release_oid_set(&min_stat, &oid_set);
        
        if (GSS_ERROR (maj_stat))
        {
            sprintf(cmd,"-ERR NO User Logged Failed\r\n");
			PopSend(cmd, strlen(cmd));
            return;
        }
        gss_ctx_id_t context_hdl = GSS_C_NO_CONTEXT;
        gss_name_t client_name = GSS_C_NO_NAME;
        gss_OID mech_type;
        gss_buffer_desc input_token = GSS_C_EMPTY_BUFFER;
        gss_buffer_desc output_token = GSS_C_EMPTY_BUFFER;
        OM_uint32 ret_flags;
        OM_uint32 time_rec;
        gss_cred_id_t delegated_cred_handle;
        do {
            string str_line = "";
            char recv_buf[4096];
            while(str_line.find("\n") == std::string::npos)
            {
                int result = ProtRecv(recv_buf, 4095);
                if(result <= 0)
                    return;
                recv_buf[result] = '\0';
                str_line += recv_buf;
            }
            
            int len_encode = BASE64_DECODE_OUTPUT_MAX_LEN(str_line.length());
            char* tmp_decode = (char*)malloc(len_encode);
            memset(tmp_decode, 0, len_encode);
            
            int outlen_decode;
            CBase64::Decode((char*)str_line.c_str(), str_line.length(), tmp_decode, &outlen_decode);
            input_token.length = outlen_decode;
            input_token.value = tmp_decode;
            maj_stat = gss_accept_sec_context(&min_stat,
                                            &context_hdl,
                                            server_creds,
                                            &input_token,
                                            GSS_C_NO_CHANNEL_BINDINGS,
                                            &client_name,
                                            &mech_type,
                                            &output_token,
                                            &ret_flags,
                                            &time_rec,
                                            &delegated_cred_handle);
            free(tmp_decode);
            
            if (GSS_ERROR(maj_stat))
            {
                sprintf(cmd,"-ERR NO User Logged Failed\r\n");
                PopSend(cmd, strlen(cmd));
                return;
            }
            
            if(!gss_oid_equal(mech_type, GSS_KRB5))
            {
                sprintf(cmd,"-ERR NO User Logged Failed\r\n");
                PopSend(cmd, strlen(cmd));
                return;
            }
                  
            if (output_token.length != 0)
            {
                int len_decode = BASE64_ENCODE_OUTPUT_MAX_LEN(output_token.length);
                char* tmp_encode = (char*)malloc(len_decode + 2);
                memset(tmp_encode, 0, len_decode);
                tmp_encode[0] = '+OK';
                tmp_encode[1] = ' ';
                int outlen_encode;
                CBase64::Encode((char*)output_token.value, output_token.length, tmp_encode + 2, &outlen_encode);
        
                PopSend(tmp_encode, outlen_encode + 2);
                gss_release_buffer(&min_stat, &output_token);
                free(tmp_encode);
            }
            if (GSS_ERROR(maj_stat))
            {
                if (context_hdl != GSS_C_NO_CONTEXT)
                    gss_delete_sec_context(&min_stat, &context_hdl, GSS_C_NO_BUFFER);
                sprintf(cmd,"-ERR NO User Logged Failed\r\n");
                PopSend(cmd, strlen(cmd));
                return;
            };
            
            if(maj_stat != GSS_S_COMPLETE)
            {
                if (context_hdl != GSS_C_NO_CONTEXT)
                    gss_delete_sec_context(&min_stat, &context_hdl, GSS_C_NO_BUFFER);
            }
        } while (maj_stat & GSS_S_CONTINUE_NEEDED);
        
        /* empty data, only status code*/
        string str_line = "";
        char recv_buf[4096];
        while(str_line.find("\n") == std::string::npos)
        {
            int result = ProtRecv(recv_buf, 4095);
            if(result <= 0)
                return;
            recv_buf[result] = '\0';
            str_line += recv_buf;
        }
        
        OM_uint32 sec_data = 0;
        gss_buffer_desc input_message_buffer1, output_message_buffer1;
        input_message_buffer1.length = 4;
        input_message_buffer1.value = &sec_data;
        int conf_state;
        maj_stat = gss_wrap (&min_stat, context_hdl, 0, 0, &input_message_buffer1, &conf_state, &output_message_buffer1);
        if (GSS_ERROR(maj_stat))
        {
            sprintf(cmd,"-ERR NO User Logged Failed\r\n");
            PopSend(cmd, strlen(cmd));
            return;
        }
        
        int len_decode = BASE64_ENCODE_OUTPUT_MAX_LEN(output_message_buffer1.length);
        char* tmp_encode = (char*)malloc(len_decode + 2);
        memset(tmp_encode, 0, len_decode);
        tmp_encode[0] = '+OK';
        tmp_encode[1] = ' ';
        int outlen_encode;
        CBase64::Encode((char*)output_message_buffer1.value, output_message_buffer1.length, tmp_encode + 2, &outlen_encode);

        PopSend(tmp_encode, outlen_encode + 2);
        gss_release_buffer(&min_stat, &output_message_buffer1);
        free(tmp_encode);
        
        str_line = "";
        while(str_line.find("\n") == std::string::npos)
        {
            int result = ProtRecv(recv_buf, 4095);
            if(result <= 0)
                return;
            recv_buf[result] = '\0';
            str_line += recv_buf;
        }
        
        int len_encode = BASE64_DECODE_OUTPUT_MAX_LEN(str_line.length());
        char* tmp_decode = (char*)malloc(len_encode);
        memset(tmp_decode, 0, len_encode);
        
        gss_buffer_desc input_message_buffer2, output_message_buffer2;
        gss_qop_t qop_state;
        int outlen_decode;
        CBase64::Decode((char*)str_line.c_str(), str_line.length(), tmp_decode, &outlen_decode);
        input_message_buffer2.length = outlen_decode;
        input_message_buffer2.value = tmp_decode;
        
        maj_stat = gss_unwrap (&min_stat, context_hdl, &input_message_buffer2, &output_message_buffer2, &conf_state, &qop_state);
        
        free(tmp_decode);
        if (GSS_ERROR(maj_stat))
        {
            sprintf(cmd,"-ERR NO User Logged Failed\r\n");
            PopSend(cmd, strlen(cmd));
            return;
        }
        
        if (context_hdl != GSS_C_NO_CONTEXT)
            gss_delete_sec_context(&min_stat, &context_hdl, GSS_C_NO_BUFFER);
        
        sprintf(cmd,"+OK OK GSSAPI authentication successful\r\n");
        PopSend(cmd, strlen(cmd));
        
        m_status = m_status|STATUS_AUTHED;
    }
#endif /* _WITH_GSSAPI_ */
	else
	{	
		sprintf(cmd, "-ERR Unrecognized Authentication type\r\n");
		PopSend(cmd,strlen(cmd));
	}
	
}

BOOL CMailPop::Parse(char* text)
{
	/* printf("[%s]\n", text); */
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
                m_authType = POP_AUTH_USER;
				return On_Supose_Username_Handler(text);
			}
			else if(strncasecmp(text,"PASS",4) == 0)
			{
				result = On_Supose_Password_Handler(text);				
			}
            else if(strncasecmp(text, "AUTH", 4) == 0)
			{
				On_Auth_Handler(text);	
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

