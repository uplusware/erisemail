/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "imap.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "util/base64.h"
#include "util/md5.h"
#include "letter.h"
#include "tinyxml/tinyxml.h"
#include "mime.h"
#include "service.h"

static const char* USER_PRI_TBL[] = { "READ-ONLY", "READ-WRITE", NULL };

static char CHAR_TBL[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890=/";

CMailImap::CMailImap(int sockfd, SSL * ssl, SSL_CTX * ssl_ctx, const char* clientip,
    StorageEngine* storage_engine, memcached_st * memcached, BOOL isSSL)
{
	m_sockfd = sockfd;
    m_ssl = ssl;

    m_ssl_ctx = ssl_ctx;

    m_bSTARTTLS = FALSE;

	m_clientip = clientip;
	m_lsockfd = NULL;
	m_lssl = NULL;
	
	m_storageEngine = storage_engine;
	m_memcached = memcached;
	
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

	On_Service_Ready();
}

CMailImap::~CMailImap()
{
	if(m_bSTARTTLS)
	    close_ssl(m_ssl, m_ssl_ctx);

	if(m_lsockfd)
		delete m_lsockfd;
	if(m_lssl)
		delete m_lssl;	
}

int CMailImap::ImapSend(const char* buf, int len)
{
	//printf("%s", buf);
	if(m_ssl)
		return SSLWrite(m_sockfd, m_ssl, buf, len, CMailBase::m_connection_idle_timeout);
	else
		return Send( m_sockfd, buf, len, CMailBase::m_connection_idle_timeout);
		
}

int CMailImap::ImapRecv(char* buf, int len)
{
	if(m_ssl)
		return m_lssl->drecv(buf, len, CMailBase::m_connection_idle_timeout);
	else
		return m_lsockfd->drecv(buf, len, CMailBase::m_connection_idle_timeout);
		
}

int CMailImap::ProtRecv(char* buf, int len)
{
	if(m_ssl)
		return m_lssl->lrecv(buf, len, CMailBase::m_connection_idle_timeout);
	else
		return m_lsockfd->lrecv(buf,len, CMailBase::m_connection_idle_timeout);
}


void CMailImap::On_Unknown(char* text)
{
	char cmd[512];
	string strTag;
	strcut(text, NULL, " ", strTag);
	
	sprintf(cmd, "%s BAD command unknown or arguments invailid\r\n", strTag.c_str());
	ImapSend( cmd, strlen(cmd));
}

void CMailImap::On_Service_Ready()
{
    srandom(time(NULL));
	char cmd[1024];
	sprintf(cmd, "* OK %s IMAP4rev1 Server is ready by eRisemail-%s powered by Uplusware\r\n", m_localhostname.c_str(), m_sw_version.c_str());
	ImapSend(cmd, strlen(cmd));

}

void CMailImap::On_Capability(char* text)
{
	char cmd[1024];
	string strTag;
	strcut(text, NULL, " ", strTag);
#ifdef _WITH_GSSAPI_	
    sprintf(cmd, "* CAPABILITY IMAP4rev1 STARTTLS AUTH=CRAM-MD5 AUTH=DIGEST-MD5 %s AUTH=GSSAPI\r\n",
        m_ca_verify_client ? "AUTH=EXTERNAL" : "");
#else
    sprintf(cmd, "* CAPABILITY IMAP4rev1 STARTTLS AUTH=CRAM-MD5 AUTH=DIGEST-MD5 %s\r\n",
        m_ca_verify_client ? "AUTH=EXTERNAL" : "");
    
#endif /* _WITH_GSSAPI_ */    
	ImapSend(cmd, strlen(cmd));
    sprintf(cmd, "%s OK CAPABILITY completed\r\n", strTag.c_str());
	ImapSend(cmd, strlen(cmd));
}

void CMailImap::On_Noop(char* text)
{	
	char cmd[1024];
	string strTag;
	strcut(text, NULL, " ", strTag);
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		if((m_status & STATUS_SELECT) == STATUS_SELECT)
		{			
			MailStorage* mailStg;
			StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
			if(!mailStg)
			{
				printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
				return;
			}
			mailStg->ListMailByDir((char*)m_username.c_str(), m_maillisttbl, (char*)m_strSelectedDir.c_str());
			
			unsigned int nExist = 0;
			unsigned int nRecent = 0;
			unsigned int nUnseen = 0;
			unsigned int firstUnseen = 0;
			nExist = m_maillisttbl.size();
			for(int i = 0; i < m_maillisttbl.size(); i++)
			{
				if( (time(NULL) - m_maillisttbl[i].mtime)  > RECENT_MSG_TIME )
				{
					nRecent++;
				}
				if( (m_maillisttbl[i].mstatus & MSG_ATTR_SEEN) != MSG_ATTR_SEEN )
				{
					if(firstUnseen == 0)
						firstUnseen = i;
					nUnseen++;
				}
			}
			
			sprintf(cmd, 
				"* %u EXISTS\r\n"
				"* %u RECENT\r\n"
				"* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)\r\n"
				"%s OK NOOP completed\r\n",
				nExist, nRecent, strTag.c_str());
		}
		else
		{
			sprintf(cmd, "%s OK NOOP Complated\r\n", strTag.c_str());
		}
	}
	else
	{
		sprintf(cmd, "%s OK NOOP completed\r\n", strTag.c_str());	
	}
	ImapSend(cmd, strlen(cmd));
}

void CMailImap::On_Logout(char* text)
{
	char cmd[1024];
	string strTag;
	strcut(text, NULL, " ", strTag);
	
	sprintf(cmd, "* BYE IMAP4rev1 Server Logout\r\n");
	ImapSend(cmd, strlen(cmd));
	sprintf(cmd, "%s BYE LOGOUT completed\r\n", strTag.c_str());
	ImapSend(cmd, strlen(cmd));
}

BOOL CMailImap::On_Authenticate(char* text)
{
	MailStorage* mailStg;
	StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
	if(!mailStg)
	{
		printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
		return FALSE;
	}
	char cmd[1024];
	string strTag;
	string strAuthType;
	
	string strText = text;
	vector<string> vecDest;
	ParseCommand(text, vecDest);
	strTag = vecDest[0];
	strAuthType = vecDest[2];
	
	if(strcasecmp(strAuthType.c_str(),"PLAIN") == 0)
	{
		return FALSE;
	}
	else if(strcasecmp(strAuthType.c_str(),"DIGEST-MD5") == 0)//(strAuthType == "DIGEST-MD5")
	{
		m_authType = atDIGEST_MD5;
		
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
		int outlen  = BASE64_ENCODE_OUTPUT_MAX_LEN(strlen(cmd));
		char* szEncoded = (char*)malloc(outlen);
		memset(szEncoded, 0, outlen);
		
		CBase64::Encode(cmd, strlen(cmd), szEncoded, &outlen);
		sprintf(cmd, "+ ");
		ImapSend(cmd,strlen(cmd));
		ImapSend(szEncoded, outlen);
		ImapSend((char*)"\r\n", 2);
		
		char szText[4096];
		memset(szText, 0, 4096);
		if(ProtRecv(szText, 4095) <= 0)
			return FALSE;
		
		////////////////////////////////
		string strEncoded;
		string strToken;
		strcut(szText, NULL, "\r\n",strEncoded);
		strtrim(strEncoded);
		outlen = BASE64_DECODE_OUTPUT_MAX_LEN(strEncoded.length());
		char* tmp = (char*)malloc(outlen);
		memset(tmp, 0, outlen);
		CBase64::Decode((char*)strEncoded.c_str(), strEncoded.length(), tmp, &outlen);
		
		string strRealm, strNonce, strCNonce, strDigestUri, strQop, strNc;
		
		string strRight;
		strcut(tmp, "username=\"", "\"", m_username);
		strtrim(m_username);
		strcut(tmp, "realm=\"", "\"", strRealm);
		
		strcut(tmp, "response=", "\n", strRight);
		strcut(strRight.c_str(), NULL, ",", strToken);
		strtrim(strToken);
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
		mailStg->GetPassword(m_username.c_str(), strpwd);
		
		unsigned char digest1[16];
		string strMD5src1;
		strMD5src1 = m_username + ":" + strRealm + ":" + strpwd;
		ietf::MD5_CTX_OBJ md5;
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
		string strTokenVerify = szMD5Dst4_1;
		
		if(strTokenVerify == strToken)
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
			outlen = BASE64_ENCODE_OUTPUT_MAX_LEN(strlen(cmd)); //strlen(cmd)*4+1;
			tmp = (char*)malloc(outlen);
			memset(tmp, 0, outlen);
			CBase64::Encode((char*)cmd, strlen(cmd), tmp, &outlen);
			
			sprintf(cmd,"+ ");
			ImapSend( cmd, strlen(cmd));
			ImapSend( tmp, outlen);
			ImapSend( (char*)"\r\n", 2);
			free(tmp);
		}
		
		///////////////////////////////////
		memset(szText, 0, 4096);
		if(ProtRecv(szText, 4095) <= 0)
			return FALSE;
		
		if(strToken == strTokenVerify)
		{
			sprintf(cmd,"%s OK User Logged in\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
			m_status = m_status|STATUS_AUTHED;
			return TRUE;
		}
		else
		{
			push_reject_list(MDA_SERVICE_NAME, m_clientip.c_str());
			
			sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
			return FALSE;
		}		
	}
	else if(strcasecmp(strAuthType.c_str(),"CRAM-MD5") == 0)
	{
		m_authType = atCRAM_MD5;
		
		char cmd[512];
		
		sprintf(cmd,"<%u%u.%u@%s>", (unsigned int)(random()%10000), (unsigned int)getpid(), (unsigned int)time(NULL), m_localhostname.c_str());
		string strDigest = cmd;
		int outlen  = BASE64_ENCODE_OUTPUT_MAX_LEN(strlen(cmd)); //strlen(cmd) *4 + 1;
		char* szEncoded = (char*)malloc(outlen);
		memset(szEncoded, 0, outlen);
		CBase64::Encode(cmd, strlen(cmd), szEncoded, &outlen);
		
		sprintf(cmd, "+ %s\r\n", szEncoded);
		free(szEncoded);
		//printf("%s\n", cmd);
		ImapSend(cmd,strlen(cmd));
		
		char szText[4096];
		memset(szText, 0, 4096);
		if(ProtRecv(szText, 4095) <= 0)
			return FALSE;
		
		string strEncoded;
		strcut(szText, NULL, "\r\n",strEncoded);
        strtrim(strEncoded);
		string strToken;
		outlen = BASE64_DECODE_OUTPUT_MAX_LEN(strEncoded.length());//strEncoded.length()*4+1;
		char* tmp = (char*)malloc(outlen);
		memset(tmp, 0, outlen);
		CBase64::Decode((char*)strEncoded.c_str(), strEncoded.length(), tmp, &outlen);
		strcut(tmp, NULL, " ", m_username);
		strcut(tmp, " ", NULL, strToken);
		free(tmp);
		strtrim(m_username);
        strtrim(strToken);
		string strpwd;
		if(mailStg->GetPassword(m_username.c_str(), strpwd) == 0)
		{
			unsigned char digest[16];
			ietf::HMAC_MD5((unsigned char*)strDigest.c_str(), strDigest.length(), (unsigned char*)strpwd.c_str(), strpwd.length(), digest);
			char szMD5dst[33];
			memset(szMD5dst, 0, 33);
			sprintf(szMD5dst,
				"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7],
				digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15]);
			
			if(strcasecmp(szMD5dst, strToken.c_str()) == 0)
			{
				sprintf(cmd,"%s OK User Logged in\r\n", strTag.c_str());
				ImapSend(cmd, strlen(cmd));
				m_status |= STATUS_AUTHED;
				return TRUE;
			}
			else
			{
				push_reject_list(MDA_SERVICE_NAME, m_clientip.c_str());
				sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
				ImapSend(cmd, strlen(cmd));
				return FALSE;
			}
		}
		else
		{
			push_reject_list(MDA_SERVICE_NAME, m_clientip.c_str());
			sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
			return FALSE;
		}
		
	}
    else if(strcasecmp(strAuthType.c_str(),"EXTERNAL") == 0)
	{   
        if(m_ssl)
        {
            m_authType = atEXTERNAL;
            
            string strEncodedUsername = vecDest[3];
            
            if(strEncodedUsername == "")
            {
                sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
                ImapSend(cmd, strlen(cmd));
                return FALSE;
            }
                
            string strMailAddr = "";
            string strAuthName = "";
            if(strEncodedUsername != "=")
            {
                int len_encode = BASE64_DECODE_OUTPUT_MAX_LEN(strEncodedUsername.length());
                char* tmp_decode = (char*)malloc(len_encode + 1);
                memset(tmp_decode, 0, len_encode + 1);
                
                int outlen_decode = len_encode;
                CBase64::Decode((char*)strEncodedUsername.c_str(), strEncodedUsername.length(), tmp_decode, &outlen_decode);
                
                strAuthName = tmp_decode;
                
                free(tmp_decode);
                
                strMailAddr = strAuthName;
                strMailAddr += "@";
                strMailAddr += CMailBase::m_email_domain;
            }

            X509* client_cert;
            client_cert = SSL_get_peer_certificate(m_ssl);
            if (client_cert != NULL)
            {
                X509_NAME * owner = X509_get_subject_name(client_cert);
                const char * owner_buf = X509_NAME_oneline(owner, 0, 0);
                
                if(strMailAddr == "" && X509_NAME_entry_count(owner) != 1)
                    {
                        X509_free (client_cert);
                        sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
                        ImapSend(cmd, strlen(cmd));
                        return FALSE;
                    }
                    
                const char* commonName;
                
                BOOL bFound = FALSE;
                int lastpos = -1;
                X509_NAME_ENTRY *e;
                for (;;)
                {
                    lastpos = X509_NAME_get_index_by_NID(owner, NID_commonName, lastpos);
                    if (lastpos == -1)
                        break;
                    e = X509_NAME_get_entry(owner, lastpos);
                    if(!e)
                        break;
                    ASN1_STRING *d = X509_NAME_ENTRY_get_data(e);
                    char *commonName = (char*)ASN1_STRING_data(d);
                    
                    if(strMailAddr == "")
                    {
                        if(strstr(commonName, "@") != NULL)
                            strcut(commonName, NULL, "@", strAuthName);
                        else
                            strAuthName = commonName;
                        bFound = TRUE;
                        break;
                    }
                    else
                    {
                        if(strcasecmp(commonName, strMailAddr.c_str()) == 0 || strmatch(commonName, strMailAddr.c_str()))
                        {
                            bFound = TRUE;
                            break;
                        }
                    }
                }
    
                
                X509_free (client_cert);
                
                if(bFound)
                {
                    m_username = strAuthName;
                    
                    sprintf(cmd,"%s OK EXTERNAL authentication successful\r\n", strTag.c_str());
                    ImapSend(cmd, strlen(cmd));
                    m_status = m_status|STATUS_AUTHED;
                    return TRUE;
                }
                else
                {
                    sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
                    ImapSend(cmd, strlen(cmd));
                    return FALSE;
             
                }
            }
            else
            {
                sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
                ImapSend(cmd, strlen(cmd));
                return FALSE;
            }
        }
        else
        {
            sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
            ImapSend(cmd, strlen(cmd));
            return FALSE;
        }
    }
