/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _MIME_H_
#define _MIME_H_

#include "util/general.h"
#include <string>
#include <vector>
#include <fstream>
#include "fstring.h"
#include "util/base64.h"
#include "util/qp.h"
#include "util/escape.h"

using namespace std;

typedef struct _mailaddr_
{
	string pName; /* Personal Name */
	string mName; /* Mailbox name */
	string dName; /* Domain Name */
} mailaddr;

class filedaddr
{
public:
	filedaddr();
	filedaddr(const char* sAddr);
	virtual ~filedaddr();
	string m_strfiled;
	vector<mailaddr> m_addrs;
};

class filedContentType
{
public:
	filedContentType();
	filedContentType(const char* sContentType);
	virtual ~filedContentType();

	string m_type1;
	string m_type2;
	string m_boundary;
	string m_charset;
	string m_filename;
};

void __inline__ get_safe_var(const char *szIn, string& strOut)
{	
	strOut = "";
	int i;
	for(;;)
	{
		if(szIn[i] == '\0')
		{
			break;
		}
		else if(szIn[i] == '+')
		{
			strOut += " ";
			i++;
		}
		else if(szIn[i] == '%')
		{
			char temp[4];
			char strc[2];
			sprintf(temp,"%c%c", szIn[i+1], szIn[i+2]);
			sscanf(temp, "%x", &strc[0]);
			strc[1] = '\0';
			strOut += strc;
			i += 3;
		}
		else
		{
			char strc[2];
			strc[0] = szIn[i];
			strc[1] = '\0';
			strOut += strc;
			i++;
		}
	}
}

void __inline__ DecodeMIMEString(const char* szMIMEString, string& strDeocoded)
{
	string strMIMEString = szMIMEString;
	string strBefore = "", strAfter = "", strCodec = "";
	while(1)
	{
		if((strMIMEString.find("=?", 0, strlen("=?")) != string::npos)&&(strMIMEString.find("?B?", 0, strlen("?B?")) != string::npos)&&(strMIMEString.find("?=", 0, strlen("?=")) != string::npos))
		{
			strcut(strMIMEString.c_str(), NULL, "=?", strBefore);
			strcut(strMIMEString.c_str(), "=?", "?B?", strCodec);
			strcut(strMIMEString.c_str(), "?B?", "?=", strDeocoded);
			strcut(strMIMEString.c_str(), "?=", NULL, strAfter);	

			int tmplen = BASE64_DECODE_OUTPUT_MAX_LEN(strMIMEString.length());
			char * tmpbuf = (char*)malloc(tmplen);
			memset(tmpbuf, 0, tmplen);
			CBase64::Decode((char*)strDeocoded.c_str(), strDeocoded.length(), tmpbuf, &tmplen);

			if(strCodec != "")
			{
				string strConvert;
				code_convert_ex(strCodec.c_str(), CMailBase::m_encoding.c_str(), tmpbuf, strConvert);
				strDeocoded = strBefore + strConvert + strAfter;
			}
			else
			{
				strDeocoded = strBefore + tmpbuf + strAfter;
			}
			
			free(tmpbuf);
			
			//DecodeMIMEString(strDeocoded.c_str(), strDeocoded);
			strMIMEString = strDeocoded.c_str();
				
		}
		else if((strMIMEString.find("=?", 0, strlen("=?")) != string::npos)&&(strMIMEString.find("?Q?", 0, strlen("?Q?")) != string::npos)&&(strMIMEString.find("?=", 0, strlen("?=")) != string::npos))
		{
			strcut(strMIMEString.c_str(), NULL, "=?", strBefore);
			strcut(strMIMEString.c_str(), "=?", "?Q?", strCodec);
			strcut(strMIMEString.c_str(), "?Q?", "?=", strDeocoded);
			strcut(strMIMEString.c_str(), "?=", NULL, strAfter);
			
			
			char * tmpbuf = (char*)malloc(strDeocoded.length() + 1);
			int tmplen = strDeocoded.length() + 1;
			memset(tmpbuf, 0, tmplen);
			DecodeQuoted(strDeocoded.c_str(), strDeocoded.length()/3*3, (unsigned char*)tmpbuf);

			if(strCodec != "")
			{
				string strConvert;
				code_convert_ex(strCodec.c_str(), CMailBase::m_encoding.c_str(), tmpbuf, strConvert);
				strDeocoded = strBefore + strConvert + strAfter;
			}
			else
			{
				strDeocoded = strBefore + tmpbuf + strAfter;
			}
			
			free(tmpbuf);
			
			//DecodeMIMEString(strDeocoded.c_str(), strDeocoded);
			strMIMEString = strDeocoded.c_str();
		}
		else
		{
			strDeocoded = strMIMEString;
			break;
		}
	}
}