#ifdef _WITH_GSSAPI_    
	else if(strcasecmp(strAuthType.c_str(),"GSSAPI") == 0)
	{        
		m_authType = atGSSAPI;
		
        ImapSend("+ \r\n", sizeof("+ \r\n") - 1);
        
        OM_uint32 maj_stat, min_stat;
        
        gss_cred_id_t server_creds = GSS_C_NO_CREDENTIAL;
        
        gss_name_t server_name = GSS_C_NO_NAME;
        
        gss_buffer_desc buf_desc;
        string str_buf_desc = "imap@";
        str_buf_desc += m_localhostname.c_str();
        
        buf_desc.value = (char *) str_buf_desc.c_str();
        buf_desc.length = str_buf_desc.length() + 1;
        
        maj_stat = gss_import_name (&min_stat, &buf_desc,
			      GSS_C_NT_HOSTBASED_SERVICE, &server_name);
        if (GSS_ERROR (maj_stat))
        {
            display_status ("gss_import_name", maj_stat, min_stat);
            sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
            return FALSE;
        }
        
        gss_OID_set oid_set = GSS_C_NO_OID_SET;
        /*
        maj_stat = gss_create_empty_oid_set(&min_stat, &oid_set);
        if (GSS_ERROR (maj_stat))
        {
            display_status ("gss_create_empty_oid_set", maj_stat, min_stat);
            sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
            return FALSE;
        }
        
        maj_stat = gss_add_oid_set_member (&min_stat, GSS_KRB5, &oid_set);
        if (GSS_ERROR (maj_stat))
        {
            display_status ("gss_add_oid_set_member", maj_stat, min_stat);
            maj_stat = gss_release_oid_set(&min_stat, &oid_set);
            sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
            return FALSE;
        }
        */
        maj_stat = gss_acquire_cred (&min_stat, server_name, 0,
			       oid_set, GSS_C_ACCEPT,
			       &server_creds, NULL, NULL);
        if (GSS_ERROR (maj_stat))
        {
            display_status ("gss_acquire_cred", maj_stat, min_stat);
            maj_stat = gss_release_oid_set(&min_stat, &oid_set);
            sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
            return FALSE;
        }
        maj_stat = gss_release_oid_set(&min_stat, &oid_set);
        
        if (GSS_ERROR (maj_stat))
        {
            display_status ("gss_release_oid_set", maj_stat, min_stat);
            sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
            return FALSE;
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
                    return FALSE;
                recv_buf[result] = '\0';
                str_line += recv_buf;
            }
            strtrim(str_line);
            
            int len_encode = BASE64_DECODE_OUTPUT_MAX_LEN(str_line.length());
            char* tmp_decode = (char*)malloc(len_encode);
            memset(tmp_decode, 0, len_encode);
            
            int outlen_decode = len_encode;
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
                display_status ("gss_accept_sec_context", maj_stat, min_stat);
                
                sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
                ImapSend(cmd, strlen(cmd));
                return FALSE;
            }
            
            if(!gss_oid_equal(mech_type, GSS_KRB5))
            {
                printf("not GSS_KRB5\n");
                if (context_hdl != GSS_C_NO_CONTEXT)
                    gss_delete_sec_context(&min_stat, &context_hdl, GSS_C_NO_BUFFER);
                sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
                ImapSend(cmd, strlen(cmd));
                return FALSE;
            }
                  
            if (output_token.length != 0)
            {
                int len_decode = BASE64_ENCODE_OUTPUT_MAX_LEN(output_token.length + 2);
                char* tmp_encode = (char*)malloc(len_decode + 2);
                memset(tmp_encode, 0, len_decode);
                tmp_encode[0] = '+';
                tmp_encode[1] = ' ';
                int outlen_encode = len_decode;
                CBase64::Encode((char*)output_token.value, output_token.length, tmp_encode + 2, &outlen_encode);
        
                ImapSend(tmp_encode, outlen_encode + 2);
                ImapSend("\r\n", 2);
                gss_release_buffer(&min_stat, &output_token);
                free(tmp_encode);
            }
            
            if(maj_stat != GSS_S_COMPLETE)
            {
                if (context_hdl != GSS_C_NO_CONTEXT)
                    gss_delete_sec_context(&min_stat, &context_hdl, GSS_C_NO_BUFFER);
            }
        } while (maj_stat & GSS_S_CONTINUE_NEEDED);
        
        string str_line = "";
        char recv_buf[4096];
        while(str_line.find("\n") == std::string::npos)
        {
            int result = ProtRecv(recv_buf, 4095);
            if(result <= 0)
                return FALSE;
            recv_buf[result] = '\0';
            str_line += recv_buf;
        }
        
        char sec_data[4];
        sec_data[0] = GSS_SEC_LAYER_NONE; //No security layer
        sec_data[1] = 0;
        sec_data[2] = 0;
        sec_data[3] = 0;
        gss_buffer_desc input_message_buffer1 = GSS_C_EMPTY_BUFFER, output_message_buffer1 = GSS_C_EMPTY_BUFFER;
        input_message_buffer1.length = 4;
        input_message_buffer1.value = sec_data;
        int conf_state;
        maj_stat = gss_wrap (&min_stat, context_hdl, 0, 0, &input_message_buffer1, &conf_state, &output_message_buffer1);
        if (GSS_ERROR(maj_stat))
        {
            if (context_hdl != GSS_C_NO_CONTEXT)
                gss_delete_sec_context(&min_stat, &context_hdl, GSS_C_NO_BUFFER);
            sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
            ImapSend(cmd, strlen(cmd));
            return FALSE;
        }
        
        int len_decode = BASE64_ENCODE_OUTPUT_MAX_LEN(output_message_buffer1.length + 2);
        char* tmp_encode = (char*)malloc(len_decode + 2);
        memset(tmp_encode, 0, len_decode);
        tmp_encode[0] = '+';
        tmp_encode[1] = ' ';
        int outlen_encode = len_decode;
        CBase64::Encode((char*)output_message_buffer1.value, output_message_buffer1.length, tmp_encode + 2, &outlen_encode);

        ImapSend(tmp_encode, outlen_encode + 2);
        ImapSend("\r\n", 2);
        gss_release_buffer(&min_stat, &output_message_buffer1);
        free(tmp_encode);
        
        str_line = "";
        while(str_line.find("\n") == std::string::npos)
        {
            int result = ProtRecv(recv_buf, 4095);
            if(result <= 0)
                return FALSE;
            recv_buf[result] = '\0';
            str_line += recv_buf;
        }
        
        int len_encode = BASE64_DECODE_OUTPUT_MAX_LEN(str_line.length());
        char* tmp_decode = (char*)malloc(len_encode);
        memset(tmp_decode, 0, len_encode);

        gss_buffer_desc input_message_buffer2 = GSS_C_EMPTY_BUFFER, output_message_buffer2 = GSS_C_EMPTY_BUFFER;
        gss_qop_t qop_state;
        int outlen_decode = len_encode;
        CBase64::Decode((char*)str_line.c_str(), str_line.length(), tmp_decode, &outlen_decode);
        input_message_buffer2.length = outlen_decode;
        input_message_buffer2.value = tmp_decode;
        
        maj_stat = gss_unwrap (&min_stat, context_hdl, &input_message_buffer2, &output_message_buffer2, &conf_state, &qop_state);
        if (GSS_ERROR(maj_stat))
        {
            if (context_hdl != GSS_C_NO_CONTEXT)
                gss_delete_sec_context(&min_stat, &context_hdl, GSS_C_NO_BUFFER);
            
            sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
            ImapSend(cmd, strlen(cmd));
            return FALSE;
        }
        memcpy(sec_data, output_message_buffer2.value, 4);
        OM_uint32 max_limit_size = sec_data[1] << 16;
        max_limit_size += sec_data[2] << 8;
        max_limit_size += sec_data[3];
        
        char* ptr_tmp = (char*)output_message_buffer2.value;
        for(int x = 4; x < output_message_buffer2.length; x++)
        {
            m_username.push_back(ptr_tmp[x]);
        }
        m_username.push_back('\0');
        free(tmp_decode);
        gss_release_buffer(&min_stat, &output_message_buffer2);
        
        gss_buffer_desc client_name_buff = GSS_C_EMPTY_BUFFER;
        maj_stat = gss_export_name (&min_stat, client_name, &client_name_buff);
        
        if(GSS_C_NO_NAME != client_name)
            gss_release_name(&min_stat, &client_name);
        
        gss_release_buffer(&min_stat, &client_name_buff);
        
        if (context_hdl != GSS_C_NO_CONTEXT)
            gss_delete_sec_context(&min_stat, &context_hdl, GSS_C_NO_BUFFER);
        
        MailStorage* mailStg;
        StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
        if(!mailStg)
        {
            printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
            return FALSE;
        }
        if(mailStg->VerifyUser(m_username.c_str()) != 0)
        {
            sprintf(cmd,"%s NO User Logged Failed\r\n", strTag.c_str());
            ImapSend(cmd, strlen(cmd));
            return FALSE;
        }
        sprintf(cmd,"%s OK GSSAPI authentication successful\r\n", strTag.c_str());
        ImapSend(cmd, strlen(cmd));
        m_status = m_status|STATUS_AUTHED;
        return TRUE;
	}
#endif /* _WITH_GSSAPI_ */    
	else
	{
		push_reject_list(MDA_SERVICE_NAME, m_clientip.c_str());
		sprintf(cmd, "%s NO authenticate Failed: unsupported authentication mechanism, credentials rejects\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
		return FALSE;
	}
	return TRUE;
}

BOOL CMailImap::On_Login(char* text)
{
	MailStorage* mailStg;
	StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
	if(!mailStg)
	{
		printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
		return FALSE;
	}
	BOOL retValue = TRUE;
	char cmd[1024];
	string strTag;
	string password;
	
	string strText = text;
	vector<string> vecDest;
	ParseCommand(text, vecDest);
	
	strTag = vecDest[0];
	m_username = vecDest[2];
	password = vecDest[3];
	
	if(mailStg->CheckLogin(m_username.c_str(), password.c_str()) == 0)
	{
		sprintf(cmd, "%s OK LOGIN Completed\r\n", strTag.c_str());
		m_status |= STATUS_AUTHED;
	}
	else
	{
		push_reject_list(MDA_SERVICE_NAME, m_clientip.c_str());
		
		sprintf(cmd, "%s NO LOGIN Failed\r\n", strTag.c_str());
		retValue = false;
	}
	ImapSend(cmd, strlen(cmd));

	return retValue;
}

//Authenticated state
void CMailImap::On_Select(char* text)
{	
	char cmd[512];
	string strTag;
	vector <string> vecDest;
	ParseCommand(text, vecDest);
	strTag = vecDest[0];
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		m_strSelectedDir = vecDest[2];
		User_Privilege eUserPri = upReadWrite;
		
		mailStg->ListMailByDir((char*)m_username.c_str(), m_maillisttbl, (char*)m_strSelectedDir.c_str());
		
		unsigned int nExist = 0;
		unsigned int nRecent = 0;
		unsigned int nUnseen = 0;
		unsigned int firstUnseen = 0;
		nExist = m_maillisttbl.size();
		for(int i = 0; i < m_maillisttbl.size(); i++)
		{
			if( (time(NULL) - m_maillisttbl[i].mtime)  > RECENT_MSG_TIME )
			{
				nRecent++;
			}
			if( (m_maillisttbl[i].mstatus & MSG_ATTR_SEEN) != MSG_ATTR_SEEN )
			{
				if(firstUnseen == 0)
					firstUnseen = i;
				nUnseen++;
			}
		}
		
		sprintf(cmd, 
			"* %u EXISTS\r\n"
			"* %u RECENT\r\n"
			"* OK [UNSEEN %u] Message %u is first unseen\r\n"
			"* OK [UIDVALIDITY %d] UIDs valid\r\n"
			"* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)\r\n"
			"* OK [PERMANENTFLAGS (\\Deleted \\Seen \\*)] Limited\r\n"
			"%s OK [%s] SELECT completed\r\n",
			nExist, nRecent, nUnseen, firstUnseen, 1, strTag.c_str(), USER_PRI_TBL[eUserPri]);
		
		m_status |= STATUS_SELECT;
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
	}
	ImapSend(cmd, strlen(cmd));
}

void CMailImap::On_Examine(char* text)
{
	char cmd[512];
	string strTag;
	vector <string> vecDest;
	ParseCommand(text, vecDest);
	strTag = vecDest[0];
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		string strMailBoxName;
		strMailBoxName = vecDest[2];
		
		mailStg->ListMailByDir((char*)m_username.c_str(), m_maillisttbl, (char*)m_strSelectedDir.c_str());
		
		unsigned int nExist = 0;
		unsigned int nRecent = 0;
		unsigned int nUnseen = 0;
		unsigned int firstUnseen = 0;
		nExist = m_maillisttbl.size();
		for(int i = 0; i < m_maillisttbl.size(); i++)
		{
			if( (time(NULL) - m_maillisttbl[i].mtime)  > RECENT_MSG_TIME )
			{
				nRecent++;
			}
			if( (m_maillisttbl[i].mstatus & MSG_ATTR_SEEN) != MSG_ATTR_SEEN )
			{
				if(firstUnseen == 0)
					firstUnseen = i;
				nUnseen++;
			}
		}
		
		sprintf(cmd, 
			"* %u EXISTS\r\n"
			"* %u RECENT\r\n"
			"* OK [UNSEEN %u] Message %u is first unseen\r\n"
			"* OK [UIDVALIDITY %d] UIDs valid\r\n"
			"* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)\r\n"
			"* OK [PERMANENTFLAGS (\\Deleted \\Seen \\*)] Limited\r\n"
			"%s OK [%s] EXAMINE completed\r\n",
			nExist, nRecent, nUnseen, 1, strTag.c_str(), USER_PRI_TBL[0]);
		ImapSend(cmd, strlen(cmd));
		m_status |= STATUS_SELECT;
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
}

void CMailImap::On_Create(char* text)
{	
	char cmd[1024];
	string strTag;
	vector <string> vecDest;
	ParseCommand(text, vecDest);
	strTag = vecDest[0];
	int dparentid = -1;
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		string dname = vecDest[2];
		
		if(mailStg->CreateDir(m_username.c_str(), dname.c_str() ) == 0)
		{
			sprintf(cmd, "%s OK CTEARE completed\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
		}
		else
		{
			sprintf(cmd, "%s No CTEARE Failed\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
		}
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
}

void CMailImap::On_Delete(char* text)
{	
	char cmd[1024];
	string strTag;
	vector <string> vecDest;
	ParseCommand(text, vecDest);
	strTag = vecDest[0];
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{		
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		if((strcasecmp(vecDest[2].c_str(), "inbox") == 0) || (strcasecmp(vecDest[2].c_str(), "drafts") == 0) || (strcasecmp(vecDest[2].c_str(), "trash") == 0) || (strcasecmp(vecDest[2].c_str(), "junk") == 0))
		{
			sprintf(cmd, "%s NO DELETE Failed: Forbid to Delete Reserved Directory\r\n", strTag.c_str());
		}
		else
		{
			if(mailStg->DeleteDir((char*)m_username.c_str(), vecDest[2].c_str()) == 0)
			{
				sprintf(cmd, "%s OK DELETE completed\r\n", strTag.c_str());
			}
			else
			{
				sprintf(cmd, "%s NO DELETE Failed\r\n", strTag.c_str());
			}
		}
		ImapSend(cmd, strlen(cmd));
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
}

void CMailImap::On_Rename(char* text)
{	
	char cmd[1024];
	string strTag;
	vector <string> vecDest;
	ParseCommand(text, vecDest);
	strTag = vecDest[0];
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		if((strcasecmp(vecDest[2].c_str(), "inbox") == 0) || (strcasecmp(vecDest[2].c_str(), "drafts") == 0) || (strcasecmp(vecDest[2].c_str(), "trash") == 0) || (strcasecmp(vecDest[2].c_str(), "junk") == 0))
		{
			sprintf(cmd, "%s NO RENAME Failed: Forbid to RENAME Reserved Directory\r\n", strTag.c_str());
		}
		else
		{
			if(mailStg->RenameDir(m_username.c_str(), vecDest[2].c_str(), vecDest[3].c_str()) == 0)
			{
				sprintf(cmd, "%s OK RENAME completed\r\n", strTag.c_str());
			}
			else
			{
				sprintf(cmd, "%s NO RENAME Failed\r\n", strTag.c_str());
			}
		}
		ImapSend(cmd, strlen(cmd));
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
}


void CMailImap::On_Subscribe(char* text)
{
	char cmd[1024];
	string strTag;
	string strText = text;
	vector<string> vecDest;
	ParseCommand(text, vecDest);
	strTag = vecDest[0];
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		string strName = vecDest[2];
		unsigned int status;
		mailStg->GetDirStatus(m_username.c_str(), strName.c_str(), status);
		status |= DIR_ATTR_SUBSCRIBED;
		mailStg->SetDirStatus(m_username.c_str(),strName.c_str(), status);
		sprintf(cmd, "%s OK SUBSCRIBE completed\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
}

void CMailImap::On_Unsubscribe(char* text)
{	

	char cmd[1024];
	string strTag;
	string strText = text;
	vector<string> vecDest;
	ParseCommand(text, vecDest);
	strTag = vecDest[0];
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);

		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		
		string strName = vecDest[2];
		unsigned int status;
		mailStg->GetDirStatus(m_username.c_str(),strName.c_str(), status);
		status = status&(~DIR_ATTR_SUBSCRIBED);
		mailStg->SetDirStatus(m_username.c_str(),strName.c_str(), status);
		
		sprintf(cmd, "%s OK SUBSCRIBE completed\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
}

void CMailImap::On_List(char* text)
{
	char cmd[1024];
	string strTag;
	string strText = text;
	vector<string> vecDest;
	ParseCommand(text, vecDest);
	strTag = vecDest[0];
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		string strReference, strName;
		strReference = vecDest[2];
		strName = vecDest[3];
		
		if((strReference == "") && (strName ==  "") )
		{
			sprintf(cmd, "* LIST (\\Noselect) \"/\" \"\"\r\n");
			ImapSend(cmd, strlen(cmd));
			
			sprintf(cmd, "%s OK LIST completed\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
		}
		else
		{	
			vector<Dir_Tree_Ex> ditbl;
			string strDirRef = strReference + "/" + strName;
			mailStg->TraversalListDir(m_username.c_str(), strDirRef.c_str(), ditbl);
			int vlen = ditbl.size();
			string strStatus;
			for(int i = 0; i < vlen; i++)
			{
				strStatus = "";
				if((ditbl[i].status & DIR_ATTR_MARKED) == DIR_ATTR_MARKED)
				{
					if(strStatus != "")
						strStatus += " ";
					strStatus = "\\Marked";
				}
				else
				{
					if(strStatus != "")
						strStatus += " ";
					strStatus = "\\Unmarked";
				}
				if((ditbl[i].status & DIR_ATTR_NOINFERIORS) == DIR_ATTR_NOINFERIORS)
				{
					if(strStatus != "")
						strStatus += " ";
					strStatus += "\\Noinferiors";
				}
				
				vector<string> vecDest1;
				vector<string> vecDest2;
				vSplitString(strReference, vecDest1, "/", TRUE, 0x7FFFFFFFU);
				vSplitString(strReference, vecDest2, "/", TRUE, 0x7FFFFFFFU);
				int vLen1, vLen2;
				vLen1 = vecDest1.size();
				vLen2 = vecDest1.size();
				if(vLen1 != vLen2)
				{
					if(strStatus != "")
						strStatus += " ";
					strStatus += "\\Noselect";
				}
				else
				{
					for(int j = 0; j < vLen1; j++)
					{
						if(strcasecmp(vecDest1[j].c_str(), vecDest2[j].c_str()) != 0)
						{
							if(strStatus != "")
								strStatus += " ";
							strStatus += "\\Noselect";
							break;
						}
					}
				}
				sprintf(cmd, "* LIST (%s) \"%s\" \"%s\"\r\n",strStatus.c_str(), strReference == "" ? "/" : strReference.c_str(), ditbl[i].path.c_str());
				ImapSend(cmd, strlen(cmd));
				
			} 
			sprintf(cmd, "%s OK LIST completed\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
		}		
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
}

void CMailImap::On_Listsub(char* text)
{	
	char cmd[1024];
	string strTag;
	string strText = text;
	vector<string> vecDest;
	ParseCommand(text, vecDest);
	strTag = vecDest[0];
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		string strReference, strName;
		strReference = vecDest[2];
		strName = vecDest[3];
		
		vector<Dir_Tree_Ex> ditbl;
		string strDirRef = strReference + "/" + strName;
		mailStg->TraversalListDir(m_username.c_str(), strDirRef.c_str(), ditbl);
		int vlen = ditbl.size();
		string strStatus;
		for(int i=0; i < vlen; i++)
		{
			if((ditbl[i].status & DIR_ATTR_SUBSCRIBED) == DIR_ATTR_SUBSCRIBED)
			{
				strStatus = "";
				if((ditbl[i].status & DIR_ATTR_MARKED) == DIR_ATTR_MARKED)
				{
					if(strStatus != "")
						strStatus += " ";
					strStatus = "\\Marked";
				}
				else
				{
					if(strStatus != "")
						strStatus += " ";
					strStatus = "\\Unmarked";
				}
				if((ditbl[i].status & DIR_ATTR_NOINFERIORS) == DIR_ATTR_NOINFERIORS)
				{
					if(strStatus != "")
						strStatus += " ";
					strStatus += "\\Noinferiors";
				}
				vector<string> vecDest1;
				vector<string> vecDest2;
				vSplitString(strReference, vecDest1, "/", TRUE, 0x7FFFFFFFU);
				vSplitString(strReference, vecDest2, "/", TRUE, 0x7FFFFFFFU);
				int vLen1, vLen2;
				vLen1 = vecDest1.size();
				vLen2 = vecDest1.size();
				if(vLen1 != vLen2)
				{
					if(strStatus != "")
						strStatus += " ";
					strStatus += "\\Noselect";
				}
				else
				{
					for(int j = 0; j < vLen1; j++)
					{
						if(strcasecmp(vecDest1[j].c_str(), vecDest2[j].c_str()) != 0)
						{
							if(strStatus != "")
								strStatus += " ";
							strStatus += "\\Noselect";
							break;
						}
					}
				}
				
				sprintf(cmd, "* LSUB (%s) \"%s\" \"%s\"\r\n",strStatus.c_str(), strReference == "" ? "/" : strReference.c_str(), ditbl[i].path.c_str());
				ImapSend(cmd, strlen(cmd));
			}
			
		} 
		sprintf(cmd, "%s OK LSUB completed\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
}

void CMailImap::On_Status(char* text)
{	
	char cmd[1024];
	string strTag;
	vector<string> vecDest;
	ParseCommand(text, vecDest);
	strTag = vecDest[0];
	string strResp ="";
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{	

		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}

		if(mailStg->ListMailByDir((char*)m_username.c_str(), m_maillisttbl, /* m_strSelectedDir.c_str()*/ vecDest[2].c_str()) != 0)
		{
			printf("List Mail By Dir Failed!\n");
			return;
		}
		
		unsigned int nExist = 0;
		unsigned int nRecent = 0;
		unsigned int nUnseen = 0;
		nExist = m_maillisttbl.size();
		for(int i = 0; i < m_maillisttbl.size(); i++)
		{
			if( (time(NULL) - m_maillisttbl[i].mtime)  > RECENT_MSG_TIME )
			{
				nRecent++;
			}
			if( (m_maillisttbl[i].mstatus & MSG_ATTR_SEEN) != MSG_ATTR_SEEN )
			{
				nUnseen++;
			}
		}

		for(int x = 3; x < vecDest.size(); x++)
		{			strtrim(vecDest[x], ")");
			strtrim(vecDest[x], "(");
			
			if(vecDest[x] == "MESSAGES")
			{
				sprintf(cmd, "MESSAGES %u", nExist);
				if(strResp == "")
				{
					strResp += cmd;
				}
				else
				{
					strResp += " ";
					strResp += cmd;
				}
			}
			else if(vecDest[x] == "RECENT")
			{
				sprintf(cmd, "RECENT %u", nRecent);
				if(strResp == "")
				{
					strResp += cmd;
				}
				else
				{
					strResp += " ";
					strResp += cmd;
				}
			}
			else if(vecDest[x] == "UIDNEXT")
			{
				if(m_maillisttbl.size() > 0)
				{
					sprintf(cmd, "UIDNEXT %u", m_maillisttbl[m_maillisttbl.size()-1].mid + 1);
					
				}
				else
				{
					sprintf(cmd, "UIDNEXT %u", 0);
				}
				if(strResp == "")
				{
					strResp += cmd;
				}
				else
				{
					strResp += " ";
					strResp += cmd;
				}
			}
			else if(vecDest[x] == "UIDVALIDITY")
			{
				int dirID;
				if(mailStg->GetDirID(m_username.c_str(), vecDest[2].c_str(), dirID) == -1)
				{
					sprintf(cmd, "%s NO STATUS Failed\r\n", strTag.c_str());
					ImapSend(cmd, strlen(cmd));
					return;
				}
				sprintf(cmd, "UIDNEXT %d", dirID);
				if(strResp == "")
				{
					strResp += cmd;
				}
				else
				{
					strResp += " ";
					strResp += cmd;
				}
			}
			else if(vecDest[x] == "UNSEEN")
			{
				sprintf(cmd, "RECENT %u", nUnseen);
				if(strResp == "")
				{
					strResp += cmd;
				}
				else
				{
					strResp += " ";
					strResp += cmd;
				}
			}
		}
		sprintf(cmd, "* %s %s (%s)\r\n", strTag.c_str(), vecDest[2].c_str(), strResp.c_str());
		ImapSend(cmd, strlen(cmd));
		
		sprintf(cmd, "%s OK STATUS Completed\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
}

void CMailImap::On_Append(char* text)
{	
	char cmd[1024];
	string strTag;
	string strText = text;
	vector<string> vecDest;
	ParseCommand(text, vecDest);
	strTag = vecDest[0];
	//printf("%s\r\n", text);
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		sprintf(cmd, "+ Ready for literal data\r\n");
		ImapSend(cmd, strlen(cmd));
		
		string dirName, strStatus, strDateTime, strLiteral;
		dirName = vecDest[2];
		if(vecDest.size() == 6)
		{
			strStatus = vecDest[3];
			strDateTime = vecDest[4];
			strLiteral = vecDest[5];
		}
		else if(vecDest.size() == 5)
		{
			if(strmatch("(*)", vecDest[3].c_str()))
				strStatus = vecDest[3];
			else
				strDateTime = vecDest[3];
			strLiteral = vecDest[4];
		}
		else if (vecDest.size() == 4)
		{
			strLiteral = vecDest[3];
		}
		else
		{
			sprintf(cmd, "%s NO APPEND Failed, parameter error.\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
			return;
		}
		unsigned int literal;
		sscanf(strLiteral.c_str(), "{%u}", &literal);

		//printf("literal: %d\r\n", literal);
		unsigned long long usermaxsize;
		if(mailStg->GetUserSize(m_username.c_str(), usermaxsize) == -1)
		{
			usermaxsize = 5000*1024;
		}

		//printf("usermaxsize: %d\r\n", usermaxsize);
		if(literal > usermaxsize)
		{
			sprintf(cmd, "%s NO APPEND Failed: The size of Message is too large.\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
			return;
		}
		unsigned int status = 0;
		
		if(strStatus.find("\\Seen", 0, sizeof("\\Seen") - 1) != string::npos)
		{
			status |= MSG_ATTR_SEEN;
		}
		if(strStatus.find("\\Answered", 0, sizeof("\\Answered") - 1) != string::npos)
		{
			status |= MSG_ATTR_ANSWERED;
		}
		if(strStatus.find("\\Flagged", 0, sizeof("\\Flagged") - 1) != string::npos)
		{
			status |= MSG_ATTR_FLAGGED;
		}
		if(strStatus.find("\\Deleted", 0, sizeof("\\Deleted") - 1) != string::npos)
		{
			status |= MSG_ATTR_DELETED;
		}
		if(strStatus.find("\\Draft", 0, sizeof("\\Draft") - 1) != string::npos)
		{
			status |= MSG_ATTR_DRAFT;
		}
		if(strStatus.find("\\Recent", 0, sizeof("\\Recent") - 1) != string::npos)
		{
			status |= MSG_ATTR_RECENT;
		}
		char newuid[256];
		sprintf(newuid, "%08x_%08x_%016lx_%08x_%s", time(NULL), getpid(), pthread_self(), random(), m_localhostname.c_str());

		//printf("%s\r\n", newuid);
		int DirID;
		if(mailStg->GetDirID(m_username.c_str(), dirName.c_str(), DirID) == -1)
		{
			//if no corresponding dir
			if(mailStg->CreateDir(m_username.c_str(), dirName.c_str()) == -1)
			{
				sprintf(cmd, "%s NO APPEND Failed, unable get the dir.\r\n", strTag.c_str());
				ImapSend(cmd, strlen(cmd));
				return;
			}
			else
			{
				if(mailStg->GetDirID(m_username.c_str(), dirName.c_str(), DirID) == -1)
				{
					sprintf(cmd, "%s NO APPEND Failed, unable get the dir.\r\n", strTag.c_str());
					ImapSend(cmd, strlen(cmd));
					return;
				}
			}
		}
		
		if(mailStg->GetUserSize(m_username.c_str(), usermaxsize) == -1)
		{
			usermaxsize = 5000*1024;
		}

		//printf("usermaxsize: %d\r\n", usermaxsize);
		
		MailLetter* Letter;
		Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, newuid, usermaxsize /*mailStg, 
		"", "", mtLocal, newuid, DirID, status,(unsigned int)time(NULL), usermaxsize*/);

		Letter_Info letter_info;
		letter_info.mail_from = "";
		letter_info.mail_to = "";
		letter_info.mail_type = mtLocal;
		letter_info.mail_uniqueid = newuid;
		letter_info.mail_dirid = DirID;
		letter_info.mail_status = status;
		letter_info.mail_time = time(NULL);
		letter_info.user_maxsize = usermaxsize;
		letter_info.mail_id = -1;
					
		char buf[1449];
		int nTotal = literal + 2;
		BOOL isOK = TRUE;
		while(1)
		{
			memset(buf, 0, 1449);
			int nRecv = ImapRecv(buf, nTotal > 1448 ? 1448 : nTotal);
			
			if(nRecv <= 0)
			{
				isOK = FALSE;
				break;
			}
			nTotal -= nRecv;

			if(Letter->Write(buf, nRecv)  < 0)
			{
				isOK = FALSE;
				break;
			}
			if(nTotal == 0)
				break;
		}
		
		if(isOK)
			Letter->SetOK();
		Letter->Close();
		if(Letter->isOK())
		{
			mailStg->InsertMailIndex(letter_info.mail_from.c_str(), letter_info.mail_to.c_str(),letter_info.mail_time,
						letter_info.mail_type, letter_info.mail_uniqueid.c_str(), letter_info.mail_dirid, letter_info.mail_status,
						Letter->GetEmlName(), Letter->GetSize(), letter_info.mail_id);
		}
		delete Letter;
		
		if(isOK)
			sprintf(cmd, "%s OK APPEND completed\r\n", strTag.c_str());
		else
			sprintf(cmd, "%s NO APPEND failed, too large.\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
}

//Select state
void CMailImap::On_Check(char* text)
{
	char cmd[1024];
	string strTag;
	strcut(text, NULL, " ", strTag);
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		if((m_status&STATUS_SELECT) == STATUS_SELECT)
		{
			sprintf(cmd, "%s OK Check Completed\r\n", strTag.c_str());
			ImapSend(cmd, strlen(cmd));
		}
		else
		{
			sprintf(cmd, "%s NO Need SELECT\r\n", strTag.c_str());
			ImapSend( cmd, strlen(cmd));
		}
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend(cmd, strlen(cmd));
	}
}
void CMailImap::On_Close(char* text)
{	
	char cmd[1024];
	string strTag;
	strcut(text, NULL, " ", strTag);
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		if((m_status&STATUS_SELECT) == STATUS_SELECT)
		{
			if(mailStg->ListMailByDir((char*)m_username.c_str(), m_maillisttbl, (char*)m_strSelectedDir.c_str()) == -1)
			{
				sprintf(cmd, "%s NO CLOSE Failed\r\n", strTag.c_str());
				ImapSend(cmd, strlen(cmd));
				return;
			}
			int vLen = m_maillisttbl.size();
			for(int x = 0; x < vLen; x++)
			{
				if((m_maillisttbl[x].mstatus & MSG_ATTR_DELETED) == MSG_ATTR_DELETED)
				{
					mailStg->DelMail(m_username.c_str(), m_maillisttbl[x].mid);
				}
			}
			sprintf(cmd, "%s OK CLOSE Completed\r\n", strTag.c_str());
			ImapSend( cmd, strlen(cmd));
		}
		else
		{
			sprintf(cmd, "%s NO Need SELECT\r\n", strTag.c_str());
			ImapSend( cmd, strlen(cmd));
		}
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend( cmd, strlen(cmd));
	}
}
void CMailImap::On_Expunge(char* text)
{	
	char cmd[1024];
	string strTag;
	strcut(text, NULL, " ", strTag);
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		MailStorage* mailStg;
		StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
		if(!mailStg)
		{
			printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
			return;
		}
		if((m_status&STATUS_SELECT) == STATUS_SELECT)
		{
			if(mailStg->ListMailByDir((char*)m_username.c_str(), m_maillisttbl, (char*)m_strSelectedDir.c_str()) == -1)
			{
				sprintf(cmd, "%s NO EXPUNGE Failed\r\n", strTag.c_str());
				ImapSend( cmd, strlen(cmd));
				return;
			}
			for(int x = 0; x < m_maillisttbl.size(); x++)
			{
				if((m_maillisttbl[x].mstatus & MSG_ATTR_DELETED) == MSG_ATTR_DELETED)
				{
					mailStg->DelMail(m_username.c_str(), m_maillisttbl[x].mid);
					sprintf(cmd, "* %d EXPUNGE\r\n", x);
				}
			}
			sprintf(cmd, "%s OK EXPUNGE Completed\r\n", strTag.c_str());
			ImapSend( cmd, strlen(cmd));
		}
		else
		{
			sprintf(cmd, "%s NO Need SELECT\r\n", strTag.c_str());
			ImapSend( cmd, strlen(cmd));
		}
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend( cmd, strlen(cmd));
	}
}
void CMailImap::On_Search(char* text)
{
	char cmd[1024];
	vector<string> vCmd;
	vSplitStringEx(text, vCmd, " ", FALSE, 2);
	string strTag = vCmd[0];
	string strArg = vCmd[2];
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		if((m_status&STATUS_SELECT) == STATUS_SELECT)
		{
			Search(strTag.c_str(), "SEARCH", strArg.c_str());
			sprintf(cmd, "%s OK SEARCH Completed\r\n", strTag.c_str());
			ImapSend( cmd, strlen(cmd));
		}
		else
		{
			sprintf(cmd, "%s NO Need SELECT\r\n", strTag.c_str());
			ImapSend( cmd, strlen(cmd));
		}
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend( cmd, strlen(cmd));
	}
}
void CMailImap::On_Fetch(char* text)
{
	char cmd[1024];
	vector<string> vCmd;
	vSplitStringEx(text, vCmd, " ", FALSE, 2);
	string strTag = vCmd[0];
	string strArg = vCmd[2];
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		if((m_status&STATUS_SELECT) == STATUS_SELECT)
		{
			Fetch(strArg.c_str());
			sprintf(cmd, "%s OK FETCH Completed\r\n", strTag.c_str());
			ImapSend( cmd, strlen(cmd));
		}
		else
		{
			sprintf(cmd, "%s NO Need SELECT\r\n", strTag.c_str());
			ImapSend( cmd, strlen(cmd));
		}
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend( cmd, strlen(cmd));
	}
}
void CMailImap::On_Store(char* text)
{
	char cmd[1024];
	vector<string> vCmd;
	vSplitStringEx(text, vCmd, " ", FALSE, 2);
	string strTag = vCmd[0];
	string strArg = vCmd[2];
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		if((m_status&STATUS_SELECT) == STATUS_SELECT)
		{
			Store(strArg.c_str());
			sprintf(cmd, "%s OK STORE Completed\r\n", strTag.c_str());
			ImapSend( cmd, strlen(cmd));
		}
		else
		{
			sprintf(cmd, "%s NO Need SELECT\r\n", strTag.c_str());
			ImapSend( cmd, strlen(cmd));
		}
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend( cmd, strlen(cmd));
	}
}
void CMailImap::On_Copy(char* text)
{
	char cmd[1024];
	vector<string> vCmd;
	vSplitStringEx(text, vCmd, " ", FALSE, 2);
	string strTag = vCmd[0];
	string strArg = vCmd[2];
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		if((m_status&STATUS_SELECT) == STATUS_SELECT)
		{
			if(Copy(strArg.c_str()) == 0)
			{
				sprintf(cmd, "%s OK COPY Completed\r\n", strTag.c_str());
			}
			else
			{
				sprintf(cmd, "%s NO COPY Failed\r\n", strTag.c_str());
			}
			ImapSend( cmd, strlen(cmd));
		}
		else
		{
			sprintf(cmd, "%s NO Need SELECT\r\n", strTag.c_str());
			ImapSend( cmd, strlen(cmd));
		}
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend( cmd, strlen(cmd));
	}
}

void CMailImap::On_UID(char* text)
{
	char cmd[1024];
	string strTag;
	string strText = text;
	vector<string> vecDest;
	
	vSplitString(strText, vecDest, " ", FALSE, 3);
	strTag = vecDest[0];
	
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		if((m_status&STATUS_SELECT) == STATUS_SELECT)
		{
			if(strcasecmp(vecDest[2].c_str(), "FETCH") == 0)
			{
				Fetch(vecDest[3].c_str(), TRUE);
				sprintf(cmd, "%s OK UID FETCH completed\r\n", strTag.c_str());
				ImapSend( cmd, strlen(cmd));
			}
			else if(strcasecmp(vecDest[2].c_str(), "SEARCH") == 0)
			{	
				Search(strTag.c_str(), "UID SEARCH", vecDest[3].c_str());
				sprintf(cmd, "%s OK UID SEARCH completed\r\n", strTag.c_str());
				ImapSend( cmd, strlen(cmd));
			}
			else if(strcasecmp(vecDest[2].c_str(), "STORE") == 0)
			{
				Store(vecDest[3].c_str(), TRUE);
				sprintf(cmd, "%s OK UID STORE completed\r\n", strTag.c_str());
				ImapSend( cmd, strlen(cmd));
			}
			else if(strcasecmp(vecDest[2].c_str(), "COPY") == 0)
			{
				if(Copy(vecDest[3].c_str(), TRUE) == 0)
					sprintf(cmd, "%s OK UID COPY completed\r\n", strTag.c_str());
				else
					sprintf(cmd, "%s NO UID COPY Failed\r\n", strTag.c_str());
				ImapSend( cmd, strlen(cmd));
			}
			else
			{
				sprintf(cmd, "%s BAD command unknown or arguments invailid\r\n", strTag.c_str());
				ImapSend( cmd, strlen(cmd));
			}
		}
		else
		{
			sprintf(cmd, "%s NO Need SELECT\r\n", strTag.c_str());
			ImapSend( cmd, strlen(cmd));
		}
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend( cmd, strlen(cmd));
	}
}

void CMailImap::On_STARTTLS(char* text)
{
	char cmd[1024];
	string strTag;
	string strText = text;
	vector<string> vecDest;
	
	vSplitString(strText, vecDest, " ", FALSE);
	strTag = vecDest[0];
	
	if(!m_enableimaptls)
	{
		sprintf(cmd, "%s BAD TLS is disabled\r\n", strTag.c_str());
		ImapSend(cmd,strlen(cmd));
		return;
	}
	if(m_bSTARTTLS == TRUE)
	{
		sprintf(cmd, "%s BAD Command not permitted when TLS active\r\n", strTag.c_str());
		ImapSend(cmd,strlen(cmd));
		return;
	}
	else
	{		
		sprintf(cmd, "%s OK Begin TLS negotiation now\r\n", strTag.c_str());
		ImapSend(cmd,strlen(cmd));
	}   
	
    if(!create_ssl(m_sockfd, m_ca_crt_root.c_str(), m_ca_crt_server.c_str(), m_ca_password.c_str(), m_ca_key_server.c_str(),
        m_ca_verify_client, &m_ssl, &m_ssl_ctx, CMailBase::m_connection_idle_timeout))
    {
        throw new string(ERR_error_string(ERR_get_error(), NULL));
    }
	
	m_lssl = new linessl(m_sockfd, m_ssl);
    
    m_bSTARTTLS = TRUE;
}

void CMailImap::On_X_atom(char* text)
{
	char cmd[1024];
	string strTag;
	string strXCmd;
	strcut(text, NULL, " ", strTag);
	strcut(text, " ", "\r\n", strXCmd);
	sprintf(cmd, "%s OK %s completed\r\n", strTag.c_str(), strXCmd.c_str());
	ImapSend( cmd, strlen(cmd));
}

void CMailImap::On_Namespace(char* text)
{
	char cmd[1024];
	string strTag;
	strcut(text, NULL, " ", strTag);
	if((m_status&STATUS_AUTHED) == STATUS_AUTHED)
	{
		sprintf(cmd, "* NAMESPACE NIL NIL ((\"\" \".\"))\r\n");
		ImapSend( cmd, strlen(cmd));
		
		sprintf(cmd, "%s OK NAMESPACE command completed\r\n", strTag.c_str());
		ImapSend( cmd, strlen(cmd));
	}
	else
	{
		sprintf(cmd, "%s NO Need LOGIN/AUTHENCIATE\r\n", strTag.c_str());
		ImapSend( cmd, strlen(cmd));
	}
}

void CMailImap::BodyStruct(MimeSummary* part, string & strStruct, BOOL isEND)
{
	
	filedContentType* fcType = part->GetContentType();
	
	if(fcType)
	{
		if(strcasecmp(fcType->m_type1.c_str(), "multipart") != 0)
		{
			strStruct += "(";
			string str1 = fcType->m_type1;
			string str2 = fcType->m_type2;
			string str3 = fcType->m_charset;
			string str4 = fcType->m_filename;
			string str5 = part->GetContentTransferEncoding();
			
			char str6[64];
			sprintf(str6, "%u %u", part->m_data->m_end - part->m_data->m_beg, part->m_data->m_linenumber);
			
			string strtmp;
			if(str1 ==  "")
				str1 = "NIL";
			else
			{
				strtmp = "\"";
				strtmp += str1 + "\"";
				str1 = strtmp;
			}
			if(str2 ==  "")
				str2 = "NIL";
			else
			{
				strtmp = "\"";
				strtmp += str2 + "\"";
				str2 = strtmp;
			}
			if(str3 == "")
			{
				str3 = "\"";
				str3 += CMailBase::m_encoding;
				str3 += "\"";
			}
			else
			{
				strtmp = "\"";
				strtmp += str3 + "\"";
				str3 = strtmp;
			}
			if(str5 == "")
				str5 = "\"7BIT\"";
			else
			{
				strtmp = "\"";
				strtmp += str5 + "\"";
				str5 = strtmp;
			}
			
			if(str4 == "")
			{
				strStruct += str1 + " " + str2 + " (\"CHARSET\" " + str3 + ") NIL NIL " + str5 + " " + str6 + " NIL NIL NIL";
			}
			else
			{
				strStruct += str1 + " " + str2 + " (\"CHARSET\" " + str3 + " \"NAME\" \"" + str4 + "\") NIL NIL " + str5 + " " + str6 + " NIL NIL NIL";
			}
			strStruct += ")";
		}
		else
		{
			strStruct += "(";
			
			for(int x = 0; x < part->GetSubPartNumber(); x++)
			{
				if(x == part->GetSubPartNumber() - 1)
					BodyStruct(part->GetPart(x), strStruct, TRUE);
				else
					BodyStruct(part->GetPart(x), strStruct);
			}
			
			strStruct += " \"";
			strStruct += fcType->m_type2.c_str();
			strStruct += "\" (\"BOUNDARY\" \"";
			strStruct += fcType->m_boundary.c_str();
			strStruct += "\") NIL NIL)";
		}
	}
	else
	{
		strStruct += "(";
		string str1 = "";
		string str2 = "";
		string str3 = "";
		string str4 = "";
		string str5 = "";
		
		char str6[64];
		sprintf(str6, "%u %u", part->m_data->m_end - part->m_beg, part->m_data->m_linenumber);
		
		string strtmp;
		if(str1 ==  "")
			str1 = "NIL";
		else
		{
			strtmp = "\"";
			strtmp += str1 + "\"";
			str1 = strtmp;
		}
		if(str2 ==  "")
			str2 = "NIL";
		else
		{
			strtmp = "\"";
			strtmp += str2 + "\"";
			str2 = strtmp;
		}
		if(str3 == "")
		{
			str3 = "\"";
			str3 += CMailBase::m_encoding;
			str3 += "\"";
		}
		else
		{
			strtmp = "\"";
			strtmp += str3 + "\"";
			str3 = strtmp;
		}
		if(str5 == "")
			str5 = "\"7BIT\"";
		else
		{
			strtmp = "\"";
			strtmp += str5 + "\"";
			str5 = strtmp;
		}
		
		if(str4 == "")
		{
			strStruct += str1 + " " + str2 + " (\"CHARSET\" " + str3 + ") NIL NIL " + str5 + " " + str6 + " NIL NIL NIL";
		}
		else
		{
			strStruct += str1 + " " + str2 + " (\"CHARSET\" " + str3 + " \"NAME\" \"" + str4 + "\") NIL NIL " + str5 + " " + str6 + " NIL NIL NIL";
		}
		strStruct += ")";
	}
	
}

void CMailImap::Fetch(const char* szArg, BOOL isUID)
{
	MailStorage* mailStg;
	StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
	if(!mailStg)
	{
		printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
		return;
	}
	char cmd[4096];
	memset(cmd, 0, 4096);
	string strSequence, strMsgItem;
	unsigned int nBegin, nEnd;
	strcut(szArg, NULL, " ", strSequence);
	strcut(szArg, " ", "\r\n", strMsgItem);
    
    strtrim(strSequence);
    strtrim(strMsgItem);
	vector<string> vecSequenceSet;
	vSplitString(strSequence, vecSequenceSet, ",");
	int zLen = vecSequenceSet.size();
	for(int z = 0; z < zLen; z++)
	{
		vector<string> vecSequenceRange;
		strSequence = vecSequenceSet[z];
		
		vSplitString(strSequence, vecSequenceRange, ":");
		if(vecSequenceRange.size() == 1)
		{
			nBegin = nEnd = atoi(vecSequenceRange[0].c_str());
		}
		else if(vecSequenceRange.size() == 2)
		{
			nBegin = atoi(vecSequenceRange[0].c_str());
			if(vecSequenceRange[1] == "*")
			{
				nEnd = 0x7FFFFFFFU;
			}
			else
			{
				nEnd = atoi(vecSequenceRange[1].c_str());
			}
		}
		
		int vLen = m_maillisttbl.size();
		
		if(strMsgItem == "ALL")
			strMsgItem = "(FLAGS INTERNALDATE RFC822.SIZE ENVELOPE)";
		else if(strMsgItem == "FAST")
			strMsgItem = "(FLAGS INTERNALDATE RFC822.SIZE)";
		else if(strMsgItem == "FULL")
			strMsgItem = "(FLAGS INTERNALDATE RFC822.SIZE ENVELOPE BODY)";
		
		strtrim(strMsgItem, "()");
		
		vector<string> vItem;
		ParseFetch(strMsgItem.c_str(), vItem);
		int iLen = vItem.size();
		for(int x = 0; x < vLen; x++)
		{
			if(((isUID ? m_maillisttbl[x].mid : x + 1) >= nBegin) && ((isUID ? m_maillisttbl[x].mid : x + 1) <= nEnd))
			{
				
				sprintf(cmd, "* %u FETCH (", x + 1);
				ImapSend( cmd, strlen(cmd));
				
				if(isUID)
				{
					sprintf(cmd, "UID %u", m_maillisttbl[x].mid);
					ImapSend(cmd, strlen(cmd));
				}
				
				MailLetter * Letter;

				string emlfile;
				mailStg->GetMailIndex(m_maillisttbl[x].mid, emlfile);
				
				Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
			
				if(Letter->GetSize()> 0)
				{
					int llen = 0;
					dynamic_mmap& lbuf = Letter->Body(llen);
					for(int i = 0 ;i < iLen; i++)
					{
						if(strmatch("BODY.PEEK[*]", vItem[i].c_str()) == TRUE)
						{
							if(strmatch("BODY.PEEK[HEADER.FIELDS (*)]", vItem[i].c_str()) == TRUE)
							{								
								string strHeaderFileds;
								vector<string> vecHeaderFileds;
								strcut(szArg, "BODY.PEEK[HEADER.FIELDS (", ")]", strHeaderFileds);
								vSplitString(strHeaderFileds, vecHeaderFileds, " ");
								int vHeaderLen = vecHeaderFileds.size();
								string strRespHeader = "";
								for(int j = 0; j < vHeaderLen; j++)
								{
									string fstrbuf = "";
									for(int x = Letter->GetSummary()->m_mime->m_header->m_beg; x < Letter->GetSummary()->m_mime->m_header->m_end; x++)
									{
										fstrbuf += lbuf[x];
										
										if(lbuf[x] == '\n' && lbuf[x + 1] != ' ' && lbuf[x] != '\t')
										{
											if(strncasecmp(fstrbuf.c_str(),vecHeaderFileds[j].c_str(), vecHeaderFileds[j].length()) == 0)
											{
												strRespHeader += fstrbuf;
											}					
											fstrbuf = "";
										}
									}
								}
								strRespHeader += "\r\n";

								if(i > 0 || isUID)
									ImapSend( " ", 1);
								
								sprintf(cmd, "BODY[HEADER.FIELDS (%s)] {%u}\r\n", strHeaderFileds.c_str(), strRespHeader.length());						
								ImapSend(cmd, strlen(cmd));
								ImapSend(strRespHeader.c_str(), strRespHeader.length());
							}
							else if(strmatch("BODY.PEEK[HEADER.FIELDS.NOT (*)]", vItem[i].c_str()) == TRUE)
							{								
								string strHeaderFileds;
								vector<string> vecHeaderFileds;
								strcut(szArg, "BODY.PEEK[HEADER.FIELDS.NOT (", ")]", strHeaderFileds);
								vSplitString(strHeaderFileds, vecHeaderFileds, " ");
								int vHeaderLen = vecHeaderFileds.size();
								string strRespHeader = "";
								
								for(int j = 0; j < vHeaderLen; j++)
								{
									string fstrbuf = "";
									for(int x = Letter->GetSummary()->m_mime->m_header->m_beg; x < Letter->GetSummary()->m_mime->m_header->m_end; x++)
									{
										fstrbuf += lbuf[x];
										
										if(lbuf[x] == '\n' && lbuf[x + 1] != ' ' && lbuf[x] != '\t')
										{
											if(strncasecmp(fstrbuf.c_str(),vecHeaderFileds[j].c_str(), vecHeaderFileds[j].length()) != 0)
											{
												strRespHeader += fstrbuf;
											}					
											fstrbuf = "";
										}
									}
								}
								strRespHeader += "\r\n";

								if(i > 0 || isUID)
									ImapSend( " ", 1);
								
								sprintf(cmd, "BODY[HEADER.FIELDS.NOT (%s)] {%u}\r\n", strHeaderFileds.c_str(), strRespHeader.length());
								
								ImapSend( cmd, strlen(cmd));
								ImapSend( strRespHeader.c_str(), strRespHeader.length());
							}
							else if(strcasecmp((char*)vItem[i].c_str(), (char*)"BODY.PEEK[]") == 0)
							{
								if(i > 0 || isUID)
									ImapSend( " ", 1);
																
								sprintf(cmd, "BODY[] {%u}\r\n", Letter->GetSummary()->m_mime->m_end - Letter->GetSummary()->m_mime->m_beg);
								ImapSend( cmd, strlen(cmd));
								
                                char read_buf[4096];
                                int read_count = 0;
                                while((read_count = Letter->Read(read_buf, 4096)) >= 0)
                                {
                                    if(read_count > 0)
                                    {
                                        if(ImapSend(read_buf, read_count) < 0)
                                            break;
                                    }
                                }
							}
							else if(strcasecmp((char*)vItem[i].c_str(), "BODY.PEEK[HEADER]") == 0)
							{
								if(i > 0 || isUID)
									ImapSend( " ", 1);
								
								sprintf(cmd, "BODY[HEADER] {%u}\r\n", Letter->GetSummary()->m_mime->m_header->m_end - Letter->GetSummary()->m_mime->m_header->m_beg);

								ImapSend( cmd, strlen(cmd));

								int nRead = 0;
								int tLen = Letter->GetSummary()->m_mime->m_header->m_end - Letter->GetSummary()->m_mime->m_header->m_beg;
								while(1)
								{
									if(nRead >= tLen)
										break;
									char sztmp[1025];
									dynamic_mmap_copy(sztmp, lbuf, Letter->GetSummary()->m_mime->m_header->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead) );
									sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0';

									nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);
									
									if(ImapSend(sztmp, strlen(sztmp)) < 0)
									{
										break;
									}
								}
								
							}
							else if(strcasecmp((char*)vItem[i].c_str(), "BODY.PEEK[TEXT]") == 0)
							{								
								BOOL isText = FALSE;
								if(i > 0 || isUID)
									ImapSend( " ", 1);
								sprintf(cmd, "BODY[TEXT] {%u}\r\n", Letter->GetSummary()->m_mime->m_data->m_end - Letter->GetSummary()->m_mime->m_data->m_beg);
								
								ImapSend( cmd, strlen(cmd));

								int nRead = 0;
								int tLen = Letter->GetSummary()->m_mime->m_data->m_end - Letter->GetSummary()->m_mime->m_data->m_beg;
								while(1)
								{
									if(nRead >= tLen)
										break;
									char sztmp[1025];
									dynamic_mmap_copy(sztmp, lbuf, Letter->GetSummary()->m_mime->m_data->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead) );
									sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0';

									nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);
									
									if(ImapSend(sztmp, strlen(sztmp)) < 0)
									{
											break;
									}
								}
							}
							else if(strmatch("BODY.PEEK[?*.MIME]", vItem[i].c_str()) == TRUE)
							{
								if(i > 0 || isUID)
									ImapSend( " ", 1);
								
								string rstrtmp;
								strcut(vItem[i].c_str(), "BODY.PEEK[", ".MIME]", rstrtmp);
								
								MimeSummary* pMp = Letter->GetSummary()->m_mime;
								vector<string> vstmp;
								vSplitString(rstrtmp, vstmp, ".");
								BOOL isExist = TRUE;
								for(int p = 0; p < vstmp.size(); p++)
								{
									if(atoi(vstmp[p].c_str()) > 0)
									{
										if(pMp->GetSubPartNumber() < atoi(vstmp[p].c_str()))
										{
											isExist = FALSE;
											break;
										}
										else
											pMp = pMp->GetPart(atoi(vstmp[p].c_str()) - 1);
									}
								}
								if(isExist)
								{
									
									
									sprintf(cmd, "BODY[%s.MIME] {%u}\r\n", rstrtmp.c_str(), pMp->m_header->m_end - pMp->m_header->m_beg);
									ImapSend( cmd, strlen(cmd));
									
									int nRead = 0;
									int tLen = pMp->m_header->m_end - pMp->m_header->m_beg;
									while(1)
									{
										if(nRead >= tLen)
											break;
										char sztmp[1025];
										dynamic_mmap_copy(sztmp, lbuf, pMp->m_header->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead) );
										sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0';

										nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);
										
										if(ImapSend(sztmp, strlen(sztmp)) < 0)
										{
											break;
										}
									}
								
								}
								else
								{
									sprintf(cmd, "BODY[%s.MIME] NIL\r\n", rstrtmp.c_str());
									ImapSend( cmd, strlen(cmd));
								}
							}
							else if(strmatch("BODY.PEEK[?*.TEXT]", vItem[i].c_str()) == TRUE)
							{
								if(i > 0 || isUID)
									ImapSend( " ", 1);
								
								string rstrtmp;
								strcut(vItem[i].c_str(), "BODY.PEEK[", ".MIME]", rstrtmp);
								MimeSummary* pMp = Letter->GetSummary()->m_mime;
								vector<string> vstmp;
								vSplitString(rstrtmp, vstmp, ".");
								BOOL isExist = TRUE;
								for(int p = 0; p < vstmp.size(); p++)
								{
									if(atoi(vstmp[p].c_str()) > 0)
									{
										if(pMp->GetSubPartNumber() < atoi(vstmp[p].c_str()))
										{
											isExist = FALSE;
											break;
										}
										else
											pMp = pMp->GetPart(atoi(vstmp[p].c_str()) - 1);
									}
								}
								if(isExist)
								{
									sprintf(cmd, "BODY[%s.TEXT] {%u}\r\n", rstrtmp.c_str(), pMp->m_data->m_end - pMp->m_data->m_beg);
									ImapSend( cmd, strlen(cmd));

									int nRead = 0;
									int tLen = pMp->m_data->m_end - pMp->m_data->m_beg;
									while(1)
									{
										if(nRead >= tLen)
											break;
										char sztmp[1025];
										dynamic_mmap_copy(sztmp, lbuf, pMp->m_data->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead));
										sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0';

										nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);
										if(ImapSend(sztmp, strlen(sztmp)) < 0)
										{
											break;
										}
									}
									
								}
								else
								{
									sprintf(cmd, "BODY[%s.TEXT] NIL\r\n", rstrtmp.c_str());
									ImapSend( cmd, strlen(cmd));
								}
							}
							else if(strmatch("BODY.PEEK[?*.HEADER]", vItem[i].c_str()) == TRUE)
							{
								if(i > 0 || isUID)
									ImapSend( " ", 1);
								string rstrtmp;
								strcut(vItem[i].c_str(), "BODY.PEEK[", ".MIME]", rstrtmp);
								MimeSummary* pMp = Letter->GetSummary()->m_mime;
								vector<string> vstmp;
								vSplitString(rstrtmp, vstmp, ".");
								BOOL isExist = TRUE;
								for(int p = 0; p < vstmp.size(); p++)
								{
									if(atoi(vstmp[p].c_str()) > 0)
									{
										if(pMp->GetSubPartNumber() < atoi(vstmp[p].c_str()))
										{
											isExist = FALSE;
											break;
										}
										else
											pMp = pMp->GetPart(atoi(vstmp[p].c_str()) - 1);
									}
								}
								if(isExist)
								{
								
									sprintf(cmd, "BODY[%s.HEADER] {%u}\r\n", rstrtmp.c_str(), pMp->m_header->m_end - pMp->m_header->m_beg);
									ImapSend( cmd, strlen(cmd));

									int nRead = 0;
									int tLen = pMp->m_header->m_end - pMp->m_header->m_beg;
									while(1)
									{
										if(nRead >= tLen)
											break;
										char sztmp[1025];
										dynamic_mmap_copy(sztmp, lbuf, pMp->m_header->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead) );
										sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0';

										nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);
										
										if(ImapSend(sztmp, strlen(sztmp)) < 0)
										{
											break;
										}
									}
									
								}
								else
								{
									sprintf(cmd, "BODY[%s.HEADER] NIL\r\n", rstrtmp.c_str());
									ImapSend( cmd, strlen(cmd));
								}
							}
							else
							{	
								if(i > 0 || isUID)
									ImapSend(" ", 1);
								
								string rstrtmp;
								strcut(vItem[i].c_str(), "BODY.PEEK[", "]", rstrtmp);
								MimeSummary* pMp = Letter->GetSummary()->m_mime;
								vector<string> vstmp;
								vSplitString(rstrtmp, vstmp, ".");
								BOOL isExist = TRUE;
								for(int p = 0; p < vstmp.size(); p++)
								{
									if(atoi(vstmp[p].c_str()) > 0)
									{
										if(pMp->GetSubPartNumber() < atoi(vstmp[p].c_str()))
										{
											isExist = FALSE;
											break;
										}
										else
											pMp = pMp->GetPart(atoi(vstmp[p].c_str()) - 1);
									}
								}
								if(isExist)
								{
									sprintf(cmd, "BODY[%s] {%u}\r\n", rstrtmp.c_str(), pMp->m_data->m_end - pMp->m_data->m_beg);
									ImapSend( cmd, strlen(cmd));
									int nRead = 0;
									int tLen = pMp->m_data->m_end - pMp->m_data->m_beg;
									while(1)
									{
										if(nRead >= tLen)
											break;
										char sztmp[1025];
										dynamic_mmap_copy(sztmp, lbuf, pMp->m_data->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead) );
										sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0'; 

										nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);
										
										if(ImapSend(sztmp, strlen(sztmp)) < 0)
										{
											break;
										}

									}
								}
								else
								{
									sprintf(cmd, "BODY[%s] NIL\r\n", rstrtmp.c_str());
									ImapSend( cmd, strlen(cmd));
								}
							}
						}
						else if(strmatch("BODY[*]", vItem[i].c_str()) == TRUE)
						{
							if(strmatch("BODY[HEADER.FIELDS (*)]", vItem[i].c_str()) == TRUE)
							{
								string strHeaderFileds;
								vector<string> vecHeaderFileds;
								strcut(szArg, "BODY[HEADER.FIELDS (", ")]", strHeaderFileds);
								vSplitString(strHeaderFileds, vecHeaderFileds, " ");
								int vHeaderLen = vecHeaderFileds.size();
								string strRespHeader = "";
								for(int j = 0; j < vHeaderLen; j++)
								{

									string fstrbuf = "";
									for(int x = Letter->GetSummary()->m_mime->m_header->m_beg; x < Letter->GetSummary()->m_mime->m_header->m_end; x++)
									{
										fstrbuf += lbuf[x];
										
										if(lbuf[x] == '\n' && lbuf[x + 1] != ' ' && lbuf[x] != '\t')
										{
											if(strncasecmp(fstrbuf.c_str(),vecHeaderFileds[j].c_str(), vecHeaderFileds[j].length()) == 0)
											{
												strRespHeader += fstrbuf;
											}					
											fstrbuf = "";
										}
									}
									
								}
								strRespHeader += "\r\n";

								if(i > 0 || isUID)
									ImapSend( " ", 1);
								
								sprintf(cmd, "BODY[HEADER.FIELDS (%s)] {%u}\r\n", strHeaderFileds.c_str(), strRespHeader.length());
								ImapSend(cmd, strlen(cmd));
								ImapSend(strRespHeader.c_str(), strRespHeader.length());
							}
							else if(strmatch("BODY[HEADER.FIELDS.NOT (*)]", vItem[i].c_str()) == TRUE)
							{
								string strHeaderFileds;
								vector<string> vecHeaderFileds;
								strcut(szArg, "BODY[HEADER.FIELDS.NOT (", ")]", strHeaderFileds);
								vSplitString(strHeaderFileds, vecHeaderFileds, " ");
								int vHeaderLen = vecHeaderFileds.size();
								string strRespHeader = "";
								
								for(int j = 0; j < vHeaderLen; j++)
								{
									string fstrbuf = "";
									for(int x = Letter->GetSummary()->m_mime->m_header->m_beg; x < Letter->GetSummary()->m_mime->m_header->m_end; x++)
									{
										fstrbuf += lbuf[x];
										
										if(lbuf[x] == '\n' && lbuf[x + 1] != ' ' && lbuf[x] != '\t')
										{
											if(strncasecmp(fstrbuf.c_str(),vecHeaderFileds[j].c_str(), vecHeaderFileds[j].length()) != 0)
											{
												strRespHeader += fstrbuf;
											}					
											fstrbuf = "";
										}
									}
								}
								strRespHeader += "\r\n";

								if(i > 0 || isUID)
									ImapSend( " ", 1);
								
								sprintf(cmd, "BODY[HEADER.FIELDS.NOT (%s)] {%u}\r\n", strHeaderFileds.c_str(), strRespHeader.length());
								
								ImapSend( cmd, strlen(cmd));
								ImapSend( strRespHeader.c_str(), strRespHeader.length());
							}
							else if(strcasecmp((char*)vItem[i].c_str(), (char*)"BODY[]") == 0)
							{	
								if(i > 0 || isUID)
									ImapSend( " ", 1);
								
								sprintf(cmd, "BODY[] {%u}\r\n", Letter->GetSummary()->m_mime->m_end - Letter->GetSummary()->m_mime->m_beg);
																
								ImapSend( cmd, strlen(cmd));
								
                                char read_buf[4096];
                                int read_count = 0;
                                while((read_count = Letter->Read(read_buf, 4096)) >= 0)
                                {
                                    if(read_count > 0)
                                    {
                                        if(ImapSend(read_buf, read_count) < 0)
                                            break;
                                    }
                                }
							}
							else if(strcasecmp((char*)vItem[i].c_str(), "BODY[HEADER]") == 0)
							{
								if(i > 0 || isUID)
									ImapSend( " ", 1);
								
								sprintf(cmd, "BODY[HEADER] {%u}\r\n", Letter->GetSummary()->m_mime->m_header->m_end -  Letter->GetSummary()->m_mime->m_header->m_beg);
								
								ImapSend( cmd, strlen(cmd));

								int nRead = 0;
								int tLen = Letter->GetSummary()->m_mime->m_header->m_end -  Letter->GetSummary()->m_mime->m_header->m_beg;
								while(1)
								{
									if(nRead >= tLen)
										break;
									char sztmp[1025];
									dynamic_mmap_copy(sztmp, lbuf, Letter->GetSummary()->m_mime->m_header->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead) );
									sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0';

									nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);
									
									if(ImapSend(sztmp, strlen(sztmp)) < 0)
									{
											break;
									}
								}
							}
							else if(strcasecmp((char*)vItem[i].c_str(), "BODY[TEXT]") == 0)
							{
								
								BOOL isText = FALSE;
								if(i > 0 || isUID)
									ImapSend( " ", 1);
								
								sprintf(cmd, "BODY[TEXT] {%u}\r\n", Letter->GetSummary()->m_mime->m_data->m_end -  Letter->GetSummary()->m_mime->m_data->m_beg);
								
								ImapSend( cmd, strlen(cmd));

								int nRead = 0;
								int tLen = Letter->GetSummary()->m_mime->m_data->m_end -  Letter->GetSummary()->m_mime->m_data->m_beg;
								while(1)
								{
									if(nRead >= tLen)
										break;
									char sztmp[1025];
									dynamic_mmap_copy(sztmp, lbuf, Letter->GetSummary()->m_mime->m_data->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead) );
									sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0';

									nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);

									if(ImapSend(sztmp, strlen(sztmp)) < 0)
									{
										break;
									}
								}
								
							}
							else if(strmatch("BODY[?*.MIME]", vItem[i].c_str()) == TRUE)
							{									
								if(i > 0 || isUID)
									ImapSend( " ", 1);							
								string rstrtmp;
								strcut(vItem[i].c_str(), "BODY[", ".MIME]", rstrtmp);
								MimeSummary* pMp = Letter->GetSummary()->m_mime;
								vector<string> vstmp;
								vSplitString(rstrtmp, vstmp, ".");
								BOOL isExist = TRUE;
								for(int p = 0; p < vstmp.size(); p++)
								{
									if(atoi(vstmp[p].c_str()) > 0)
									{
										if(pMp->GetSubPartNumber() < atoi(vstmp[p].c_str()))
										{
											isExist = FALSE;
											break;
										}
										else
											pMp = pMp->GetPart(atoi(vstmp[p].c_str()) - 1);
									}
								}
								if(isExist)
								{
									sprintf(cmd, "BODY[%s.MIME] {%u}\r\n", rstrtmp.c_str(), pMp->m_header->m_end - pMp->m_header->m_beg);
									ImapSend( cmd, strlen(cmd));

									int nRead = 0;
									int tLen = pMp->m_header->m_end - pMp->m_header->m_beg;
									while(1)
									{
										if(nRead >= tLen)
											break;
										char sztmp[1025];
										dynamic_mmap_copy(sztmp, lbuf, pMp->m_header->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead) );
										sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0';

										nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);

										if(ImapSend(sztmp, strlen(sztmp)) < 0)
										{
											break;
										}
									}
								}
								else
								{
									sprintf(cmd, "BODY[%s.MIME] NIL\r\n", rstrtmp.c_str());
									ImapSend( cmd, strlen(cmd));
								}
							}
							else if(strmatch("BODY[?*.TEXT]", vItem[i].c_str()) == TRUE)
							{
								if(i > 0 || isUID)
									ImapSend( " ", 1);

								string rstrtmp;
								strcut(vItem[i].c_str(), "BODY[", ".MIME]", rstrtmp);
								MimeSummary* pMp = Letter->GetSummary()->m_mime;
								vector<string> vstmp;
								vSplitString(rstrtmp, vstmp, ".");
								BOOL isExist = TRUE;
								for(int p = 0; p < vstmp.size(); p++)
								{
									if(atoi(vstmp[p].c_str()) > 0)
									{
										if(pMp->GetSubPartNumber() < atoi(vstmp[p].c_str()))
										{
											isExist = FALSE;
											break;
										}
										else
											pMp = pMp->GetPart(atoi(vstmp[p].c_str()) - 1);
									}
								}
								if(isExist)
								{
									sprintf(cmd, "BODY[%s.TEXT] {%u}\r\n", rstrtmp.c_str(), pMp->m_data->m_end - pMp->m_data->m_beg);
									ImapSend( cmd, strlen(cmd));

									int nRead = 0;
									int tLen = pMp->m_data->m_end - pMp->m_data->m_beg;
									while(1)
									{
										if(nRead >= tLen)
											break;
										char sztmp[1025];
										dynamic_mmap_copy(sztmp, lbuf, pMp->m_data->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead) );
										sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0';\

										nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);

										if(ImapSend(sztmp, strlen(sztmp)) < 0)
										{
											break;
										}
									}
									
								}
								else
								{
									sprintf(cmd, "BODY[%s.TEXT] NIL\r\n", rstrtmp.c_str());
									ImapSend( cmd, strlen(cmd));
								}
							}
							else if(strmatch("BODY[?*.HEADER]", vItem[i].c_str()) == TRUE)
							{
								if(i > 0 || isUID)
									ImapSend( " ", 1);
								
								string rstrtmp;
								strcut(vItem[i].c_str(), "BODY[", ".MIME]", rstrtmp);
								MimeSummary* pMp = Letter->GetSummary()->m_mime;
								vector<string> vstmp;
								vSplitString(rstrtmp, vstmp, ".");
								BOOL isExist = TRUE;
								for(int p = 0; p < vstmp.size(); p++)
								{
									if(atoi(vstmp[p].c_str()) > 0)
									{
										if(pMp->GetSubPartNumber() < atoi(vstmp[p].c_str()))
										{
											isExist = FALSE;
											break;
										}
										else
											pMp = pMp->GetPart(atoi(vstmp[p].c_str()) - 1);
									}
								}
								if(isExist)
								{
									sprintf(cmd, "BODY[%s.HEADER] {%u}\r\n", rstrtmp.c_str(), pMp->m_header->m_end - pMp->m_header->m_beg);
									ImapSend( cmd, strlen(cmd));

									int nRead = 0;
									int tLen = pMp->m_header->m_end - pMp->m_header->m_beg;
									while(1)
									{
										if(nRead >= tLen)
											break;
										char sztmp[1025];
										dynamic_mmap_copy(sztmp, lbuf, pMp->m_header->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead) );
										sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0';

										nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);

										if(ImapSend(sztmp, strlen(sztmp)) < 0)
										{
											break;
										}
									}
								}
								else
								{
									sprintf(cmd, "BODY[%s.HEADER] NIL\r\n", rstrtmp.c_str());
									ImapSend( cmd, strlen(cmd));
								}
							}
							else
							{	
								if(i > 0 || isUID)
									ImapSend( " ", 1);
								
								string rstrtmp;
								strcut(vItem[i].c_str(), "BODY[", "]", rstrtmp);
								MimeSummary* pMp = Letter->GetSummary()->m_mime;
								vector<string> vstmp;
								vSplitString(rstrtmp, vstmp, ".");
								BOOL isExist = TRUE;
								for(int p = 0; p < vstmp.size(); p++)
								{
									if(atoi(vstmp[p].c_str()) > 0)
									{
										if(pMp->GetSubPartNumber() < atoi(vstmp[p].c_str()))
										{
											isExist = FALSE;
											break;
										}
										else
											pMp = pMp->GetPart(atoi(vstmp[p].c_str()) - 1);
									}
								}
								if(isExist)
								{
									sprintf(cmd, "BODY[%s] {%u}\r\n", rstrtmp.c_str(), pMp->m_data->m_end -  pMp->m_data->m_beg);
									ImapSend( cmd, strlen(cmd));
									int nRead = 0;
									int tLen = pMp->m_data->m_end - pMp->m_data->m_beg;
									while(1)
									{
										if(nRead >= tLen)
											break;
										char sztmp[1025];
										dynamic_mmap_copy(sztmp, lbuf, pMp->m_data->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead) );
										sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0';

										nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);

										if(ImapSend(sztmp, strlen(sztmp)) < 0)
										{
											break;
										}
									}
								}
								else
								{
									sprintf(cmd, "BODY[%s] NIL\r\n", rstrtmp.c_str());
									ImapSend( cmd, strlen(cmd));
								}
							}
						}
						else if(strcasecmp(vItem[i].c_str(), "BODY") == 0)
						{	
							if(i > 0 || isUID)
								ImapSend( " ", 1);
														
							sprintf(cmd, "BODY[] {%u}\r\n", Letter->GetSummary()->m_mime->m_end - Letter->GetSummary()->m_mime->m_beg);

							ImapSend( cmd, strlen(cmd));
							
                            char read_buf[4096];
                            int read_count = 0;
                            while((read_count = Letter->Read(read_buf, 4096)) >= 0)
                            {
                                if(read_count > 0)
                                {
                                    if(ImapSend(read_buf, read_count) < 0)
                                        break;
                                }
                            }
						}
						else if(
							strmatch("BODY.PEEK[]<*.*>", vItem[i].c_str())||
							strmatch("BODY[]<*.*>", vItem[i].c_str())||
							strmatch("BODY<*.*>", vItem[i].c_str())
							)
						{
							if(i > 0 || isUID)
								ImapSend( " ", 1);
							
							string strRange;
							strcut(vItem[i].c_str(), "<", ">", strRange);
							strtrim(strRange);
							unsigned int origin, octet;
							sscanf(strRange.c_str(), "%u.%u", &origin, &octet);
                            
							unsigned int llen = Letter->GetSize();
                            
							llen = origin + octet > llen ? llen  - origin : octet;
							
                            sprintf(cmd, "BODY[]<%u.%u> {%u}\r\n", origin, llen, llen);
                            
							if(i > 0 || isUID)
								ImapSend( " ", 1);
							ImapSend(cmd, strlen(cmd));
							
                            //move the original position
                            Letter->Seek(origin);
                            
                            unsigned int wlen = 0;
                            char read_buf[4096];
                            int read_count = 0;
                            while((read_count = Letter->Read(read_buf, (llen - wlen) > 4096 ? 4096 : (llen - wlen))) >= 0)
                            {
                                if(read_count > 0)
                                {
                                    if(ImapSend(read_buf, read_count) < 0)
                                        break;
                                    
                                    wlen += read_count;
                                    if(wlen >= llen)
                                        break;
                                }
                            }
						}
						
						else if(strcasecmp(vItem[i].c_str(), "BODYSTRUCTURE") == 0)
						{
							string strStruct;
							
							BodyStruct(Letter->GetSummary()->m_mime, strStruct, TRUE);
							
							//printf("%s\n", strStruct.c_str());
							
							if(i > 0 || isUID)
								ImapSend( " ", 1);
							
							sprintf(cmd, "BODYSTRUCTURE ");
							ImapSend(cmd, strlen(cmd));
							
							ImapSend(strStruct.c_str(), strStruct.length());						
						}
						else if(strcasecmp(vItem[i].c_str(), "RFC822") == 0)
						{
							if(i > 0 || isUID)
								ImapSend( " ", 1);
														
							sprintf(cmd, "RFC822 {%u}\r\n", Letter->GetSummary()->m_mime->m_end - Letter->GetSummary()->m_mime->m_beg);

							ImapSend( cmd, strlen(cmd));

							int nRead = 0;
							int tLen = Letter->GetSummary()->m_mime->m_end - Letter->GetSummary()->m_mime->m_beg;
							while(1)
							{
								if(nRead >= tLen)
									break;
								char sztmp[1025];
								dynamic_mmap_copy(sztmp, lbuf, Letter->GetSummary()->m_mime->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead) );
								sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0';

								nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);

								if(ImapSend(sztmp, strlen(sztmp)) < 0)
								{
									break;
								}
							}
						}
						else if(strcasecmp(vItem[i].c_str(), "RFC822.HEADER") == 0)
						{
							if(i > 0 || isUID)
								ImapSend( " ", 1);
							
							sprintf(cmd, "BODY[HEADER] {%u}\r\n", Letter->GetSummary()->m_mime->m_header->m_end - Letter->GetSummary()->m_mime->m_header->m_beg);
							
							ImapSend( cmd, strlen(cmd));

							int nRead = 0;
							int tLen = Letter->GetSummary()->m_mime->m_header->m_end - Letter->GetSummary()->m_mime->m_header->m_beg;
							while(1)
							{
								if(nRead >= tLen)
									break;
								char sztmp[1025];
								dynamic_mmap_copy(sztmp, lbuf, Letter->GetSummary()->m_mime->m_header->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead) );
								sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0';

								nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);

								if(ImapSend(sztmp, strlen(sztmp)) < 0)
								{
									break;
								}
							}
							
						}
						else if(strcasecmp(vItem[i].c_str(), "RFC822.TEXT") == 0)
						{
							if(i > 0 || isUID)
								ImapSend( " ", 1);
							
							BOOL isText = FALSE;
							
							sprintf(cmd, "BODY[TEXT] {%u}\r\n", Letter->GetSummary()->m_mime->m_data->m_end - Letter->GetSummary()->m_mime->m_data->m_beg);
							
							ImapSend( cmd, strlen(cmd));


							int nRead = 0;
							int tLen = Letter->GetSummary()->m_mime->m_data->m_end - Letter->GetSummary()->m_mime->m_data->m_beg;
							while(1)
							{
								if(nRead >= tLen)
									break;
								char sztmp[1025];
								dynamic_mmap_copy(sztmp, lbuf, Letter->GetSummary()->m_mime->m_data->m_beg + nRead, (tLen - nRead) > 1024 ? 1024 : (tLen - nRead) );
								sztmp[(tLen - nRead) > 1024 ? 1024 : (tLen - nRead)] = '\0';

								nRead += (tLen - nRead) > 1024 ? 1024 : (tLen - nRead);

								if(ImapSend(sztmp, strlen(sztmp)) < 0)
								{
									break;
								}
							}
							
						}
						else if(strcasecmp(vItem[i].c_str(), "ENVELOPE") == 0)
						{
							string strIntTime;
							OutTimeString(m_maillisttbl[x].mtime, strIntTime);
							string strEnve;
							strEnve = "ENVELOPE (";
							
							strEnve += "\"";
							strEnve += strIntTime.c_str();
							strEnve += "\"";
							strEnve += " ";
							
							strEnve += "\"";
							strEnve += Letter->GetSummary()->m_header->GetSubject();
							strEnve += "\"";
							strEnve += " ";
							
							/*from info*/
							if(Letter->GetSummary()->m_header->GetFrom())
							{
								strEnve += "(";
								for(int k = 0; k < Letter->GetSummary()->m_header->GetFrom()->m_addrs.size(); k++)
								{
									strEnve += "(";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetFrom()->m_addrs[k].pName;
									strEnve += "\"";
									strEnve += " ";
									
									strEnve += "NIL";
									strEnve += " ";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetFrom()->m_addrs[k].mName;
									strEnve += "\"";
									strEnve += " ";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetFrom()->m_addrs[k].dName;
									strEnve += "\"";
									
									strEnve += ")";
								}
								strEnve += ") ";
							}
							else
							{
								strEnve += "NIL ";
							}
							
							
							/*Sender info*/
							if(Letter->GetSummary()->m_header->GetSender())
							{
								strEnve += "(";
								for(int k = 0; k < Letter->GetSummary()->m_header->GetSender()->m_addrs.size(); k++)
								{
									strEnve += "(";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetSender()->m_addrs[k].pName;
									strEnve += "\"";
									strEnve += " ";
									
									strEnve += "NIL";
									strEnve += " ";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetSender()->m_addrs[k].mName;
									strEnve += "\"";
									strEnve += " ";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetSender()->m_addrs[k].dName;
									strEnve += "\"";
									
									strEnve += ")";
								}
								strEnve += ") ";
							}
							else
							{
								if(Letter->GetSummary()->m_header->GetFrom())
								{
									strEnve += "(";
									for(int k = 0; k < Letter->GetSummary()->m_header->GetFrom()->m_addrs.size(); k++)
									{
										strEnve += "(";
										
										strEnve += "\"";
										strEnve += Letter->GetSummary()->m_header->GetFrom()->m_addrs[k].pName;
										strEnve += "\"";
										strEnve += " ";
										
										strEnve += "NIL";
										strEnve += " ";
										
										strEnve += "\"";
										strEnve += Letter->GetSummary()->m_header->GetFrom()->m_addrs[k].mName;
										strEnve += "\"";
										strEnve += " ";
										
										strEnve += "\"";
										strEnve += Letter->GetSummary()->m_header->GetFrom()->m_addrs[k].dName;
										strEnve += "\"";
										
										strEnve += ")";
									}
									strEnve += ") ";
								}
								else
								{
									strEnve += "NIL ";
								}
							}
							
							/* Reply-To */
							if(Letter->GetSummary()->m_header->GetReplyTo())
							{
								strEnve += "(";
								for(int k = 0; k < Letter->GetSummary()->m_header->GetReplyTo()->m_addrs.size(); k++)
								{
									strEnve += "(";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetReplyTo()->m_addrs[k].pName;
									strEnve += "\"";
									strEnve += " ";
									
									strEnve += "NIL";
									strEnve += " ";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetReplyTo()->m_addrs[k].mName;
									strEnve += "\"";
									strEnve += " ";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetReplyTo()->m_addrs[k].dName;
									strEnve += "\"";
									
									strEnve += ")";
								}
								strEnve += ") ";
							}
							else
							{
								if(Letter->GetSummary()->m_header->GetFrom())
								{
									strEnve += "(";
									for(int k = 0; k < Letter->GetSummary()->m_header->GetFrom()->m_addrs.size(); k++)
									{
										strEnve += "(";
										
										strEnve += "\"";
										strEnve += Letter->GetSummary()->m_header->GetFrom()->m_addrs[k].pName;
										strEnve += "\"";
										strEnve += " ";
										
										strEnve += "NIL";
										strEnve += " ";
										
										strEnve += "\"";
										strEnve += Letter->GetSummary()->m_header->GetFrom()->m_addrs[k].mName;
										strEnve += "\"";
										strEnve += " ";
										
										strEnve += "\"";
										strEnve += Letter->GetSummary()->m_header->GetFrom()->m_addrs[k].dName;
										strEnve += "\"";
										
										strEnve += ")";
									}
									strEnve += ") ";
								}
								else
								{
									strEnve += "NIL ";
								}
							}
							
							
							/* To */
							if(Letter->GetSummary()->m_header->GetTo())
							{
								strEnve += "(";
								for(int k = 0; k < Letter->GetSummary()->m_header->GetTo()->m_addrs.size(); k++)
								{
									strEnve += "(";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetTo()->m_addrs[k].pName;
									strEnve += "\"";
									strEnve += " ";
									
									strEnve += "NIL";
									strEnve += " ";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetTo()->m_addrs[k].mName;
									strEnve += "\"";
									strEnve += " ";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetTo()->m_addrs[k].dName;
									strEnve += "\"";
									
									strEnve += ")";
								}
								strEnve += ") ";
							}
							else
							{
								strEnve += "NIL ";
							}
							
							/* CC */
							if(Letter->GetSummary()->m_header->GetCc())
							{
								strEnve += "(";
								for(int k = 0; k < Letter->GetSummary()->m_header->GetCc()->m_addrs.size(); k++)
								{
									strEnve += "(";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetCc()->m_addrs[k].pName;
									strEnve += "\"";
									strEnve += " ";
									
									strEnve += "NIL";
									strEnve += " ";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetCc()->m_addrs[k].mName;
									strEnve += "\"";
									strEnve += " ";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetCc()->m_addrs[k].dName;
									strEnve += "\"";
									
									strEnve += ")";
								}
								strEnve += ") ";
							}
							else
							{
								strEnve += "NIL ";
							}
							
							
							/*BCC*/
							if(Letter->GetSummary()->m_header->GetBcc())
							{
								strEnve += "(";
								for(int k = 0; k < Letter->GetSummary()->m_header->GetBcc()->m_addrs.size(); k++)
								{
									strEnve += "(";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetBcc()->m_addrs[k].pName;
									strEnve += "\"";
									strEnve += " ";
									
									strEnve += "NIL";
									strEnve += " ";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetBcc()->m_addrs[k].mName;
									strEnve += "\"";
									strEnve += " ";
									
									strEnve += "\"";
									strEnve += Letter->GetSummary()->m_header->GetBcc()->m_addrs[k].dName;
									strEnve += "\"";
									
									strEnve += ")";
								}
								strEnve += ") ";
							}
							else
							{
								strEnve += "NIL ";
							}
							
							
							if(strcmp(Letter->GetSummary()->m_header->GetInReplyTo(), "") == 0)
							{
								strEnve += "NIL ";
							}
							else
							{
								strEnve += "\"";
								strEnve += Letter->GetSummary()->m_header->GetInReplyTo();
								strEnve += "\" ";
							}
							
							if(strcmp(Letter->GetSummary()->m_header->GetMessageID(), "") == 0)
							{
								strEnve += "NIL ";
							}
							else
							{
								strEnve += "\"";
								strEnve += Letter->GetSummary()->m_header->GetMessageID();
								strEnve += "\"";
							}
							strEnve += ")";
							
							if(i > 0 || isUID)
								ImapSend( " ", 1);

							ImapSend(strEnve.c_str(), strEnve.length());
						}
						else if(strcasecmp(vItem[i].c_str(), "FLAGS") == 0)
						{
							unsigned int status = m_maillisttbl[x].mstatus;
							string strFlags = ""; 
							if((status & MSG_ATTR_ANSWERED) != 0)
							{
								if(strFlags == "")
									strFlags += "\\Answered";
								else
									strFlags += " \\Answered";
							}
							if((status & MSG_ATTR_DELETED) != 0)
							{
								if(strFlags == "")
									strFlags += "\\Deleted";
								else
									strFlags += " \\Deleted";
							}
							if((status & MSG_ATTR_DRAFT) != 0)
							{
								if(strFlags == "")
									strFlags += "\\Draft";
								else
									strFlags += " \\Draft";
							}
							if((status & MSG_ATTR_FLAGGED) != 0)
							{
								if(strFlags == "")
									strFlags += "\\Flagged";
								else
									strFlags += " \\Flagged";
							}
							if((time(NULL) - m_maillisttbl[x].mtime) < RECENT_MSG_TIME)
							{
								if(strFlags == "")
									strFlags += "\\Recent";
								else
									strFlags += " \\Recent";
							}
							if((status & MSG_ATTR_SEEN) != 0)
							{
								if(strFlags == "")
									strFlags += "\\Seen";
								else
									strFlags += " \\Seen";
							}
							sprintf(cmd, "FLAGS (%s)", strFlags.c_str());
							
							if(i > 0 || isUID)
								ImapSend( " ", 1);
							ImapSend(cmd, strlen(cmd));
						}
						else if(strcasecmp(vItem[i].c_str(), "INTERNALDATE") == 0)
						{
							sprintf(cmd, "INTERNALDATE \"%s\"",Letter->GetSummary()->m_header->GetDate());
							if(i > 0 || isUID)
								ImapSend( " ", 1);
							ImapSend( cmd, strlen(cmd));
						}
						else if(strcasecmp(vItem[i].c_str(), "RFC822.SIZE") == 0)
						{
							sprintf(cmd, "RFC822.SIZE %u", Letter->GetSize());
							if(i > 0 || isUID)
								ImapSend( " ", 1);
							ImapSend( cmd, strlen(cmd));
							
							
						}
						else if(strcasecmp(vItem[i].c_str(), "UID") == 0)
						{
							if(!isUID)
							{
								sprintf(cmd, "UID %u", m_maillisttbl[x].mid);
								if(i > 0)
									ImapSend( " ", 1);
								ImapSend( cmd, strlen(cmd));
							}
						}
						
					}
				}

				sprintf(cmd, ")\r\n");
				ImapSend(cmd, strlen(cmd));
				
				delete Letter;
			}
		}
		
	}
}