class filedContentDisposition
{
public:
	filedContentDisposition()
	{

	}
	filedContentDisposition(const char* sContentDisposition)
	{
		m_filename = "";
		m_disposition = "";
		fnfy_strcut(sContentDisposition, ":", " \t\"\r\n", " \t\";\r\n", m_disposition);
		if(strcasestr(sContentDisposition, "filename=") != NULL)
		{
			fnfy_strcut(sContentDisposition, "filename="," \t\r\n\"",";\"\r\n", m_filename);
			
		}
		else if(strcasestr(sContentDisposition, "filename*=") != NULL)
		{
			fnfy_strcut(sContentDisposition, "filename*="," \t\r\n\"", ";\"\r\n", m_filename);
		}
		else
		{
			string strtmp = "";
			int f = 0;
			for(;;)
			{
				char szbuf[64];
				sprintf(szbuf, "filename*%d=", f);
				if(strcasestr(sContentDisposition, szbuf) != NULL)
				{
					strtmp = "";
					fnfy_strcut(sContentDisposition, "filename=","\r\n\"",";\"\r\n", strtmp);
					m_filename += strtmp;
				}
				else
					break;
				f++;
			}
		}

		
		string tmp = m_filename;
		DecodeMIMEString(tmp.c_str(), m_filename);
	};
	virtual ~filedContentDisposition() {};

	string m_disposition;
	string m_filename;
};

class mimeblock
{
public:
	mimeblock(){ m_beg = 0; m_end = 0; }
	virtual ~mimeblock() {};
	
	unsigned int m_linenumber;
	unsigned int m_blocksize;
	unsigned int m_beg;
	unsigned int m_end;
};

class mimepart : public mimeblock
{
public:
	mimepart(const char* buffer, vector<fbufseg>* pvbufseg, unsigned int beg, unsigned int end);
	virtual ~mimepart();
	filedContentType* GetContentType();
	const char* GetContentID() {  return m_Content_ID.c_str(); }
	const char* GetContentTransferEncoding() { return m_Content_Transfer_Encoding.c_str(); }
	filedContentDisposition* GetContentDisposition() { return m_Content_Disposition; }
	
	unsigned int GetSubPartNumber();
	mimepart* GetPart(unsigned int x);

	mimeblock m_header;
	mimeblock m_data;
	vector<fbufseg>* GetSegs() { return m_pfbufseg; }
	const char* Buffer() { return m_buffer;}
protected:
	char* m_buffer;
	vector<fbufseg>* m_pfbufseg;
	vector<mimepart*> m_vmimepart;
	
	filedContentType* m_Content_Type;
	string m_Content_ID;
	string m_Content_Transfer_Encoding;
	filedContentDisposition* m_Content_Disposition;

private:
	void parse();
};

class MIME
{
public:
	MIME(const char* buf, int len);
	virtual ~MIME();
	
	void GlobalParse();
	void HeaderParse();
	
	filedaddr* GetFrom();
	filedaddr* GetTo();
	filedaddr* GetSender();
	filedaddr* GetReplyTo();
	filedaddr* GetCc();
	filedaddr* GetBcc();
	const char* GetSubject();
	const char* GetDecodedSubject();
	const char* GetDate();
	struct tm* GetTm();
	const char* GetMessageID();
	const char* GetInReplyTo();
		
	const char* getfield(unsigned int x, string& fstrbuf);
	
	mimepart * m_rootpart;
	
private:

	BOOL m_parsedGlobal;
	BOOL m_parsedHeader;
	char* m_buffer;
	int m_buffer_len;
	vector<fbufseg> m_vfbufseg;

	filedaddr* m_From;
	filedaddr* m_Sender;	/* Sender */
	filedaddr* m_ReplyTo; /* Reply-To: */
	filedaddr* m_To;
	filedaddr* m_CC;
	filedaddr* m_BCC;
	string m_Subject;
	string m_DecodedSubject;
	string m_Message_ID;
	string m_User_Agent;
	string m_In_Reply_To;
	string m_Date;
	struct tm m_tm;
	string m_Mime_Version;
	
public:
	string m_strMimeVersion;

};

#endif /* _MIME_H_ */