void CMailImap::Store(const char* szArg, BOOL isUID)
{	
	MailStorage* mailStg;
	StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
	if(!mailStg)
	{
		printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
		return;
	}
	char cmd[512];
	string strSequence, strMsgItem;
	unsigned int nBegin, nEnd;
	strcut(szArg, NULL, " ", strSequence);
	strcut(szArg, " ", "\r\n", strMsgItem);
    strtrim(strSequence);
    strtrim(strMsgItem);
	vector<string> vecSequenceSet;
	vSplitString(strSequence, vecSequenceSet, ",");
	int zLen = vecSequenceSet.size();
	for(int z = 0; z < zLen; z++)
	{
		vector<string> vecSequenceRange;
		strSequence = vecSequenceSet[z];
		
		vSplitString(strSequence, vecSequenceRange, ":");
		if(vecSequenceRange.size() == 1)
		{
			nBegin = nEnd = atoi(vecSequenceRange[0].c_str());
		}
		else if(vecSequenceRange.size() == 2)
		{
			nBegin = atoi(vecSequenceRange[0].c_str());
			if(vecSequenceRange[1] == "*")
			{
				nEnd = 0x7FFFFFFFU;
			}
			else
			{
				nEnd = atoi(vecSequenceRange[1].c_str());
			}
		}
		
		int vLen = m_maillisttbl.size();
		for(int x = 0; x < vLen; x++)
		{
			if(((isUID ? m_maillisttbl[x].mid : x + 1) >= nBegin) && ((isUID ? m_maillisttbl[x].mid : x + 1) <= nEnd))
			{
				if(strncasecmp(strMsgItem.c_str(),"FLAGS.SILENT ", sizeof("FLAGS.SILENT ") - 1) == 0)
				{
					mailStg->SetMailStatus(m_username.c_str(), m_maillisttbl[x].mid, 0);
					if(strMsgItem.find("\\Seen", 0, sizeof("\\Seen") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_SEEN;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Answered", 0, sizeof("\\Answered") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_ANSWERED;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Flagged", 0, sizeof("\\Flagged") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_FLAGGED;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Deleted", 0, sizeof("\\Deleted") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_DELETED;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Draft", 0, sizeof("\\Draft") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_DRAFT;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Recent", 0, sizeof("\\Recent") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_RECENT;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
				}
				else if(strncasecmp(strMsgItem.c_str(),"FLAGS ", sizeof("FLAGS ") - 1) == 0)
				{
					mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, 0);
					if(strMsgItem.find("\\Seen", 0, sizeof("\\Seen") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_SEEN;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Answered", 0, sizeof("\\Answered") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_ANSWERED;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Flagged", 0, sizeof("\\Flagged") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_FLAGGED;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Deleted", 0, sizeof("\\Deleted") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_DELETED;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Draft", 0, sizeof("\\Draft") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_DRAFT;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Recent", 0, sizeof("\\Recent") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_RECENT;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					unsigned int status;
					mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
					string strStatus = "";
					if((status & MSG_ATTR_SEEN) == MSG_ATTR_SEEN)
					{
						if(strStatus == "")
							strStatus += "\\Seen";
						else
							strStatus += " \\Seen";
					}
					if((status & MSG_ATTR_ANSWERED) == MSG_ATTR_ANSWERED)
					{
						if(strStatus == "")
							strStatus += "\\Answered";
						else
							strStatus += " \\Answered";
					}
					if((status & MSG_ATTR_FLAGGED) == MSG_ATTR_FLAGGED)
					{
						if(strStatus == "")
							strStatus += "\\Flagged";
						else
							strStatus += " \\Flagged";
					}
					if((status & MSG_ATTR_DELETED) == MSG_ATTR_DELETED)
					{
						if(strStatus == "")
							strStatus += "\\Deleted";
						else
							strStatus += " \\Deleted";
					}
					if((status & MSG_ATTR_DRAFT) == MSG_ATTR_DRAFT)
					{
						if(strStatus == "")
							strStatus += "\\Draft";
						else
							strStatus += " \\Draft";
					}
					if((time(NULL) - m_maillisttbl[x].mtime) < RECENT_MSG_TIME)
					{
						if(strStatus == "")
							strStatus += "\\Recent";
						else
							strStatus += " \\Recent";
					}
					if(isUID)
						sprintf(cmd ,"* %u FETCH (FLAGS (%s)) UID %u\r\n", x+1, strStatus.c_str(), m_maillisttbl[x].mid);
					else
						sprintf(cmd ,"* %u FETCH (FLAGS (%s))\r\n", x+1, strStatus.c_str());
					ImapSend( cmd, strlen(cmd));
				}
				else if(strncasecmp(strMsgItem.c_str(),"+FLAGS.SILENT ", sizeof("+FLAGS.SILENT ") - 1) == 0)
				{
					if(strMsgItem.find("\\Seen", 0, sizeof("\\Seen") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_SEEN;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Answered", 0, sizeof("\\Answered") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_ANSWERED;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Flagged", 0, sizeof("\\Flagged") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_FLAGGED;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Deleted", 0, sizeof("\\Deleted") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_DELETED;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Draft", 0, sizeof("\\Draft") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_DRAFT;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Recent", 0, sizeof("\\Recent") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_RECENT;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
				}
				else if(strncasecmp(strMsgItem.c_str(),"+FLAGS ", sizeof("+FLAGS ") - 1) == 0)
				{
					if(strMsgItem.find("\\Seen", 0, sizeof("\\Seen") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_SEEN;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Answered", 0, sizeof("\\Answered") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_ANSWERED;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Flagged", 0, sizeof("\\Flagged") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_FLAGGED;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Deleted", 0, sizeof("\\Deleted") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_DELETED;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Draft", 0, sizeof("\\Draft") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_DRAFT;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Recent", 0, sizeof("\\Recent") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status |= MSG_ATTR_RECENT;
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					unsigned int status;
					mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
					string strStatus = "";
					if((status & MSG_ATTR_SEEN) == MSG_ATTR_SEEN)
					{
						if(strStatus == "")
							strStatus += "\\Seen";
						else
							strStatus += " \\Seen";
					}
					if((status & MSG_ATTR_ANSWERED) == MSG_ATTR_ANSWERED)
					{
						if(strStatus == "")
							strStatus += "\\Answered";
						else
							strStatus += " \\Answered";
					}
					if((status & MSG_ATTR_FLAGGED) == MSG_ATTR_FLAGGED)
					{
						if(strStatus == "")
							strStatus += "\\Flagged";
						else
							strStatus += " \\Flagged";
					}
					if((status & MSG_ATTR_DELETED) == MSG_ATTR_DELETED)
					{
						if(strStatus == "")
							strStatus += "\\Deleted";
						else
							strStatus += " \\Deleted";
					}
					if((status & MSG_ATTR_DRAFT) == MSG_ATTR_DRAFT)
					{
						if(strStatus == "")
							strStatus += "\\Draft";
						else
							strStatus += " \\Draft";
					}
					if((time(NULL) - m_maillisttbl[x].mtime) < RECENT_MSG_TIME)
					{
						if(strStatus == "")
							strStatus += "\\Recent";
						else
							strStatus += " \\Recent";
					}
					if(isUID)
						sprintf(cmd ,"* %u FETCH (FLAGS (%s)) UID %u\r\n",x+1, strStatus.c_str(), m_maillisttbl[x].mid);
					else
						sprintf(cmd ,"* %u FETCH (FLAGS (%s))\r\n", x+1, strStatus.c_str());
					ImapSend( cmd, strlen(cmd));
					
				}
				else if(strncasecmp(strMsgItem.c_str(),"-FLAGS ", sizeof("-FLAGS ") - 1) == 0)
				{
					if(strMsgItem.find("\\Seen", 0, sizeof("\\Seen") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status &= (~MSG_ATTR_SEEN);
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Answered", 0, sizeof("\\Answered") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status &= (~MSG_ATTR_ANSWERED);
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Flagged", 0, sizeof("\\Flagged") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status &= (~MSG_ATTR_FLAGGED);
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Deleted", 0, sizeof("\\Deleted") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status &= (~MSG_ATTR_DELETED);
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Draft", 0, sizeof("\\Draft") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status &= (~MSG_ATTR_DRAFT);
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Recent", 0, sizeof("\\Recent") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status &= (~MSG_ATTR_RECENT);
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					unsigned int status;
					mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
					string strStatus = "";
					if((status & MSG_ATTR_SEEN) == MSG_ATTR_SEEN)
					{
						if(strStatus == "")
							strStatus += "\\Seen";
						else
							strStatus += " \\Seen";
					}
					if((status & MSG_ATTR_ANSWERED) == MSG_ATTR_ANSWERED)
					{
						if(strStatus == "")
							strStatus += "\\Answered";
						else
							strStatus += " \\Answered";
					}
					if((status & MSG_ATTR_FLAGGED) == MSG_ATTR_FLAGGED)
					{
						if(strStatus == "")
							strStatus += "\\Flagged";
						else
							strStatus += " \\Flagged";
					}
					if((status & MSG_ATTR_DELETED) == MSG_ATTR_DELETED)
					{
						if(strStatus == "")
							strStatus += "\\Deleted";
						else
							strStatus += " \\Deleted";
					}
					if((status & MSG_ATTR_DRAFT) == MSG_ATTR_DRAFT)
					{
						if(strStatus == "")
							strStatus += "\\Draft";
						else
							strStatus += " \\Draft";
					}
					if((time(NULL) - m_maillisttbl[x].mtime) < RECENT_MSG_TIME)
					{
						if(strStatus == "")
							strStatus += "\\Recent";
						else
							strStatus += " \\Recent";
					}
					if(isUID)
						sprintf(cmd ,"* %u FETCH (FLAGS (%s)) UID %u\r\n", x+1, strStatus.c_str(), m_maillisttbl[x].mid);
					else
						sprintf(cmd ,"* %u  FETCH (FLAGS (%s))\r\n", x+1, strStatus.c_str());
					ImapSend( cmd, strlen(cmd));
				}
				else if(strncasecmp(strMsgItem.c_str(),"-FLAGS.SILENT ", sizeof("-FLAGS.SILENT ") - 1) == 0)
				{
					if(strMsgItem.find("\\Seen", 0, sizeof("\\Seen") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status &= (~MSG_ATTR_SEEN);
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Answered", 0, sizeof("\\Answered") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status &= (~MSG_ATTR_ANSWERED);
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Flagged", 0, sizeof("\\Flagged") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status &= (~MSG_ATTR_FLAGGED);
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Deleted", 0, sizeof("\\Deleted") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status &= (~MSG_ATTR_DELETED);
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Draft", 0, sizeof("\\Draft") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status &= (~MSG_ATTR_DRAFT);
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
					if(strMsgItem.find("\\Recent", 0, sizeof("\\Recent") - 1) != string::npos)
					{
						unsigned int status;
						mailStg->GetMailStatus(m_maillisttbl[x].mid, status);
						status &= (~MSG_ATTR_RECENT);
						mailStg->SetMailStatus(m_username.c_str(),m_maillisttbl[x].mid, status);
					}
				}
			}
		}
	}
}

int CMailImap::Copy(const char* szArg, BOOL isUID)
{	
	MailStorage* mailStg;
	StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
	if(!mailStg)
	{
		printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
		return -1;
	}
	char cmd[512];
	string strSequence, strMsgItem;
	unsigned int nBegin, nEnd;
	strcut(szArg, NULL, " ", strSequence);
	strcut(szArg, " ", "\r\n", strMsgItem);
    strtrim(strSequence);
    strtrim(strMsgItem);
	vector<string> vecSequenceSet;
	vSplitString(strSequence, vecSequenceSet, ",");
	int zLen = vecSequenceSet.size();
	for(int z = 0; z < zLen; z++)
	{
		vector<string> vecSequenceRange;
		strSequence = vecSequenceSet[z];
		
		vSplitString(strSequence, vecSequenceRange, ":");
		if(vecSequenceRange.size() == 1)
		{
			nBegin = nEnd = atoi(vecSequenceRange[0].c_str());
		}
		else if(vecSequenceRange.size() == 2)
		{
			nBegin = atoi(vecSequenceRange[0].c_str());
			if(vecSequenceRange[1] == "*")
			{
				nEnd = 0x7FFFFFFFU;
			}
			else
			{
				nEnd = atoi(vecSequenceRange[1].c_str());
			}
		}
		
		strtrim(strMsgItem,"\" ");
		int dirID;
		if(mailStg->GetDirID(m_username.c_str(), strMsgItem.c_str(), dirID) == -1)
		{
			return -1;
		}
		int vLen = m_maillisttbl.size();
		
		for(int x = 0; x < vLen; x++)
		{
			if(((isUID ? m_maillisttbl[x].mid : x + 1) >= nBegin) && ((isUID ? m_maillisttbl[x].mid : x + 1) <= nEnd))
			{
				char newuid[256];
				sprintf(newuid, "%08x_%08x_%016lx_%08x_%s", time(NULL), getpid(), pthread_self(), random(), m_localhostname.c_str());
				
				unsigned long long usermaxsize;
				if(mailStg->GetUserSize(m_username.c_str(), usermaxsize) == -1)
				{
					usermaxsize = 5000*1024;
				}
				
				MailLetter* oldLetter, *newLetter;

				string emlfile;
				mailStg->GetMailIndex(m_maillisttbl[x].mid, emlfile);
				
				oldLetter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
				newLetter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, newuid, usermaxsize /*mailStg, 
					"", "", mtLocal, newuid, dirID, m_maillisttbl[x].mstatus, (unsigned int)time(NULL), usermaxsize*/);
				
				Letter_Info letter_info;
				letter_info.mail_from = "";
				letter_info.mail_to = "";
				letter_info.mail_type = mtLocal;
				letter_info.mail_uniqueid = newuid;
				letter_info.mail_dirid = dirID;
				letter_info.mail_status = m_maillisttbl[x].mstatus;
				letter_info.mail_time = time(NULL);
				letter_info.user_maxsize = usermaxsize;
				letter_info.mail_id = -1;
				
				int wlen = 0;
				BOOL isOK = TRUE;
                
                char read_buf[4096];
                int read_count = 0;
                while((read_count = oldLetter->Read(read_buf, 4096)) >= 0)
                {
                    if(read_count > 0)
                    {
                        if(newLetter->Write(read_buf, read_count) < 0)
                        {
                            isOK = FALSE;
                            break;
                        }
                    }
                }
				if(isOK)
					newLetter->SetOK();
				newLetter->Close();
				if(newLetter->isOK())
				{
					mailStg->InsertMailIndex(letter_info.mail_from.c_str(), letter_info.mail_to.c_str(),letter_info.mail_time,
						letter_info.mail_type, letter_info.mail_uniqueid.c_str(), letter_info.mail_dirid, letter_info.mail_status,
						newLetter->GetEmlName(), newLetter->GetSize(), letter_info.mail_id);
				}
				delete oldLetter;
				delete newLetter;
			}
		}
	}
	return 0;
}

void CMailImap::ParseFetch(const char* text, vector<string>& vDst)
{
	int isFlag = 0;
	int sLen = strlen(text);
	BOOL inSemicolon = FALSE;
	BOOL isTransferred = FALSE;
	string strCell = "";
	char tmp[3];
	for(int i = 0; i <= sLen; i++)
	{
		if(isTransferred)
		{
			memset(tmp, 0, 3);
			memcpy(tmp, &text[i], 1);
			strCell += tmp;
			isTransferred = FALSE;
		}
		else
		{
			if(((text[i] == ' ')||
				(text[i] == '\r')||(text[i] == '\n')||(text[i] == '\0'))
				&&(!inSemicolon)&&(isFlag == 0))
			{
				vDst.push_back(strCell);
				strCell = "";
				if((text[i] == '\r')||(text[i] == '\n'))
					break;
			}
			else if((text[i] == '[')&&(!inSemicolon))
			{
				isFlag++;
				memset(tmp, 0, 3);
				memcpy(tmp, &text[i], 1);
				strCell += tmp;
			}
			else if((text[i] == ']')&&(!inSemicolon))
			{
				isFlag--;
				memset(tmp, 0, 3);
				memcpy(tmp, &text[i], 1);
				strCell += tmp;
			}
			else if(text[i] == '\"')
			{
				if(inSemicolon == TRUE)
				{
					inSemicolon = FALSE;
				}
				else
				{
					inSemicolon = TRUE;
				}
			}
			else if(text[i] == '\\')
			{
				isTransferred = TRUE;
			}
			else
			{
				memset(tmp, 0, 3);
				memcpy(tmp, &text[i], 1);
				strCell += tmp;
			}
		}
	}
}

void CMailImap::ParseCommand(const char* text, vector<string>& vDst)
{
	int sLen = strlen(text);
	BOOL inSemicolon = FALSE;
	BOOL isTransferred = FALSE;
	string strCell = "";
	char tmp[3];
	for(int i = 0; i <= sLen; i++)
	{
		if(isTransferred)
		{
			memset(tmp, 0, 3);
			memcpy(tmp, &text[i], 1);
			strCell += tmp;
			isTransferred = FALSE;
		}
		else
		{
			if(((text[i] == ' ')||(text[i] == '\r')||(text[i] == '\n')||(text[i] == '\0'))&&(!inSemicolon))
			{
				vDst.push_back(strCell);
				strCell = "";
				if((text[i] == '\r')||(text[i] == '\n'))
					break;
			}
			else if(text[i] == '\"')
			{
				if(inSemicolon == TRUE)
				{
					inSemicolon = FALSE;
				}
				else
				{
					inSemicolon = TRUE;
				}
			}
			else if(text[i] == '\\')
			{
				isTransferred = TRUE;
			}
			else
			{
				memset(tmp, 0, 3);
				memcpy(tmp, &text[i], 1);
				strCell += tmp;
			}
		}
	}
}

BOOL CMailImap::Parse(char* text)
{
	//printf("%s", text);
	string strNotag;
	strcut(text, " ", " ", strNotag);
	
	//Any state
	BOOL retValue = TRUE;
	
	if(strncasecmp(strNotag.c_str(), "CAPABILITY", sizeof("CAPABILITY") - 1) == 0)
	{
		On_Capability(text);
	}
	else if(strncasecmp(strNotag.c_str(), "NOOP", sizeof("NOOP") - 1) == 0)
	{
		
		On_Noop(text);

	}
	else if(strncasecmp(strNotag.c_str(), "LOGOUT", sizeof("LOGOUT") - 1) == 0)
	{
		On_Logout(text);
		return FALSE;
	}
	
	//Non-athenticated state
	else if(strncasecmp(strNotag.c_str(), "AUTHENTICATE", sizeof("AUTHENTICATE") - 1) == 0)
	{

		retValue = On_Authenticate(text);

	}
	else if(strncasecmp(strNotag.c_str(), "LOGIN", sizeof("LOGIN") - 1) == 0)
	{
		
		retValue = On_Login(text);
		
	}
	else if(strncasecmp(strNotag.c_str(), "SELECT", sizeof("SELECT") - 1) == 0)
	{
		
		On_Select(text);

	}
	else if(strncasecmp(strNotag.c_str(), "EXAMINE", sizeof("EXAMINE") - 1) == 0)
	{
		
		On_Examine(text);

	}
	else if(strncasecmp(strNotag.c_str(), "CREATE", sizeof("CREATE") - 1) == 0)
	{
		
		On_Create(text);
		
	}
	else if(strncasecmp(strNotag.c_str(), "DELETE", sizeof("DELETE") - 1) == 0)
	{
		
		On_Delete(text);
		
	}
	else if(strncasecmp(strNotag.c_str(), "RENAME", sizeof("RENAME") - 1) == 0)
	{
		
		On_Rename(text);

		
	}
	else if(strncasecmp(strNotag.c_str(), "SUBSCRIBE", sizeof("SUBSCRIBE") - 1) == 0)
	{
		
		On_Subscribe(text);
		
	}
	else if(strncasecmp(strNotag.c_str(), "UNSUBSCRIBE", sizeof("UNSUBSCRIBE") - 1) == 0)
	{
		
		On_Unsubscribe(text);
		
	}
	else if(strncasecmp(strNotag.c_str(), "LIST", sizeof("LIST") - 1) == 0)
	{
		
		On_List(text);
		
	}
	else if(strncasecmp(strNotag.c_str(), "LSUB", sizeof("LSUB") - 1) == 0)
	{
		
		On_Listsub(text);
	}
	else if(strncasecmp(strNotag.c_str(), "STATUS", sizeof("STATUS") - 1) == 0)
	{
		
		On_Status(text);
		
	}
	else if(strncasecmp(strNotag.c_str(), "APPEND", sizeof("APPEND") - 1) == 0)
	{
		
		On_Append(text);

		
	}
	else if(strncasecmp(strNotag.c_str(), "CHECK", sizeof("CHECK") - 1) == 0)
	{
		On_Check(text);
	}
	else if(strncasecmp(strNotag.c_str(), "CLOSE", sizeof("CLOSE") - 1) == 0)
	{
		
		On_Close(text);

		
	}
	else if(strncasecmp(strNotag.c_str(), "EXPUNGE", sizeof("EXPUNGE") - 1) == 0)
	{
		
		On_Expunge(text);

		
	}
	else if(strncasecmp(strNotag.c_str(), "SEARCH", sizeof("SEARCH") - 1) == 0)
	{
		
		On_Search(text);

		
	}
	else if(strncasecmp(strNotag.c_str(), "FETCH", sizeof("FETCH") - 1) == 0)
	{
		
		On_Fetch(text);

		
	}
	else if(strncasecmp(strNotag.c_str(), "STORE", sizeof("STORE") - 1) == 0)
	{
		
		On_Store(text);

		
	}
	else if(strncasecmp(strNotag.c_str(), "COPY", sizeof("COPY") - 1) == 0)
	{
		
		On_Copy(text);

		
	}
	else if(strncasecmp(strNotag.c_str(), "UID", sizeof("UID") - 1) == 0)
	{
		
		On_UID(text);

	}
	else if(strncasecmp(strNotag.c_str(), "STARTTLS", sizeof("STARTTLS") - 1) == 0)
	{
		On_STARTTLS(text);
	}
	else if(strncasecmp(strNotag.c_str(), "X", sizeof("X") - 1) == 0)
	{
		On_X_atom(text);
	}
	else if(strncasecmp(strNotag.c_str(),"NAMESPACE", sizeof("NAMESPACE") - 1) == 0)
	{
		On_Namespace(text);
	}
	else
	{
		On_Unknown(text);
	}
	return retValue;
}

void CMailImap::On_Service_Error()
{
	char cmd[1024];
	sprintf(cmd, "* NO %s IMAP4rev1 Server is not ready yet\r\n", m_email_domain.c_str());
	ImapSend( cmd, strlen(cmd));
}

void CMailImap::ParseSearch(const char* text, vector<Search_Item>& vDst)
{
	int sLen = strlen(text);
	int nBracket = 0;
	BOOL inSemicolon = FALSE;
	BOOL isTransferred = FALSE;
	BOOL isDeep = FALSE;
	string strCell = "";
	char tmp[3];
	for(int i = 0; i <= sLen; i++)
	{
		if(isTransferred)
		{
			memset(tmp, 0, 3);
			memcpy(tmp, &text[i], 1);
			strCell += tmp;
			isTransferred = FALSE;
		}
		else
		{
			unsigned char value = (unsigned char)text[i];
			if(value > 0x7F)
			{
				memset(tmp, 0, 3);
				memcpy(tmp, &text[i], 2);
				strCell += tmp;
				i++;
			}
			else if(((text[i] == ' ')||(text[i] == '\n')||(text[i] == '\0'))&&(!inSemicolon)&&(nBracket == 0))
			{
				Search_Item tmp;
				tmp.item = strCell;
				tmp.isDeep= isDeep;
				vDst.push_back(tmp);
				strCell = "";
				isDeep = FALSE;
				if(text[i] == '\0')
					break;
			}
			else if(text[i] == '\r')
			{
				//ignore
			}
			else if(text[i] == '\\')
			{
				isTransferred = TRUE;
			}
			else if(text[i] == '\"')
			{
				memset(tmp, 0, 3);
				memcpy(tmp, &text[i], 1);
				strCell += tmp;
				
				if(nBracket == 0)
				{
					if(inSemicolon == TRUE)
					{
						inSemicolon = FALSE;
					}
					else
					{
						inSemicolon = TRUE;
					}
				}
			}
			else if((text[i] == '(')&&(!inSemicolon))
			{					
				if(nBracket > 0)
				{
					memset(tmp, 0, 3);
					memcpy(tmp, &text[i], 1);
					strCell += tmp;
				}
				else
				{
					isDeep = TRUE;
				}
				nBracket++;
				
			}
			else if((text[i] == ')')&&(!inSemicolon))
			{
				nBracket--;
				if(nBracket > 0)
				{
					memset(tmp, 0, 3);
					memcpy(tmp, &text[i], 1);
					strCell += tmp;
				}
				
			}
			else
			{
				memset(tmp, 0, 3);
				memcpy(tmp, &text[i], 1);
				strCell += tmp;
			}
		}
	}
}


void CMailImap::SearchRecv(const char* szTag, const char* szCmd, string & strsearch)
{
	if(strmatch("*{*}\r\n", strsearch.c_str()))
	{
		char cmd[1024];
		sprintf(cmd, "%s OK %s Completed\r\n", szTag, szCmd);
		ImapSend(cmd, strlen(cmd));
		
		unsigned int clen;
		sscanf(strsearch.c_str(), "%*[^{]{%d}\r\n", &clen);
		char* cbuf = new char[clen + 4096];
		memset(cbuf, 0, clen + 4096);
		if(ProtRecv(cbuf, clen + 4095) > 0)
		{
			if(strcmp(cbuf, "") != 0)
			{
				strsearch += cbuf;
				delete[] cbuf;
				SearchRecv(szTag, szCmd, strsearch);
			}
			else
			{
				delete[] cbuf;
			}
		}
	}
}

void CMailImap::Search(const char * szTag, const char* szCmd, const char* szArg)
{
	MailStorage* mailStg;
	StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
	if(!mailStg)
	{
		printf("%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
		return;
	}
	string strsearch;
	strsearch = szArg;
	
	SearchRecv(szTag, szCmd, strsearch);
	
	mailStg->ListMailByDir((char*)m_username.c_str(), m_maillisttbl, (char*)m_strSelectedDir.c_str());
	map<int,int> vSearchResult;
	int listtbllen = m_maillisttbl.size();
	for(int x = 0; x < listtbllen; x++)
	{
		vSearchResult.insert(map<int, int>::value_type(m_maillisttbl[x].mid, 1));
	}
	
	SubSearch(mailStg, strsearch.c_str(), vSearchResult);
	
	map<int, int>::iterator iter;
	
	string resp;
	resp = "* SEARCH";
	char cmd[16];
	int count = 0;
	for(iter = vSearchResult.begin(); iter != vSearchResult.end(); iter++)
	{
		if(iter->second == 1)
		{
			count++;
			sprintf(cmd, " %d", iter->first);
			resp += cmd;
		}
	}
	resp += "\r\n";
	if(count > 0)
		ImapSend(resp.c_str(), resp.length());
}

void CMailImap::SubSearch(MailStorage* mailStg, const char* szArg, map<int,int>& vSearchResult)
{
	Search_Relation eSearchRelation = ANDing;
	BOOL bIsNot = FALSE;
	
	vector<Search_Item> vItem;
	ParseSearch(szArg, vItem);
	int vItemLength = vItem.size();
	int listtbllen = m_maillisttbl.size();
	for(int i = 0; i < vItemLength; i++)
	{
		if(vItem[i].isDeep)
		{
			SubSearch(mailStg, vItem[i].item.c_str(), vSearchResult);
		}
		else
		{
			if(vItem[i].item== "ALL")
			{
				for(int y = 0; y < listtbllen; y++)
				{
					vSearchResult[m_maillisttbl[y].mid] = 1;
				}
			}
			else if(vItem[i].item == "ANSWERED")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_ANSWERED) != MSG_ATTR_ANSWERED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_ANSWERED) == MSG_ATTR_ANSWERED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_ANSWERED) == MSG_ATTR_ANSWERED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_ANSWERED) != MSG_ATTR_ANSWERED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
					}
				}
			}
			else if( vItem[i].item == "BCC")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
				
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string bcc = Letter->GetSummary()->m_header->GetBcc()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(bcc.find(condition.c_str(), 0,condition.length()) == string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string bcc = Letter->GetSummary()->m_header->GetBcc()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(bcc.find(condition.c_str(), 0,condition.length()) != string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string bcc = Letter->GetSummary()->m_header->GetBcc()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(bcc.find(condition.c_str(), 0,condition.length()) != string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
						
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string bcc = Letter->GetSummary()->m_header->GetBcc()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(bcc.find(condition.c_str(), 0,condition.length()) == string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				i++;
			}
			else if(vItem[i].item == "BEFORE")
			{
				unsigned int year1, month1, day1;
				
				char szmon[32];
				sscanf(vItem[i+1].item.c_str(), "%d-%[^-]-%d",&day1, szmon, &year1);
				month1 = getmonthnumber(szmon);
				
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) <= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) >= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) >= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) <= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				i++;
			}
			else if(vItem[i].item == "BODY")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						bIsNot = FALSE;
					}
					else
					{
						
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						bIsNot = FALSE;
					}
					else
					{
						
					}
				}
			}
			else if(vItem[i].item == "CC")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string cc = Letter->GetSummary()->m_header->GetCc()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(cc.find(condition.c_str(), 0,condition.length()) == string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string cc = Letter->GetSummary()->m_header->GetCc()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(cc.find(condition.c_str(), 0,condition.length()) != string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string cc = Letter->GetSummary()->m_header->GetCc()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(cc.find(condition.c_str(), 0,condition.length()) != string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
						
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string cc = Letter->GetSummary()->m_header->GetCc()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(cc.find(condition.c_str(), 0,condition.length()) == string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				i++;
			}
			else if(vItem[i].item == "DELETED")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_DELETED) != MSG_ATTR_DELETED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_DELETED) == MSG_ATTR_DELETED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_DELETED) == MSG_ATTR_DELETED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_DELETED) != MSG_ATTR_DELETED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
					}
				}
			}
			else  if(vItem[i].item == "DRAFT")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_DRAFT) != MSG_ATTR_DRAFT)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_DRAFT) == MSG_ATTR_DRAFT)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_DRAFT) == MSG_ATTR_DRAFT)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_DRAFT) != MSG_ATTR_DRAFT)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
					}
				}
			}
			else  if(vItem[i].item == "FLAGGED")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_FLAGGED) != MSG_ATTR_FLAGGED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_FLAGGED) == MSG_ATTR_FLAGGED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_FLAGGED) == MSG_ATTR_FLAGGED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_FLAGGED) != MSG_ATTR_FLAGGED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
					}
				}
			}
			else if(vItem[i].item == "SUBJECT")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						if(strmatch("{*}", vItem[i+1].item.c_str()))
						{
							for(int y = 0; y < listtbllen; y++)
							{
								string emlfile;
								mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
								
								MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
								if(Letter->GetSize()> 0)
								{
									string subject = Letter->GetSummary()->m_header->GetDecodedSubject();
									string condition = strmatch("\"*\"", vItem[i+2].item.c_str()) ? vItem[i+2].item.substr(1, vItem[i+2].item.length() - 2) : vItem[i+2].item;
									if(subject.find(condition.c_str(), 0, condition.length()) == string::npos)
									{
										vSearchResult[m_maillisttbl[y].mid] = 1;
									}
								}
								if(Letter)
									delete Letter;
							}
							i += 2;
						}
						else
						{
							for(int y = 0; y < listtbllen; y++)
							{
								string emlfile;
								mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
								
								MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
								if(Letter->GetSize()> 0)
								{
									string subject = Letter->GetSummary()->m_header->GetDecodedSubject();
									string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
									if(subject.find(condition.c_str(), 0, condition.length()) == string::npos)
									{
										vSearchResult[m_maillisttbl[y].mid] = 1;
									}
								}
								if(Letter)
									delete Letter;
							}
							i += 1;
						}
						bIsNot = FALSE;
					}
					else
					{
						if(strmatch("{*}", vItem[i+1].item.c_str()))
						{
							for(int y = 0; y < listtbllen; y++)
							{
								string emlfile;
								mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
								
								MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
								if(Letter->GetSize()> 0)
								{
									string subject = Letter->GetSummary()->m_header->GetDecodedSubject();
									string condition = strmatch("\"*\"", vItem[i+2].item.c_str()) ? vItem[i+2].item.substr(1, vItem[i+2].item.length() - 2) : vItem[i+2].item;
									if(subject.find(condition.c_str(), 0, condition.length()) != string::npos)
									{
										vSearchResult[m_maillisttbl[y].mid] = 1;
									}
								}
								if(Letter)
									delete Letter;
							}
							i += 2;
						}
						else
						{
							for(int y = 0; y < listtbllen; y++)
							{
								string emlfile;
								mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
								
								MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
								if(Letter->GetSize()> 0)
								{
									string subject = Letter->GetSummary()->m_header->GetDecodedSubject();
									string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
									if(subject.find(condition.c_str(), 0, condition.length()) != string::npos)
									{
										vSearchResult[m_maillisttbl[y].mid] = 1;
									}
								}
								if(Letter)
									delete Letter;
							}
							i += 1;
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						if(strmatch("{*}", vItem[i+1].item.c_str()))
						{
							for(int y = 0; y < listtbllen; y++)
							{
								string emlfile;
								mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
								
								MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
								if(Letter->GetSize()> 0)
								{
									string subject = Letter->GetSummary()->m_header->GetDecodedSubject();
									string condition = strmatch("\"*\"", vItem[i+2].item.c_str()) ? vItem[i+2].item.substr(1, vItem[i+2].item.length() - 2) : vItem[i+2].item;
									if(subject.find(condition.c_str(), 0, condition.length()) != string::npos)
									{
										vSearchResult[m_maillisttbl[y].mid] = 0;
									}
								}
								if(Letter)
									delete Letter;
							}
							i += 2;
						}
						else
						{
							for(int y = 0; y < listtbllen; y++)
							{
								string emlfile;
								mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
								
								MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
								if(Letter->GetSize()> 0)
								{
									string subject = Letter->GetSummary()->m_header->GetDecodedSubject();
									string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
									if(subject.find(condition.c_str(), 0, condition.length()) != string::npos)
									{
										vSearchResult[m_maillisttbl[y].mid] = 0;
									}
								}
								if(Letter)
									delete Letter;
							}
							i += 1;
						}
						bIsNot = FALSE;
					}
					else
					{
						if(strmatch("{*}", vItem[i+1].item.c_str()))
						{
							for(int y = 0; y < listtbllen; y++)
							{
								string emlfile;
								mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
								
								MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
								if(Letter->GetSize()> 0)
								{
									string subject = Letter->GetSummary()->m_header->GetDecodedSubject();
									string condition = strmatch("\"*\"", vItem[i+2].item.c_str()) ? vItem[i+2].item.substr(1, vItem[i+2].item.length() - 2) : vItem[i+2].item;
									if(subject.find(condition.c_str(), 0, condition.length()) == string::npos)
									{
										vSearchResult[m_maillisttbl[y].mid] = 0;
									}
								}
								if(Letter)
									delete Letter;
							}
							i += 2;
						}
						else
						{
							for(int y = 0; y < listtbllen; y++)
							{
								string emlfile;
								mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
								
								MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
								if(Letter->GetSize()> 0)
								{
									string subject = Letter->GetSummary()->m_header->GetDecodedSubject();
									string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
									if(subject.find(condition.c_str(), 0, condition.length()) == string::npos)
									{
										vSearchResult[m_maillisttbl[y].mid] = 0;
									}
								}
								if(Letter)
									delete Letter;
							}
							i += 1;
						}
					}
				}
			}
			else if(vItem[i].item == "HEADER")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						bIsNot = FALSE;
					}
					else
					{
						
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						bIsNot = FALSE;
					}
					else
					{
						
					}
				}
				i++;			
			}
			else if(vItem[i].item == "KEYWORD")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						bIsNot = FALSE;
					}
					else
					{
						
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						bIsNot = FALSE;
					}
					else
					{
						
					}
				}
				i++;
			}
			else if( vItem[i].item == "LARGER")
			{
				int letterSize = atoi(vItem[i+1].item.c_str());
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize() <= letterSize)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize() >= letterSize)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize() >= letterSize)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
							if(Letter)
								delete Letter;
						}
						
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize() <= letterSize)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				i++;
			}
			else if( vItem[i].item == "NEW")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((time(NULL) - m_maillisttbl[y].mtime > RECENT_MSG_TIME)&&
								((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) == MSG_ATTR_SEEN))
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((time(NULL) - m_maillisttbl[y].mtime < RECENT_MSG_TIME)&&
								((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) != MSG_ATTR_SEEN))
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((time(NULL) - m_maillisttbl[y].mtime < RECENT_MSG_TIME)&&
								((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) != MSG_ATTR_SEEN))
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((time(NULL) - m_maillisttbl[y].mtime > RECENT_MSG_TIME)&&
								((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) == MSG_ATTR_SEEN))
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
					}
				}
			}
			else if( vItem[i].item == "NOT")
			{
				bIsNot = TRUE;
			}
			else if( vItem[i].item.c_str() == "OLD")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((time(NULL) - m_maillisttbl[y].mtime < RECENT_MSG_TIME) && 
								((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) != MSG_ATTR_SEEN))
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((time(NULL) - m_maillisttbl[y].mtime > RECENT_MSG_TIME)&&
								((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) == MSG_ATTR_SEEN))
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((time(NULL) - m_maillisttbl[y].mtime > RECENT_MSG_TIME)&&
								((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) == MSG_ATTR_SEEN))
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((time(NULL) - m_maillisttbl[y].mtime < RECENT_MSG_TIME)&&
								((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) != MSG_ATTR_SEEN))
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
					}
				}
			}
			else if(vItem[i].item == "ON")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						bIsNot = FALSE;
					}
					else
					{
						
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						bIsNot = FALSE;
					}
					else
					{
						
					}
				}
			}
			else if( vItem[i].item == "OR")
			{
				eSearchRelation = ORing;
			}
			else  if(vItem[i].item.c_str() == "RECENT")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if(time(NULL) - m_maillisttbl[y].mtime > RECENT_MSG_TIME)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if(time(NULL) - m_maillisttbl[y].mtime < RECENT_MSG_TIME)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if(time(NULL) - m_maillisttbl[y].mtime < RECENT_MSG_TIME)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if(time(NULL) - m_maillisttbl[y].mtime > RECENT_MSG_TIME)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
					}
				}
			}
			else  if(vItem[i].item == "SEEN")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) != MSG_ATTR_SEEN)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) == MSG_ATTR_SEEN)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) == MSG_ATTR_SEEN)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) != MSG_ATTR_SEEN)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
					}
				}
			}
			else if(vItem[i].item == "SENTBEFORE")
			{
				unsigned int year1, month1, day1;
				char szmon[32];
				sscanf(vItem[i+1].item.c_str(), "%d-%[^-]-%d",&day1, szmon, &year1);
				month1 = getmonthnumber(szmon);
				
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) <= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) >= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) >= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) <= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				i++;
			}
			else if(vItem[i].item == "SENTON")
			{
				unsigned int year1, month1, day1;
				
				char szmon[32];
				sscanf(vItem[i+1].item.c_str(), "%d-%[^-]-%d",&day1, szmon, &year1);
				month1 = getmonthnumber(szmon);
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) != 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) == 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) == 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) != 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				i++;
			}
			else if(vItem[i].item == "SENTSINCE")
			{
				unsigned int year1, month1, day1;
				
				char szmon[32];
				sscanf(vItem[i+1].item.c_str(), "%d-%[^-]-%d",&day1, szmon, &year1);
				month1 = getmonthnumber(szmon);
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) >= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) <= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) <= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) >= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				i++;
			}
			else if(vItem[i].item == "SINCE")
			{
				unsigned int year1, month1, day1;
				
				char szmon[32];
				sscanf(vItem[i+1].item.c_str(), "%d-%[^-]-%d",&day1, szmon, &year1);
				month1 = getmonthnumber(szmon);
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) >= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) <= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) <= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								if(datecmp(year1, month1, day1, Letter->GetSummary()->m_header->GetTm()->tm_year + 1900, Letter->GetSummary()->m_header->GetTm()->tm_mon, Letter->GetSummary()->m_header->GetTm()->tm_mday) >= 0)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				i++;
			}
			else if( vItem[i].item == "SMALLER")
			{
				int letterSize = atoi(vItem[i+1].item.c_str());
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize() >= letterSize)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize() <= letterSize)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize() <= letterSize)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
							if(Letter)
								delete Letter;
						}
						
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize() >= letterSize)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				i++;
			}
			else if(vItem[i].item == "FROM")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string from = Letter->GetSummary()->m_header->GetFrom()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(from.find(condition.c_str(), 0,condition.length()) == string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string from = Letter->GetSummary()->m_header->GetFrom()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(from.find(condition.c_str(), 0,condition.length()) != string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string from = Letter->GetSummary()->m_header->GetFrom()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(from.find(condition.c_str(), 0,condition.length()) != string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
						
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string from = Letter->GetSummary()->m_header->GetFrom()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(from.find(condition.c_str(), 0,condition.length()) == string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				i++;
			}
			else if(vItem[i].item == "TEXT")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						bIsNot = FALSE;
					}
					else
					{
						
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						bIsNot = FALSE;
					}
					else
					{
						
					}
				}
			}
			else if(vItem[i].item == "TO")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string to = Letter->GetSummary()->m_header->GetTo()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(to.find(condition.c_str(), 0,condition.length()) == string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string to = Letter->GetSummary()->m_header->GetTo()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(to.find(condition.c_str(), 0,condition.length()) != string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 1;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string to = Letter->GetSummary()->m_header->GetTo()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(to.find(condition.c_str(), 0,condition.length()) != string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
						
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							string emlfile;
							mailStg->GetMailIndex(m_maillisttbl[y].mid, emlfile);
							
							MailLetter* Letter = new MailLetter(mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_memcached, emlfile.c_str());
							if(Letter->GetSize()> 0)
							{
								string to = Letter->GetSummary()->m_header->GetTo()->m_strfiled;
								string condition = strmatch("\"*\"", vItem[i+1].item.c_str()) ? vItem[i+1].item.substr(1, vItem[i+1].item.length() - 2) : vItem[i+1].item;
								if(to.find(condition.c_str(), 0,condition.length()) == string::npos)
								{
									vSearchResult[m_maillisttbl[y].mid] = 0;
								}
							}
							if(Letter)
								delete Letter;
						}
					}
				}
				i++;
			}
			else if(vItem[i].item == "UID")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						bIsNot = FALSE;
					}
					else
					{
						
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						bIsNot = FALSE;
					}
					else
					{
						
					}
				}
			}
			else  if(vItem[i].item == "UNANSWERED")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_ANSWERED) == MSG_ATTR_ANSWERED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_ANSWERED) != MSG_ATTR_ANSWERED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_ANSWERED) != MSG_ATTR_ANSWERED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_ANSWERED) == MSG_ATTR_ANSWERED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
					}
				}
			}
			else  if(vItem[i].item == "UNDELETED")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_DELETED) == MSG_ATTR_DELETED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_DELETED) != MSG_ATTR_DELETED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_DELETED) != MSG_ATTR_DELETED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_DELETED) == MSG_ATTR_DELETED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
					}
				}
			}
			else  if(vItem[i].item == "UNFLAGGED")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_FLAGGED) == MSG_ATTR_FLAGGED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_FLAGGED) != MSG_ATTR_FLAGGED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_FLAGGED) != MSG_ATTR_FLAGGED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_FLAGGED) == MSG_ATTR_FLAGGED)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
					}
				}
			}
			else  if(vItem[i].item == "UNKEYWORD")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						bIsNot = FALSE;
					}
					else
					{
						
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						bIsNot = FALSE;
					}
					else
					{
						
					}
				}
			}
			else  if(vItem[i].item == "UNSEEN")
			{
				if(eSearchRelation == ORing)
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) == MSG_ATTR_SEEN)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) != MSG_ATTR_SEEN)
							{
								vSearchResult[m_maillisttbl[y].mid] = 1;
							}
						}
					}
				}
				else
				{
					if(bIsNot == TRUE)
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) != MSG_ATTR_SEEN)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
						bIsNot = FALSE;
					}
					else
					{
						for(int y = 0; y < listtbllen; y++)
						{
							if((m_maillisttbl[y].mstatus & MSG_ATTR_SEEN) == MSG_ATTR_SEEN)
							{
								vSearchResult[m_maillisttbl[y].mid] = 0;
							}
						}
					}
				}
			}
			else
			{
				//do nothing
			}
		}
	}
}


