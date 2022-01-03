/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _WEBMAIL_H_
#define _WEBMAIL_H_

#include <mqueue.h>
#include <semaphore.h>
#include <libmemcached/memcached.h>
#include "util/base64.h"
#include "util/qp.h"
#include "util/escape.h"
#include "util/DES.h"
#include "util/security.h"
#include "base.h"
#include "storage.h"
#include "letter.h"
#include "mime.h"
#include "fstring.h"
#include "http.h"
#include "email.h"
#include "cache.h"
#include "service.h"
#include "stgengine.h"
#include "posixname.h"
#include "postnotify.h"

#define ERISEMAIL_WEB_SERVER_NAME        "Server: eRisemail Web Server/1.6.10 (*NIX)"

#define RSP_200_OK_CACHE			"HTTP/1.1 200 OK\r\n"   \
                                    ERISEMAIL_WEB_SERVER_NAME "\r\n"    \
                                    "Cache-Control: max-age=31536000\r\n"
                                    
#define RSP_200_OK_NO_CACHE			"HTTP/1.1 200 OK\r\n"   \
                                    ERISEMAIL_WEB_SERVER_NAME "\r\n"   \
                                    "Pragma: no-cache\r\n"   \
                                    "Cache-Control: no-cache, must-revalidate\r\n"   \
                                    "Expires: 0\r\n"
                                    
#define RSP_200_OK_XML				"HTTP/1.1 200 OK\r\n"   \
                                    ERISEMAIL_WEB_SERVER_NAME "\r\n"   \
                                    "Pragma: no-cache\r\n"   \
                                    "Cache-Control: no-cache, must-revalidate\r\n"   \
                                    "Expires: 0\r\n"   \
                                    "Content-Type: text/xml\r\n"
                                    
#define RSP_200_OK_JSON				"HTTP/1.1 200 OK\r\n"   \
                                    ERISEMAIL_WEB_SERVER_NAME "\r\n"   \
                                    "Pragma: no-cache\r\n"   \
                                    "Cache-Control: no-cache, must-revalidate\r\n"   \
                                    "Expires: 0\r\n"   \
                                    "Content-Type: text/json\r\n"


#define RSP_200_OK_CACHE_PLAIN		"HTTP/1.1 200 OK\r\n"   \
                                    ERISEMAIL_WEB_SERVER_NAME "\r\n"   \
                                    "Content-Type: text/plain\r\n"
                                    
#define RSP_200_OK_NO_CACHE_PLAIN	"HTTP/1.1 200 OK\r\n"   \
                                    ERISEMAIL_WEB_SERVER_NAME "\r\n"   \
                                    "Pragma: no-cache\r\n"   \
                                    "Cache-Control: no-cache, must-revalidate\r\n"   \
                                    "Expires: 0\r\n"   \
                                    "Content-Type: text/plain\r\n"

#define RSP_301_MOVED_NO_CACHE		"HTTP/1.1 301 Moved Permanently\r\n"   \
                                    ERISEMAIL_WEB_SERVER_NAME "\r\n"
                                    
#define RSP_304_NOT_MODIFIED		"HTTP/1.1 304 Not Modified\r\n"   \
                                    ERISEMAIL_WEB_SERVER_NAME "\r\n"

/* Non html body (Content-Length: 0) */
#define RSP_404_NOT_FOUND			"HTTP/1.1 404 Not Found\r\n"   \
                                    ERISEMAIL_WEB_SERVER_NAME "\r\n"   \
                                    "Content-Length: 0\r\n"   \
                                    "\r\n"
                                    
#define RSP_500_SYS_ERR				"HTTP/1.1 500 Server Error\r\n"   \
                                    ERISEMAIL_WEB_SERVER_NAME "\r\n"   \
                                    "Content-Length: 0\r\n"   \
                                    "\r\n"
                                    
#define RSP_501_SYS_ERR				"HTTP/1.1 501 Not Implemented\r\n"   \
                                    ERISEMAIL_WEB_SERVER_NAME "\r\n"   \
                                    "Content-Length: 0\r\n"   \
                                    "\r\n"


typedef struct
{
	string tmpname;
	string filename;
	string filetype1;
	string filetype2;
	unsigned int size;

} AttInfo;

class doc
{
protected:
	CHttp * m_session;
	memcached_st * m_memcached;
public:
	doc(CHttp* session)
	{
		m_session = session;
        m_memcached = m_session->GetMemCached();
	}
	virtual ~doc(){}
	
	virtual void Response()
	{
		string strExtName;
		string strResp;
		string strDoc;
		string strPage;

        string strHTTPDate;
		OutHTTPDateString(time(NULL), strHTTPDate);
                
		strDoc = m_session->GetQueryPage();
        
		if(m_session->m_cache->m_htdoc.find(strDoc) == m_session->m_cache->m_htdoc.end())
		{
			strDoc = CMailBase::m_html_path.c_str();
			strDoc += "/";
			strDoc += m_session->GetQueryPage();
			ifstream* fs_doc = new ifstream(strDoc.c_str(), ios_base::binary);
			string strline;
			if(!fs_doc || !fs_doc->is_open())
			{
                // 404 NOT FOUND
				strResp = RSP_404_NOT_FOUND;
				m_session->HttpSend(strResp.c_str(), strResp.length());
			}
			else
			{   
                string strLastModifiedDate;
                struct stat file_stat;
                if( stat(strDoc.c_str(), &file_stat) == 0)
                {
                    OutHTTPDateString(file_stat.st_mtime, strLastModifiedDate);
                }
                
                if(strcasecmp(m_session->GetIfModifiedSince(), strLastModifiedDate.c_str()) == 0)
                {
                    strResp = RSP_304_NOT_MODIFIED;
                    strResp += "Date: ";
                    strResp += strHTTPDate;
                    strResp += "\r\n";
                    strResp += "Content-Length: 0\r\n";
                    strResp += "\r\n";
                    
                    m_session->EnableKeepAlive(TRUE);
                    m_session->HttpSend(strResp.c_str(), strResp.length());
                }
                else
                {
                    lowercase(m_session->GetQueryPage(), strPage);				
                    get_extend_name(strPage.c_str(), strExtName);
                    
                    strResp = RSP_200_OK_CACHE;
                    
                    
                    strResp += "Date: ";
                    strResp += strHTTPDate;
                    strResp += "\r\n";
                    
                    if(strLastModifiedDate != "")
                    {
                        strResp += "Last-Modified: ";
                        strResp += strLastModifiedDate;
                        strResp += "\r\n";
                    }
                    strResp += "Content-Type: ";
                    
                    
                    if(m_session->m_cache->m_type_table.find(strExtName) == m_session->m_cache->m_type_table.end())
                    {
                        strResp += "application/*";
                    }
                    else
                    {
                        strResp += m_session->m_cache->m_type_table[strExtName];
                    }
                    strResp += "\r\n";
                    
                    char szContentLength[64];
                    sprintf(szContentLength, "%u", file_stat.st_size);
                    strResp += "Content-Length: ";
                    strResp += szContentLength;
                    strResp += "\r\n";
                    strResp += "\r\n";
                    
                    m_session->EnableKeepAlive(TRUE);
                    m_session->HttpSend(strResp.c_str(), strResp.length());
                    
                    char rbuf[1449];
                    while(1)
                    {
                        if(fs_doc->eof())
                        {
                            break;
                        }
                        //if(fs_doc->read(rbuf, 1448) < 0)
                        //{
                        //    break;
                        //}
                        fs_doc->read(rbuf, 1448);
                        int rlen = fs_doc->gcount();
                        m_session->HttpSend(rbuf, rlen);
                    }
                }
			}
			if(fs_doc && fs_doc->is_open())
            {
				fs_doc->close();
            }
            if(fs_doc)
                delete fs_doc;
            
		}
		else
		{
            string strLastModifiedDate;
            OutHTTPDateString(m_session->m_cache->m_htdoc[strDoc].last_modified, strLastModifiedDate);
                
            if(strcasecmp(m_session->GetIfModifiedSince(), strLastModifiedDate.c_str()) == 0)
            {
                strResp = RSP_304_NOT_MODIFIED;
                strResp += "Date: ";
                strResp += strHTTPDate;
                strResp += "\r\n";
                strResp += "Content-Length: 0\r\n";
                strResp += "\r\n";
                
                m_session->EnableKeepAlive(TRUE);
                m_session->HttpSend(strResp.c_str(), strResp.length());
            }
            else
            {
                lowercase(m_session->GetQueryPage(), strPage);
                get_extend_name(strPage.c_str(), strExtName);
                
                strResp = RSP_200_OK_CACHE;
                strResp += "Date: ";
                strResp += strHTTPDate;
                strResp += "\r\n";

                strResp += "Last-Modified: ";
                strResp += strLastModifiedDate;
                strResp += "\r\n";
                
                strResp += "Content-Type: ";
                
                if(m_session->m_cache->m_type_table.find(strExtName) == m_session->m_cache->m_type_table.end())
                {
                    strResp += "application/*";
                }
                else
                {
                    strResp += m_session->m_cache->m_type_table[strExtName];
                }
                strResp += "\r\n";
                
                char szContentLength[64];
                sprintf(szContentLength, "%u", m_session->m_cache->m_htdoc[strDoc].flen);
                strResp += "Content-Length: ";
                strResp += szContentLength;
                strResp += "\r\n";
                strResp += "\r\n";
                
                m_session->EnableKeepAlive(TRUE);
                m_session->HttpSend(strResp.c_str(), strResp.length());
                m_session->HttpSend(m_session->m_cache->m_htdoc[strDoc].pbuf, m_session->m_cache->m_htdoc[strDoc].flen);
            }
		}
	}
protected:
	
	int generate_cookie_content(const char* szName, const char* szValue, const char* szPath, string & strOut)
	{
		strOut = szName;
		strOut += "=";
		strOut += szValue;
		strOut += "; Version=1; Path=";
		strOut += szPath;
		return 0;
	}
	
	void to_safety_querystring(const char* src, string& dst)
	{
		encodeURI((unsigned char *) src, dst);
	}
	
	const char* to_safty_xmlstring(string& str)
	{
		Replace(str, "&", "&amp;");
		Replace(str, "<", "&lt;");
		Replace(str, ">", "&gt;");
		Replace(str, "'", "&apos;");
		Replace(str, "\"", "&quot;");
		
		return str.c_str();
	}
	
	void ascii_to_htmlstring(string& str)
	{
		Replace(str, "&", "&amp;");
		
		Replace(str, "\r", "");
		Replace(str, "\n", "<BR>");
		Replace(str, " ", "&nbsp;");
		Replace(str, "\t", "&nbsp;&nbsp;&nbsp;&nbsp;");
		
		Replace(str, "<", "&lt;");
		Replace(str, ">", "&gt;");
		Replace(str, "'", "&apos;");
		Replace(str, "\"", "&quot;");
	}
	
    int check_userauth_token(const char* token, string & username)
    {
               
        string plain_username;
        strcut(token, NULL, "#", plain_username);
        
        string encryted_username_role;
        strcut(token, "#", NULL, encryted_username_role);
        
        string decryted_username_role;
        
		if(Security::Decrypt(encryted_username_role.c_str(), encryted_username_role.length(), decryted_username_role, CMailBase::DESKey()) == -1)
        {            
            return -1;
        }
		
        for(int x = 0; x < decryted_username_role.length(); x++)
        {
            if((decryted_username_role[x] >= 'a' && decryted_username_role[x] <= 'z') 
                || (decryted_username_role[x] >= 'A' && decryted_username_role[x] <= 'Z')
                || (decryted_username_role[x] >= '0' && decryted_username_role[x] <= '9')
                || (decryted_username_role[x] == '_' || decryted_username_role[x] == '.' || decryted_username_role[x] == ':'))
            {
                       
                string  decryted_username, decryted_role;
                strcut(decryted_username_role.c_str(), NULL, ":", decryted_username);
                strcut(decryted_username_role.c_str(), ":", ":", decryted_role);
                
                if(plain_username != "" && plain_username == decryted_username && decryted_role == "U")
                {
                    username = plain_username;
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
               return -1;
            }
        }
        
        return -1;
    }
    
    int check_adminauth_token(const char* token, string & username)
    {
               
        string plain_username;
        strcut(token, NULL, "#", plain_username);
        
        string encryted_username_role;
        strcut(token, "#", NULL, encryted_username_role);
        
        string decryted_username_role;
        
		if(Security::Decrypt(encryted_username_role.c_str(), encryted_username_role.length(), decryted_username_role, CMailBase::DESKey()) == -1)
        {            
            return -1;
        }
		
        for(int x = 0; x < decryted_username_role.length(); x++)
        {
            if((decryted_username_role[x] >= 'a' && decryted_username_role[x] <= 'z') 
                || (decryted_username_role[x] >= 'A' && decryted_username_role[x] <= 'Z')
                || (decryted_username_role[x] >= '0' && decryted_username_role[x] <= '9')
                || (decryted_username_role[x] == '_' || decryted_username_role[x] == '.' || decryted_username_role[x] == ':'))
            {
                       
                string  decryted_username, decryted_role;
                strcut(decryted_username_role.c_str(), NULL, ":", decryted_username);
                strcut(decryted_username_role.c_str(), ":", ":", decryted_role);
                
                if(plain_username != "" && plain_username == decryted_username && decryted_role == "A")
                {
                    username = plain_username;
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
               return -1;
            }
        }
        
        return -1;
    }
    
	void MailFileData(MimeSummary* part, string filename, string& type, string& dispos, fbuffer & fbuf, dynamic_mmap& lbuf)
	{
		if(part->GetSubPartNumber() == 0)
		{
			if(part->GetContentType() != NULL)
			{
				if(part->GetContentType()->m_filename == filename || ( part->GetContentDisposition() && part->GetContentDisposition()->m_filename == filename))
				{
					type = part->GetContentType()->m_type1;
					type += "/";
					
					type += part->GetContentType()->m_type2;
					
					dispos = (part->GetContentDisposition() == NULL ? "" : part->GetContentDisposition()->m_disposition);

					string strtext = "";
					for(int x = part->m_data->m_beg; x < part->m_data->m_end; x++)
					{
						strtext += lbuf[x];
						
						if(lbuf[x] == '\n')
						{
							if( strcasecmp(part->GetContentTransferEncoding(), "base64") == 0)
							{
								strtrim(strtext);
								if(strtext != "")
								{								
									int nlen = BASE64_DECODE_OUTPUT_MAX_LEN(strtext.length());
									char* szbuf = (char*)malloc(nlen + 1);
									memset(szbuf , 0, nlen + 1);
									CBase64::Decode((char*)strtext.c_str(), strtext.length(), szbuf, &nlen);
									fbuf.bufcat(szbuf, nlen);
									free(szbuf);
								}
							}
							else if(( strcasecmp(part->GetContentTransferEncoding(), "QP") == 0) || ( strcasecmp(part->GetContentTransferEncoding(), "quoted-printable") == 0)) 
							{
								strtrim(strtext);
								if(strtext != "")
								{								
									int nlen = strtext.length();
									char* szbuf = (char*)malloc(nlen + 1);
									memset(szbuf, 0, nlen + 1);
									DecodeQuoted(strtext.c_str(), strtext.length()/3*3, (unsigned char*)szbuf);
									fbuf.bufcat(szbuf, nlen/3);
									free(szbuf);
								}
							}
							else
							{
								fbuf.bufcat(strtext.c_str(), strtext.length());
							}
							strtext = "";
						}
					}
				}
			}
		}
		else
		{
			for(int a = 0; a < part->GetSubPartNumber(); a ++)
			{
				MailFileData(part->GetPart(a), filename, type, dispos, fbuf, lbuf);
			}
		}
	}
	
	void MailFileType(MimeSummary* part, string filename, string & type)
	{
		if(part->GetSubPartNumber() == 0)
		{
			if(part->GetContentType() != NULL)
			{
				if(part->GetContentType()->m_filename == filename || ( part->GetContentDisposition() && part->GetContentDisposition()->m_filename == filename))
				{
					if(part->GetContentType())
					{
						type = part->GetContentType()->m_type1;
						type += "/";
						type += part->GetContentType()->m_type2;
					}
				}
			}
		}
		else
		{
			for(int a = 0; a < part->GetSubPartNumber(); a ++)
			{
				MailFileType(part->GetPart(a), filename, type);
			}
		}
	}
	
	void MailFileDataEx(MimeSummary * part, string contentid, string& type, string& dispos, fbuffer & fbuf, dynamic_mmap& lbuf)
	{
		if(part->GetSubPartNumber() == 0)
		{
			if(part->GetContentType() != NULL)
			{
				if(part->GetContentID()== contentid)
				{
					type = part->GetContentType()->m_type1;
					type += "/";
					type += part->GetContentType()->m_type2;
					
					dispos = (part->GetContentDisposition() == NULL ? "" : part->GetContentDisposition()->m_disposition);
					
					string strtext = "";
					for(int x = part->m_data->m_beg; x < part->m_data->m_end; x++)
					{
						strtext += lbuf[x];
						
						if(lbuf[x] == '\n')
						{
							if( strcasecmp(part->GetContentTransferEncoding(), "base64") == 0)
							{
								strtrim(strtext);
								if(strtext != "")
								{								
									int nlen = BASE64_DECODE_OUTPUT_MAX_LEN(strtext.length());
									char* szbuf = (char*)malloc(nlen + 1);
									memset(szbuf , 0, nlen + 1);
									CBase64::Decode((char*)strtext.c_str(), strtext.length(), szbuf, &nlen);
									fbuf.bufcat(szbuf, nlen);
									free(szbuf);
								}
							}
							else if(( strcasecmp(part->GetContentTransferEncoding(), "QP") == 0) || ( strcasecmp(part->GetContentTransferEncoding(), "quoted-printable") == 0)) 
							{
								strtrim(strtext);
								if(strtext != "")
								{								
									int nlen = strtext.length();
									char* szbuf = (char*)malloc(nlen + 1);
									memset(szbuf, 0, nlen + 1);
									DecodeQuoted(strtext.c_str(), strtext.length()/3*3, (unsigned char*)szbuf);
									fbuf.bufcat(szbuf, nlen/3);
									free(szbuf);
								}
							}
							else
							{
								fbuf.bufcat(strtext.c_str(), strtext.length());
							}
							strtext = "";
						}
					}
				}
			}
		}
		else
		{
			for(int a = 0; a < part->GetSubPartNumber(); a ++)
			{
				MailFileDataEx(part->GetPart(a), contentid, type, dispos, fbuf, lbuf);
			}
		}
	}
	
	void ExtractAttach(const char* username, MimeSummary* part, vector<AttInfo>& attlist, dynamic_mmap& lbuf)
	{
		if(part->GetSubPartNumber() == 0)
		{
			if(part->GetContentType() != NULL)
			{
				if(strcasecmp(part->GetContentType()->m_type1.c_str(), "multipart") != 0 && strcasecmp(part->GetContentType()->m_type2.c_str(), "mixed") != 0)
				{
					if(part->GetContentType()->m_filename != "" || ( part->GetContentDisposition() && part->GetContentDisposition()->m_filename != ""))
					{
						AttInfo att;
						att.filename = part->GetContentType()->m_filename == "" ? part->GetContentDisposition()->m_filename : part->GetContentType()->m_filename;
						att.filetype1 = part->GetContentType()->m_type1;
						att.filetype2 = part->GetContentType()->m_type2;
						
						string strSrc, strDst;

						if(code_convert_ex(part->GetContentType()->m_charset.c_str(),CMailBase::m_encoding.c_str(), att.filename.c_str(), strSrc) != 0)
						{
							strSrc = att.filename;
						}
						
						encodeURI((const unsigned char*)strSrc.c_str(), strDst);
						att.filename = strDst;
						
						char szFileName[1024];
						sprintf(szFileName, "%s_%08x_%08x_%016lx_%08x.att", username, time(NULL), getpid(), pthread_self(), random());
						
						string strFilePath;
						strFilePath = CMailBase::m_private_path.c_str();
						strFilePath += "/tmp/";
						strFilePath += szFileName;
						
						ofstream* attachfd =  new ofstream(strFilePath.c_str(), ios_base::binary|ios::out|ios::trunc);
						string strTmp;		
						if(attachfd->is_open())
						{
							att.tmpname = szFileName;
							att.size = (part->m_data->m_end - part->m_data->m_beg)*3/4;
							attlist.push_back(att);
							
							int nWrite = 0;
							int tLen = part->m_end - part->m_beg;
							while(1)
							{
								if(nWrite >= tLen)
									break;
                                
                                char tmp_buf[1024];
                                dynamic_mmap_copy(tmp_buf, lbuf, part->m_beg + nWrite, (tLen - nWrite) > 1024 ? 1024 : (tLen - nWrite));
								attachfd->write(tmp_buf, (tLen - nWrite) > 1024 ? 1024 : (tLen - nWrite));
								nWrite += (tLen - nWrite) > 1024 ? 1024 : (tLen - nWrite);
							}
							attachfd->close();
						}
						if(attachfd)
							delete attachfd;
					}
				}
			}
		}
		else
		{
			for(int a = 0; a < part->GetSubPartNumber(); a++)
			{
				ExtractAttach(username, part->GetPart(a), attlist, lbuf);
			}
		}
	}
	
	void MailFileTypeEx(MimeSummary* part, string contentid, string & type)
	{
		if(part->GetSubPartNumber() == 0)
		{
			if(part->GetContentType() != NULL)
			{
				if(part->GetContentID()== contentid)
				{
					if(part->GetContentType())
					{
						type = part->GetContentType()->m_type1;
						type += "/";
						type += part->GetContentType()->m_type2;
					}
				}
			}
		}
		else
		{
			for(int a = 0; a < part->GetSubPartNumber(); a++)
			{
				MailFileTypeEx(part->GetPart(a), contentid, type);
			}
		}
	}

	BOOL isCalendar(MimeSummary* part)
	{
		if(part->GetSubPartNumber() == 0)
		{
			if(part->GetContentType() != NULL)
			{
				if((strcasecmp(part->GetContentType()->m_type1.c_str(), "text") == 0) && (part->GetContentType()->m_filename == ""))
				{
					if(strcasecmp(part->GetContentType()->m_type2.c_str(), "calendar") == 0)
					{
						return TRUE;
					}
					
				}
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
			for(int a = 0; a < part->GetSubPartNumber(); a++)
			{
				if(isCalendar(part->GetPart(a)) == TRUE)
				{
					return TRUE;
				}
			}

			return FALSE;
		}
	}
	
	void MailInfo(MimeSummary* part, string& strtext, string& strhtml, string& strcalendar, string& strattach, dynamic_mmap & lbuf)
	{
		if(part->GetSubPartNumber() == 0)
		{
			if(part->GetContentType() != NULL)
			{
				if((strcasecmp(part->GetContentType()->m_type1.c_str(), "text") == 0)
					&&(part->GetContentType()->m_filename == ""))
				{
					if(strcasecmp(part->GetContentType()->m_type2.c_str(), "plain") == 0)
					{
						string strtmp = "";
						for(int x= part->m_data->m_beg; x < part->m_data->m_end; x++)
						{
							strtmp += lbuf[x];
							if(lbuf[x] == '\n')
							{
								if( strcasecmp(part->GetContentTransferEncoding(), "base64") == 0)
								{
									int blen = BASE64_DECODE_OUTPUT_MAX_LEN(strtmp.length());
									char* bbuf = (char*)malloc(blen + 1);
									memset(bbuf, 0, blen + 1);
									CBase64::Decode((char*)strtmp.c_str(), strtmp.length(), bbuf, &blen);

									string strConvert;
									if(code_convert_ex(part->GetContentType()->m_charset.c_str(),CMailBase::m_encoding.c_str(), bbuf, strConvert) == 0)
									{
										strtext += strConvert;
									}
									else
									{
										strtext += bbuf;
									}
									free(bbuf);
								}
								else if(( strcasecmp(part->GetContentTransferEncoding(), "QP") == 0) || ( strcasecmp(part->GetContentTransferEncoding(), "quoted-printable") == 0)) 
								{
									int blen = strtmp.length();
									char* bbuf = (char*)malloc(blen + 1);
									memset(bbuf, 0, blen + 1);
									DecodeQuoted(strtmp.c_str(), strtmp.length()/3*3, (unsigned char*)bbuf);

									string strConvert;
									if(code_convert_ex(part->GetContentType()->m_charset.c_str(),CMailBase::m_encoding.c_str(), bbuf, strConvert) == 0)
									{
										strtext += strConvert;
									}
									else
									{
										strtext += bbuf;
									}
									
									free(bbuf);
								}
								else
								{
									string strConvert;
									if(code_convert_ex(part->GetContentType()->m_charset.c_str(),CMailBase::m_encoding.c_str(), strtmp.c_str(), strConvert) == 0)
									{
										strtext += strConvert;
									}
									else
									{
										strtext += strtmp;
									}
									
								}
								strtmp = "";
							}
						}
					}
					else if(strcasecmp(part->GetContentType()->m_type2.c_str(), "html") == 0)
					{
						string strtmp = "";
						for(int x= part->m_data->m_beg; x < part->m_data->m_end; x++)
						{
							strtmp += lbuf[x];
							if(lbuf[x] == '\n')
							{
								if( strcasecmp(part->GetContentTransferEncoding(), "base64") == 0)
								{
									int blen = BASE64_DECODE_OUTPUT_MAX_LEN(strtmp.length());
									char* bbuf = (char*)malloc(blen + 1);
									memset(bbuf, 0, blen + 1);
									CBase64::Decode((char*)strtmp.c_str(), strtmp.length(), bbuf, &blen);

									string strConvert;
									if(code_convert_ex(part->GetContentType()->m_charset.c_str(),CMailBase::m_encoding.c_str(), bbuf, strConvert) == 0)
									{
										strhtml += strConvert;
									}
									else
									{
										strhtml += bbuf;
									}
									
									free(bbuf);
								}
								else if(( strcasecmp(part->GetContentTransferEncoding(), "QP") == 0) || ( strcasecmp(part->GetContentTransferEncoding(), "quoted-printable") == 0)) 
								{
									int blen = strtmp.length();
									char* bbuf = (char*)malloc(blen + 1);
									memset(bbuf, 0, blen + 1);
									DecodeQuoted(strtmp.c_str(), strtmp.length()/3*3, (unsigned char*)bbuf);

									string strConvert;
									if(code_convert_ex(part->GetContentType()->m_charset.c_str(),CMailBase::m_encoding.c_str(), bbuf, strConvert) == 0)
									{
										strhtml += strConvert;
									}
									else
									{
										strhtml += bbuf;
									}
									
									free(bbuf);
								}
								else
								{
									string strConvert;
									if(code_convert_ex(part->GetContentType()->m_charset.c_str(),CMailBase::m_encoding.c_str(), strtmp.c_str(), strConvert) == 0)
									{
										strhtml += strConvert;
									}
									else
									{
										strhtml += strtmp;
									}
									
								}
								strtmp = "";
							}
						}
					}
					else if(strcasecmp(part->GetContentType()->m_type1.c_str(), "text") == 0 && strcasecmp(part->GetContentType()->m_type2.c_str(), "calendar") == 0)
					{
						string strtmp = "";
						for(int x= part->m_data->m_beg; x < part->m_data->m_end; x++)
						{
							strtmp += lbuf[x];
							if(lbuf[x] == '\n')
							{
								if( strcasecmp(part->GetContentTransferEncoding(), "base64") == 0)
								{
									int blen = BASE64_DECODE_OUTPUT_MAX_LEN(strtmp.length());
									char* bbuf = (char*)malloc(blen + 1);
									memset(bbuf, 0, blen + 1);
									CBase64::Decode((char*)strtmp.c_str(), strtmp.length(), bbuf, &blen);

									string strConvert;
									if(code_convert_ex(part->GetContentType()->m_charset.c_str(),CMailBase::m_encoding.c_str(), bbuf, strConvert) == 0)
									{
										strcalendar+= strConvert;
									}
									else
									{
										strcalendar += bbuf;
									}
									free(bbuf);
								}
								else if(( strcasecmp(part->GetContentTransferEncoding(), "QP") == 0) || ( strcasecmp(part->GetContentTransferEncoding(), "quoted-printable") == 0)) 
								{
									int blen = strtmp.length();
									char* bbuf = (char*)malloc(blen + 1);
									memset(bbuf, 0, blen + 1);
									DecodeQuoted(strtmp.c_str(), strtmp.length()/3*3, (unsigned char*)bbuf);

									string strConvert;
									if(code_convert_ex(part->GetContentType()->m_charset.c_str(),CMailBase::m_encoding.c_str(), bbuf, strConvert) == 0)
									{
										strcalendar += strConvert;
									}
									else
									{
										strcalendar += bbuf;
									}
									
									free(bbuf);
								}
								else
								{
									string strConvert;
									if(code_convert_ex(part->GetContentType()->m_charset.c_str(),CMailBase::m_encoding.c_str(), strtmp.c_str(), strConvert) == 0)
									{
										strcalendar += strConvert;
									}
									else
									{
										strcalendar += strtmp;
									}
									
								}
								strtmp = "";
							}
						}
					}
				}
				else
				{
					if(part->GetContentType())
					{
						strattach += part->GetContentType()->m_filename;
						strattach += "|";
						strattach += part->GetContentType()->m_type1;
						strattach += "*";
					}
					else
					{
						if(part->GetContentDisposition())
						{
							strattach += part->GetContentDisposition()->m_filename;
							strattach += "|";
							strattach += part->GetContentType()->m_type1;
							strattach += "*";
						}
					}
				}
			}
			else
			{
				for(int x= part->m_data->m_beg; x < part->m_data->m_end; x++)
				{
					strtext += lbuf[x];
				}
			}
		}
		else
		{
			for(int a = 0; a < part->GetSubPartNumber(); a++)
			{
				MailInfo(part->GetPart(a), strtext, strhtml, strcalendar, strattach, lbuf);
			}
		}
	}
};


class storage
{
public:
	storage(StorageEngine* stgEngine)
	{	
		m_stgengine_instance = new StorageEngineInstance(stgEngine, &m_mailStg);
		if(!m_stgengine_instance)
		{
			m_mailStg = NULL;
		}
	}
	virtual ~storage()
	{
		if(m_stgengine_instance)
			delete m_stgengine_instance;
	}

protected:
	StorageEngineInstance* m_stgengine_instance;
	MailStorage* m_mailStg;

};

class ApiAddUserToGroup : public doc, public storage
{
public:
	ApiAddUserToGroup(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiAddUserToGroup() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			return;
		}
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
        
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			string strUserlist;
			string strGroupname;
			
			m_session->parse_urlencode_value("USER_LIST", strUserlist);
			m_session->parse_urlencode_value("GROUP_NAME", strGroupname);
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";

			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			vector<string> vUser;
			vSplitString(strUserlist, vUser, ";");
			
			for(int i =0 ; i < vUser.size(); i++)
			{
				m_mailStg->AppendUserToGroup(vUser[i].c_str(), strGroupname.c_str());
			}
			
			strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			strResp +="\r\n";

			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";

			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiDelUserFromGroup : public doc, public storage
{
public:
	ApiDelUserFromGroup(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiDelUserFromGroup() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
		
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			string strUserlist;
			string strGroupname;
			
			m_session->parse_urlencode_value("USER_LIST", strUserlist);
			m_session->parse_urlencode_value("GROUP_NAME", strGroupname);
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			vector<string> vUser;
			vSplitString(strUserlist, vUser, ";");
			
			for(int i =0 ; i < vUser.size(); i++)
			{
				m_mailStg->RemoveUserFromGroup(vUser[i].c_str(), strGroupname.c_str());
			}
			
			strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiCreateUser : public doc, public storage
{
public:
	ApiCreateUser(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiCreateUser() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);

		string username, password;

		
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			string strUserName;
			string strUserPwd;
			string strAlias;
            string strHost;
			string strType;
			string strLevel;
			
			m_session->parse_urlencode_value("NEW_USERNAME", strUserName);
			m_session->parse_urlencode_value("NEW_USERPWD", strUserPwd);
			m_session->parse_urlencode_value("NEW_ALIAS", strAlias);
			m_session->parse_urlencode_value("NEW_TYPE", strType);
			m_session->parse_urlencode_value("NEW_LEVEL", strLevel);
#ifdef _WITH_DIST_
            m_session->parse_urlencode_value("NEW_HOST", strHost);
#else
            strHost = "";
#endif /* _WITH_DIST_ */
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strAlias.c_str(), strAlias);

			//POST DATA is UTF-8 format
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			unsigned int lid;
			if(m_mailStg && m_mailStg->AddID(strUserName.c_str(), strUserPwd.c_str(), strAlias.c_str(), strHost.c_str(), strType == "member" ? utMember : utGroup, urGeneralUser, 5000, atoi(strLevel.c_str())) == 0)
			{
				strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
				
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiUpdateUser : public doc, public storage
{
public:
	ApiUpdateUser(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiUpdateUser() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;

		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			string strUserName;
			string strAlias;
            string strHost;
			string strLevel;
			string strStatus;
			
			m_session->parse_urlencode_value("EDIT_USERNAME", strUserName);
			m_session->parse_urlencode_value("EDIT_ALIAS", strAlias);
			m_session->parse_urlencode_value("EDIT_LEVEL", strLevel);
			m_session->parse_urlencode_value("EDIT_USER_STATUS", strStatus);
#ifdef _WITH_DIST_
            m_session->parse_urlencode_value("EDIT_HOST", strHost);
#else
            strHost = "";
#endif /* _WITH_DIST_ */
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strAlias.c_str(), strAlias);
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			unsigned int lid;
			if(m_mailStg && m_mailStg->UpdateID(strUserName.c_str(), strAlias.c_str(), strHost.c_str(), strStatus == "Active" ? usActive : usDisabled, atoi(strLevel.c_str())) == 0)
			{
				strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
				
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiDeleteUser : public doc, public storage
{
public:
	ApiDeleteUser(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiDeleteUser() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;

		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			string strUserName;
			
			m_session->parse_urlencode_value("USERNAME", strUserName);
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			if(m_mailStg && m_mailStg->DelID(strUserName.c_str()) == 0)
			{
				strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
				
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};


class ApiUpdateLevel : public doc, public storage
{
public:
	ApiUpdateLevel(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiUpdateLevel() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
		
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			string strLevelId;
			string strLevelName;
			string strDescription;
			string strMailmaxsize;
			string strBoxmaxsize;
			string strAudit;
			string strMailsizethreshold;
			string strAttchsizethreshold;
			string strDefault;
			
			m_session->parse_urlencode_value("EDIT_LEVELID", strLevelId);
			m_session->parse_urlencode_value("EDIT_LEVELNAME", strLevelName);
			m_session->parse_urlencode_value("EDIT_DESCRIPTION", strDescription);
			m_session->parse_urlencode_value("EDIT_MAILMAXSIZE", strMailmaxsize);
			m_session->parse_urlencode_value("EDIT_BOXMAXSIZE", strBoxmaxsize);
			m_session->parse_urlencode_value("EDIT_AUDIT", strAudit);
			m_session->parse_urlencode_value("EDIT_MAILSIZETHRESHOLD", strMailsizethreshold);
			m_session->parse_urlencode_value("EDIT_ATTACHSIZETHRESHOLD", strAttchsizethreshold);
			m_session->parse_urlencode_value("EDIT_DEFAULT", strDefault);

			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strDescription.c_str(), strDescription);
			
			
			unsigned int lid = atoi(strLevelId.c_str());
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			if(m_mailStg && m_mailStg->UpdateLevel(lid, strLevelName.c_str(), strDescription.c_str(), atollu(strMailmaxsize.c_str())*1024, atollu(strBoxmaxsize.c_str())*1024, (strAudit == "true" ? eaTrue : eaFalse), atoi(strMailsizethreshold.c_str())*1024, atoi(strAttchsizethreshold.c_str())*1024) == 0)
			{
				if(strDefault == "true")
				{
					if(m_mailStg && m_mailStg->SetDefaultLevel(lid) == 0)
					{
						strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
					}
					else
					{
						strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
					}
				}
				else
					strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiDeleteLevel : public doc, public storage
{
public:
	ApiDeleteLevel(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiDeleteLevel() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
		
		
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			string strLevelId;
			
			m_session->parse_urlencode_value("LEVELID", strLevelId);
			
			unsigned int lid = atoi(strLevelId.c_str());
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			if(m_mailStg && m_mailStg->DelLevel(lid) == 0)
			{
				strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiDefaultLevel : public doc, public storage
{
public:
	ApiDefaultLevel(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiDefaultLevel() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
		
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			string strLevelId;
			
			m_session->parse_urlencode_value("LEVELID", strLevelId);
			
			unsigned int lid = atoi(strLevelId.c_str());
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			if(m_mailStg && m_mailStg->SetDefaultLevel(lid) == 0)
			{
				strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};


class ApiCreateLevel : public doc, public storage
{
public:
	ApiCreateLevel(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiCreateLevel() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
		
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			string strLevelName;
			string strDescription;
			string strMailmaxsize;
			string strBoxmaxsize;
			string strAudit;
			string strMailsizethreshold;
			string strAttchsizethreshold;
			string strDefault;
			
			m_session->parse_urlencode_value("NEW_LEVELNAME", strLevelName);
			m_session->parse_urlencode_value("NEW_DESCRIPTION", strDescription);
			m_session->parse_urlencode_value("NEW_MAILMAXSIZE", strMailmaxsize);
			m_session->parse_urlencode_value("NEW_BOXMAXSIZE", strBoxmaxsize);
			m_session->parse_urlencode_value("NEW_AUDIT", strAudit);
			m_session->parse_urlencode_value("NEW_MAILSIZETHRESHOLD", strMailsizethreshold);
			m_session->parse_urlencode_value("NEW_ATTACHSIZETHRESHOLD", strAttchsizethreshold);
			m_session->parse_urlencode_value("NEW_DEFAULT", strDefault);

			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strDescription.c_str(), strDescription);
			
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			unsigned int lid;
			if(m_mailStg && m_mailStg->AddLevel(strLevelName.c_str(), strDescription.c_str(), atollu(strMailmaxsize.c_str())*1024, atollu(strBoxmaxsize.c_str())*1024, (strAudit == "true" ? eaTrue : eaFalse), atoi(strMailsizethreshold.c_str())*1024, atoi(strAttchsizethreshold.c_str())*1024, lid) == 0)
			{
				if(strDefault == "true")
				{
					if(m_mailStg && m_mailStg->SetDefaultLevel(lid) == 0)
					{
						strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
					}
					else
					{
						strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
					}
				}
				else
					strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiListLevels : public doc, public storage
{
public:
	ApiListLevels(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiListLevels() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
		
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{	
			vector <Level_Info> litbl;
			
			if(m_mailStg && m_mailStg->ListLevel(litbl) == 0)
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail><response errno=\"0\" reason=\"\">";
				
				char szTmp[32];
				
				char* ENABLE_STRING[] = {"false", "true"};
				
				for(int x = 0; x < litbl.size(); x++)
				{
					sprintf(szTmp, "%d", litbl[x].lid);
					strResp +="<level lid=\"";
					strResp += szTmp;
					
					strResp += "\" name=\"";
					strResp += litbl[x].lname;
					
					strResp += "\" description=\"";
					strResp += litbl[x].ldescription;
					
					sprintf(szTmp, "%llu", litbl[x].mailmaxsize);
					strResp += "\" mailmaxsize=\"";
					strResp += szTmp;
					
					sprintf(szTmp, "%llu", litbl[x].boxmaxsize);
					strResp +="\" boxmaxsize=\"";
					strResp += szTmp;
					
					strResp +="\" enableaudit=\"";
					strResp += ENABLE_STRING[litbl[x].enableaudit];
					
					sprintf(szTmp, "%d", litbl[x].mailsizethreshold);
					strResp +="\" mailsizethreshold=\"";
					strResp += szTmp;
					
					sprintf(szTmp, "%d", litbl[x].attachsizethreshold);
					strResp +="\" attachsizethreshold=\"";
					strResp += szTmp;
					
					strResp +="\" default=\"";
					strResp += ENABLE_STRING[litbl[x].ldefault];
					
					sprintf(szTmp, "%d", litbl[x].ltime);
					strResp +="\" ltime=\"";
					strResp += szTmp;
					strResp +="\" />";
				}
				strResp += "</response>"
					"</erisemail>";
			}
			else
			{
				strResp = RSP_500_SYS_ERR;
			}		
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiListCluster : public doc, public storage
{
public:
	ApiListCluster(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiListCluster() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
		
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{	
			vector <Level_Info> litbl;
			
			if(m_mailStg && m_mailStg->ListLevel(litbl) == 0)
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail><response errno=\"0\" reason=\"\">";
				
				strResp +="<cluster host=\"";
                strResp += CMailBase::m_localhostname;
                strResp +="\" desc=\"local host\"/>";
				
#ifdef _WITH_DIST_
                string strline;
                ifstream clustersfilein(CMailBase::m_clusters_list_file.c_str(), ios_base::binary);
                if(clustersfilein.is_open())
                {
                    while(getline(clustersfilein, strline))
                    {
                        strtrim(strline);
                        
                        string strHost;
                        string strDesc;
                        
                        if(strline != "" && strncmp(strline.c_str(), "#", 1) != 0)
                        {
                            strcut(strline.c_str(), NULL, ":", strHost);
                            strcut(strline.c_str(), ":", NULL, strDesc);
                            if(strcasecmp(strHost.c_str(), CMailBase::m_localhostname.c_str()) != 0)
                            {
                                to_safty_xmlstring(strHost);
                                to_safty_xmlstring(strDesc);
                                strResp +="<cluster host=\"";
                                strResp += strHost;
                                strResp +="\" desc=\"";
                                strResp += strDesc;
                                strResp +="\" />";
                            }
                        }
                    }
                }
                clustersfilein.close();
#endif /* _WITH_DIST_ */
				strResp += "</response>"
					"</erisemail>";
			}
			else
			{
				strResp = RSP_500_SYS_ERR;
			}		
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiListUsers : public doc, public storage
{
public:
	ApiListUsers(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiListUsers() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
		
		if(check_adminauth_token(strauth.c_str(), username) == 0 || check_userauth_token(strauth.c_str(), username) == 0)
		{
			char* uType[] = {NULL, "Member", "Group", NULL};
			char* uRole[] = {NULL, "User", "Administrator", NULL};
			char* uStatus[] = {"Active", "Disable", NULL};
			
			string strorderby;
			m_session->parse_urlencode_value("ORDER_BY", strorderby);
			string strdesc;
			m_session->parse_urlencode_value("DESC", strdesc);
			
			vector <User_Info> listtbl;
			if(m_mailStg && m_mailStg->ListID(listtbl, strorderby.c_str(), strdesc == "true" ? TRUE : FALSE) == 0)
			{
				char szTmp[64];
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail><response errno=\"0\" reason=\"\">";
				for(int x = 0; x< listtbl.size(); x++)
				{
					Level_Info linfo;
					m_mailStg->GetLevel(listtbl[x].level, linfo);
					
					strResp +="<user name=\"";
					strResp += listtbl[x].username;
					strResp += "\" domain=\"";
					strResp += CMailBase::m_email_domain;
					strResp += "\" alias=\"";
					strResp += listtbl[x].alias;
                    strResp += "\" host=\"";
					strResp += listtbl[x].host;
					strResp +="\" type=\"";
					strResp += uType[listtbl[x].type];
					strResp +="\" role=\"";
					strResp += uRole[listtbl[x].role];
					
					strResp +="\" status=\"";
					strResp += uStatus[listtbl[x].status];
					
					strResp +="\" lname=\"";
					strResp += linfo.lname;
					
					sprintf(szTmp, "%d", listtbl[x].level);
					strResp +="\" level=\"";
					strResp += szTmp;
					strResp +="\" />";
				}
				strResp += "</response>"
					"</erisemail>";
			}
			else
			{
				strResp = RSP_500_SYS_ERR;
			}		
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiListGroups : public doc, public storage
{
public:
	ApiListGroups(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiListGroups() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
		
		if(check_adminauth_token(strauth.c_str(), username) == 0 || check_userauth_token(strauth.c_str(), username) == 0)
		{
			char* uType[] = {NULL, "Member", "Group", NULL};
			char* uRole[] = {NULL, "User", "Administrator", NULL};
			char* uStatus[] = {"Active", "Disable", NULL};
			
			vector <User_Info> listtbl;
			if(m_mailStg && m_mailStg->ListGroup(listtbl) == 0)
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail><response errno=\"0\" reason=\"\">";
				for(int x = 0; x< listtbl.size(); x++)
				{
					strResp +="<user name=\"";
					strResp += listtbl[x].username;
					strResp += "\" domain=\"";
					strResp += CMailBase::m_email_domain;
					strResp += "\" alias=\"";
					strResp += listtbl[x].alias;
					strResp +="\" type=\"";
					strResp += uType[listtbl[x].type];
					strResp +="\" role=\"";
					strResp += uRole[listtbl[x].role];
					strResp +="\" status=\"";
					strResp += uStatus[listtbl[x].status];
					strResp +="\" />";
				}
				strResp += "</response>"
					"</erisemail>";
			}
			else
			{
				strResp = RSP_500_SYS_ERR;
			}		
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiListMembers : public doc, public storage
{
public:
	ApiListMembers(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiListMembers() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
		
		if(check_adminauth_token(strauth.c_str(), username) == 0 || check_userauth_token(strauth.c_str(), username) == 0)
		{
			char* uType[] = {NULL, "Member", "Group", NULL};
			char* uRole[] = {NULL, "User", "Administrator", NULL};
			char* uStatus[] = {"Active", "Disable", NULL};
			
			vector <User_Info> listtbl;
			if(m_mailStg && m_mailStg->ListMember(listtbl) == 0)
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail><response errno=\"0\" reason=\"\">";
				for(int x = 0; x< listtbl.size(); x++)
				{
					strResp +="<user name=\"";
					strResp += listtbl[x].username;
					strResp += "\" domain=\"";
					strResp += CMailBase::m_email_domain;
					strResp += "\" alias=\"";
					strResp += listtbl[x].alias;
					strResp +="\" type=\"";
					strResp += uType[listtbl[x].type];
					strResp +="\" role=\"";
					strResp += uRole[listtbl[x].role];
					strResp +="\" status=\"";
					strResp += uStatus[listtbl[x].status];
					strResp +="\" />";
				}
				strResp += "</response>"
					"</erisemail>";
			}
			else
			{
				strResp = RSP_500_SYS_ERR;
			}		
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiListMembersofGroup : public doc, public storage
{
public:
	ApiListMembersofGroup(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiListMembersofGroup() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		string strgroup;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		m_session->parse_urlencode_value("GROUP_NAME", strgroup);
		
		string username, password;
		
		if(check_adminauth_token(strauth.c_str(), username) == 0 || check_userauth_token(strauth.c_str(), username) == 0)
		{
			char* uType[] = {NULL, "Member", "Group", NULL};
			char* uRole[] = {NULL, "User", "Administrator", NULL};
			char* uStatus[] = {"Active", "Disable", NULL};
			
			vector <User_Info> listtbl;
			if(m_mailStg && m_mailStg->ListMemberOfGroup(strgroup.c_str(), listtbl) == 0)
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail><response errno=\"0\" reason=\"\">";
				for(int x = 0; x< listtbl.size(); x++)
				{
					strResp +="<user name=\"";
					strResp += listtbl[x].username;
					strResp += "\" domain=\"";
					strResp += CMailBase::m_email_domain;
					strResp += "\" alias=\"";
					strResp += listtbl[x].alias;
					strResp +="\" type=\"";
					strResp += uType[listtbl[x].type];
					strResp +="\" role=\"";
					strResp += uRole[listtbl[x].role];
					strResp +="\" status=\"";
					strResp += uStatus[listtbl[x].status];
					strResp +="\" />";
				}
				strResp += "</response>"
					"</erisemail>";
			}
			else
			{
				strResp = RSP_500_SYS_ERR;
			}		
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};


class ApiListDirs : public doc, public storage
{
public:
	ApiListDirs(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiListDirs() {}
	
	void List(const char* username, int nParentID, const char* globalpath, string& strXML)
	{
		vector<Dir_Info> listtbl;
		if(m_mailStg && m_mailStg->ListSubDir(username, nParentID, listtbl) == 0)
		{
			for(int x = 0; x< listtbl.size(); x++)
			{
				string strName;
				utf7_modified_to_standard_ex(listtbl[x].name, strName);
				code_convert_ex("UTF-7", CMailBase::m_encoding.c_str(), strName.c_str(), strName);
				char szTmp[128];
				to_safty_xmlstring(strName);
				strXML +="<dir name=\"";
				strXML += strName;
				strXML +="\" ";
				
				sprintf(szTmp, "%d", listtbl[x].did);
				strXML +="id=\"";
				strXML += szTmp;
				strXML +="\" ";
				
				string strGlobalPath = globalpath;
				strGlobalPath += "/";
				strGlobalPath += strName;
				
				
				strXML +="globalpath=\"";
				strXML += strGlobalPath;
				strXML +="\" ";
				
				strXML +="owner=\"";
				strXML += listtbl[x].owner;
				strXML +="\" ";
				
				sprintf(szTmp, "%d", listtbl[x].childrennum);
				
				strXML +="childrennum=\"";
				strXML += szTmp;
				strXML +="\">\n";
				
				strXML +="</dir>\n";
			}
		}
	}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strParentID;
		m_session->parse_urlencode_value("PID", strParentID);
		
		string strGlobalPath;
		m_session->parse_urlencode_value("GPATH", strGlobalPath);
		int nPID = -1;
		if(strParentID != "")
		{
			nPID = atoi(strParentID.c_str());
		}
		string username, password;
		
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
            string strXML = "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strXML += "<erisemail><response errno=\"0\" reason=\"\">";

			List(username.c_str(), nPID, strGlobalPath.c_str(), strXML);
			
			strXML += "</response></erisemail>";
            
            strResp += strXML;
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};


class ApiTraversalDirs : public doc, public storage
{
public:
	ApiTraversalDirs(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiTraversalDirs() {}
	
	void Traversal(const char* username, int nParentID, const char* globalpath, string& strXML)
	{
		vector<Dir_Info> listtbl;
		if(m_mailStg && m_mailStg->ListSubDir(username, nParentID, listtbl) == 0)
		{
			for(int x = 0; x< listtbl.size(); x++)
			{
				string strName;
				utf7_modified_to_standard_ex(listtbl[x].name, strName);
				
				code_convert_ex("UTF-7", CMailBase::m_encoding.c_str(), strName.c_str(), strName);
				
				char szTmp[128];
				to_safty_xmlstring(strName);
				
				strXML +="<dir name=\"";
				strXML += strName;
				strXML +="\" ";
				
				sprintf(szTmp, "%d", listtbl[x].did);
				strXML +="id=\"";
				strXML += szTmp;
				strXML +="\" ";
				
				string strGlobalPath = globalpath;
				strGlobalPath += "/";
				strGlobalPath += strName;
				
				
				strXML +="globalpath=\"";
				strXML += strGlobalPath;
				strXML +="\" ";
				
				strXML +="owner=\"";
				strXML += listtbl[x].owner;
				strXML +="\" ";
				
				sprintf(szTmp, "%d", listtbl[x].childrennum);
				
				strXML +="childrennum=\"";
				strXML += szTmp;
				strXML +="\">\n";
				
				Traversal(username, listtbl[x].did, strGlobalPath.c_str(), strXML);
				strXML +="</dir>\n";
			}
		}
	}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strParentID;
		m_session->parse_urlencode_value("PID", strParentID);
		
		string strGlobalPath;
		m_session->parse_urlencode_value("GPATH", strGlobalPath);
		
		int nPID = -1;
		if(strParentID != "")
		{
			nPID = atoi(strParentID.c_str());
		}
		
		string username, password;
		
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail><response errno=\"0\" reason=\"\">";
			
			Traversal(username.c_str(), nPID, strGlobalPath.c_str(),  strResp);
			
			strResp += "</response></erisemail>";
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};


class ApiListMails : public doc, public storage
{
public:
	ApiListMails(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiListMails() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strDirID;
		string strBeg;
		string strRow;
		m_session->parse_urlencode_value("DIRID", strDirID);
		m_session->parse_urlencode_value("BEG", strBeg);
		m_session->parse_urlencode_value("ROW", strRow);
		
		string username, password;

		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			int nDirID;
			int nBeg, nRow;
			if(strDirID == "")
			{
				m_mailStg->GetDirID(username.c_str(), "INBOX", nDirID);
			}
			else
			{
				nDirID = atoi(strDirID.c_str());
			}
			
			if(strBeg == "")
			{
				nBeg = 0;
			}
			else
			{
				nBeg = atoi(strBeg.c_str());
			}
			
			if(strRow == "")
			{
				nRow = 20;
			}
			else
			{
				nRow = atoi(strRow.c_str());
			}
			
			vector<Mail_Info> listtbl;
			if(m_mailStg && m_mailStg->LimitListMailByDir(username.c_str(), listtbl, nDirID, nBeg, nRow) == 0)
			{
				if(nDirID < 0)
					nDirID = 0;
				
				char szTmp[128];
				
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail><response errno=\"0\" reason=\"\">";
				
				for(int x = 0; x < listtbl.size(); x++)
				{
					string emlfile;
					m_mailStg->GetMailIndex(listtbl[x].mid, emlfile);
					
					MailLetter * Letter = NULL;
					Letter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), emlfile.c_str());
					if(Letter && Letter->GetSize() > 0)
					{
						string strSubject;
                        string strSender;
                        string strFromAddr;
						if(Letter->GetSummary()->m_header)
                        {    
                            strSubject = Letter->GetSummary()->m_header->GetDecodedSubject();
                            to_safty_xmlstring(strSubject);
                            strSender = Letter->GetSummary()->m_header->GetFrom() == NULL ? "" : Letter->GetSummary()->m_header->GetFrom()->m_addrs[0].pName;
                            strFromAddr = "";
                            if(Letter->GetSummary()->m_header->GetFrom() != NULL)
                            {
                                strFromAddr = Letter->GetSummary()->m_header->GetFrom()->m_addrs[0].mName;
                                strFromAddr += "@";
                                strFromAddr += Letter->GetSummary()->m_header->GetFrom()->m_addrs[0].dName;
                            }
                            to_safty_xmlstring(strSender);
                        }
						sprintf(szTmp, "%u", Letter->GetSize());
						string strSize = szTmp;
						unsigned long attach_sumsize, attach_count;
						
						Letter->GetAttachSummary(attach_count, attach_sumsize);
						
						sprintf(szTmp, "%u", attach_count);
						string strAttachCount = szTmp;
						sprintf(szTmp, "%u", listtbl[x].mid);
						string strID = szTmp;
						
						BOOL bCalendar = isCalendar(Letter->GetSummary()->m_mime);
						
						string strTime;
						OutSimpleTimeString(listtbl[x].mtime, strTime);

						string strSort;
						sprintf(szTmp, "%u", listtbl[x].mtime);
						strSort = szTmp;
						
						string strSeen;
						string strFlaged;
						string strAnswered;
						string strDraft;
						if((listtbl[x].mstatus & MSG_ATTR_SEEN) == MSG_ATTR_SEEN)
						{
							strSeen = "yes";
						}
						else
						{
							strSeen = "no";
						}
						if((listtbl[x].mstatus & MSG_ATTR_FLAGGED) == MSG_ATTR_FLAGGED)
						{
							strFlaged = "yes";
						}
						else
						{
							strFlaged = "no";
						}
						if((listtbl[x].mstatus & MSG_ATTR_ANSWERED) == MSG_ATTR_ANSWERED)
						{
							strAnswered = "yes";
						}
						else
						{
							strAnswered = "no";
						}
						if((listtbl[x].mstatus & MSG_ATTR_DRAFT) == MSG_ATTR_DRAFT)
						{
							strDraft = "yes";
						}
						else
						{
							strDraft = "no";
						}
						
						strResp +="<mail ";

						strResp +="msgtype=\"";
						strResp += bCalendar ? "calendar" : "mail";
						strResp +="\" ";
							
						strResp +="id=\"";
						strResp += strID;
						strResp +="\" ";
						
						strResp +="uniqid=\"";
						strResp += listtbl[x].uniqid;
						strResp +="\" ";
						
						strResp +="sender=\"";
						strResp += strSender;
						strResp +="\" ";
						
						strResp +="fromaddr=\"";
						strResp += strFromAddr;
						strResp +="\" ";
						
						strResp +="subject=\"";
						strResp += strSubject;
						strResp +="\" ";
						
						strResp +="size=\"";
						strResp += strSize;
						strResp +="\" ";
						
						strResp +="attachcount=\"";
						strResp += strAttachCount;
						strResp +="\" ";
						
						strResp +="time=\"";
						strResp += strTime;
						strResp +="\" ";

						strResp +="sort=\"";
						strResp += strSort;
						strResp +="\" ";
							
						strResp +="seen=\"";
						strResp += strSeen;
						strResp +="\" ";
						
						strResp +="flaged=\"";
						strResp += strFlaged;
						strResp +="\" ";
						
						strResp +="answered=\"";
						strResp += strAnswered;
						strResp +="\" ";
						
						strResp +="draft=\"";
						strResp += strDraft;
						strResp +="\" ";
						
						strResp +="/>";
					}
					if(Letter)
						delete Letter;
				}
				strResp += "</response>"
					"</erisemail>";
			}
			else
			{
				strResp = RSP_500_SYS_ERR;
			}				
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}

		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiListUnauditedMails : public doc, public storage
{
public:
	ApiListUnauditedMails(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiListUnauditedMails() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strBeg;
		string strRow;
		m_session->parse_urlencode_value("BEG", strBeg);
		m_session->parse_urlencode_value("ROW", strRow);
		
		string username, password;

		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			int nBeg, nRow;
			
			if(strBeg == "")
			{
				nBeg = 0;
			}
			else
			{
				nBeg = atoi(strBeg.c_str());
			}
			
			if(strRow == "")
			{
				nRow = 20;
			}
			else
			{
				nRow = atoi(strRow.c_str());
			}
			
			vector<Mail_Info> listtbl;
			if(m_mailStg && m_mailStg->LimitListUnauditedExternMailByDir(username.c_str(), listtbl, nBeg, nRow) == 0)
			{	
				char szTmp[128];
				
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail><response errno=\"0\" reason=\"\">";
				
				for(int x = 0; x < listtbl.size(); x++)
				{
					string emlfile;
					m_mailStg->GetMailIndex(listtbl[x].mid, emlfile);
					
					MailLetter * Letter = NULL;
					Letter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), emlfile.c_str());
					if(Letter && Letter->GetSize() > 0)
					{
                        string strSubject;
                        string strSender;
                        string strFromAddr;
						if(Letter->GetSummary()->m_header)
                        {
                            string strSubject = Letter->GetSummary()->m_header->GetDecodedSubject();
                            to_safty_xmlstring(strSubject);
                            string strSender = Letter->GetSummary()->m_header->GetFrom() == NULL ? "" : Letter->GetSummary()->m_header->GetFrom()->m_addrs[0].pName;
                            strFromAddr = "";
                            if(Letter->GetSummary()->m_header->GetFrom() != NULL)
                            {
                                strFromAddr = Letter->GetSummary()->m_header->GetFrom()->m_addrs[0].mName;
                                strFromAddr += "@";
                                strFromAddr += Letter->GetSummary()->m_header->GetFrom()->m_addrs[0].dName;
                            }
						}
						to_safty_xmlstring(strSender);
						sprintf(szTmp, "%u", Letter->GetSize());
						string strSize = szTmp;
						
						unsigned long attach_sumsize, attach_count;
						Letter->GetAttachSummary(attach_count, attach_sumsize);
						sprintf(szTmp, "%u", attach_count);
						string strAttachCount = szTmp;
						
						sprintf(szTmp, "%u", listtbl[x].mid);
						string strID = szTmp;
						
						string strTime;
						OutSimpleTimeString(listtbl[x].mtime, strTime);
						
						string strSeen;
						string strFlaged;
						string strAnswered;
						string strDraft;
						if((listtbl[x].mstatus & MSG_ATTR_SEEN) == MSG_ATTR_SEEN)
						{
							strSeen = "yes";
						}
						else
						{
							strSeen = "no";
						}
						if((listtbl[x].mstatus & MSG_ATTR_FLAGGED) == MSG_ATTR_FLAGGED)
						{
							strFlaged = "yes";
						}
						else
						{
							strFlaged = "no";
						}
						if((listtbl[x].mstatus & MSG_ATTR_ANSWERED) == MSG_ATTR_ANSWERED)
						{
							strAnswered = "yes";
						}
						else
						{
							strAnswered = "no";
						}
						if((listtbl[x].mstatus & MSG_ATTR_DRAFT) == MSG_ATTR_DRAFT)
						{
							strDraft = "yes";
						}
						else
						{
							strDraft = "no";
						}
						
						strResp +="<mail ";
						
						strResp +="id=\"";
						strResp += strID;
						strResp +="\" ";
						
						strResp +="uniqid=\"";
						strResp += listtbl[x].uniqid;
						strResp +="\" ";
						
						strResp +="sender=\"";
						strResp += strSender;
						strResp +="\" ";
						
						strResp +="fromaddr=\"";
						strResp += strFromAddr;
						strResp +="\" ";
						
						strResp +="subject=\"";
						strResp += strSubject;
						strResp +="\" ";
						
						strResp +="size=\"";
						strResp += strSize;
						strResp +="\" ";
						
						strResp +="attachcount=\"";
						strResp += strAttachCount;
						strResp +="\" ";
						
						strResp +="time=\"";
						strResp += strTime;
						strResp +="\" ";
						
						strResp +="seen=\"";
						strResp += strSeen;
						strResp +="\" ";
						
						strResp +="flaged=\"";
						strResp += strFlaged;
						strResp +="\" ";
						
						strResp +="answered=\"";
						strResp += strAnswered;
						strResp +="\" ";
						
						strResp +="draft=\"";
						strResp += strDraft;
						strResp +="\" ";
						
						strResp +="/>";
					}
					if(Letter)
						delete Letter;
				}
				strResp += "</response>"
					"</erisemail>";
				
			}
			else
			{
				strResp = RSP_500_SYS_ERR;
			}				
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiReadMail : public doc, public storage
{
public:
	ApiReadMail(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiReadMail() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strID;
		m_session->parse_urlencode_value("ID", strID);
		string strEdit;
		m_session->parse_urlencode_value("EDIT", strEdit);
		
		unsigned int nMailID = atoi(strID.c_str());
		string username, password;

		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			BOOL isPass = TRUE;
			if(check_adminauth_token(strauth.c_str(), username) == 0)
			{
				string mailowner;
				if( m_mailStg->GetMailOwner(nMailID, mailowner) == 0)
				{
					if(strcasecmp(mailowner.c_str(), username.c_str()) != 0 )
					{
						isPass = FALSE;
					}
				}
				else
				{
					int maildir = -1;
					unsigned int mailstatus;
					if(m_mailStg && m_mailStg->GetMailDir(nMailID, maildir) != 0 || maildir != -1 || m_mailStg->GetMailStatus(nMailID, mailstatus) == -1 || mailstatus & MSG_ATTR_UNAUDITED != MSG_ATTR_UNAUDITED)
					{
						isPass = FALSE;
					}
				}
			}
			else
			{
				string mailowner;
				if( m_mailStg->GetMailOwner(nMailID, mailowner) == -1 || strcasecmp(mailowner.c_str(), username.c_str()) != 0 )
				{					
					isPass = FALSE;
				}
			}
			
			if(!isPass)
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				
				strResp += "<erisemail>"
					"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
					"</erisemail>";
				
				return;
			}
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			char szTmp[128];
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp +="<erisemail><response errno=\"0\" reason=\"\">";

			string emlfile;
			m_mailStg->GetMailIndex(nMailID, emlfile);
					
			MailLetter * Letter;
			Letter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), emlfile.c_str());
			if(Letter->GetSize() > 0)
			{	
				int llen = 0;
				dynamic_mmap& lbuf = Letter->Body(llen);
				
                if(Letter->GetSummary()->m_header)
                {
                    string strFrom = Letter->GetSummary()->m_header->GetFrom() == NULL ? "" : Letter->GetSummary()->m_header->GetFrom()->m_strfiled;
                    to_safty_xmlstring(strFrom);
                    strResp += "<from>";
                    strResp += strFrom;
                    strResp += "</from>";
                    
                    string strTo = Letter->GetSummary()->m_header->GetTo() == NULL ? "" : Letter->GetSummary()->m_header->GetTo()->m_strfiled;
                    to_safty_xmlstring(strTo);
                    strResp += "<to>";
                    strResp += strTo;
                    strResp += "</to>";
                    
                    string strCC = Letter->GetSummary()->m_header->GetCc() == NULL ? "" : Letter->GetSummary()->m_header->GetCc()->m_strfiled;
                    to_safty_xmlstring(strCC);
                    strResp += "<cc>";
                    strResp += strCC;
                    strResp += "</cc>";
                    
                    string strBCC = Letter->GetSummary()->m_header->GetBcc() == NULL ? "" : Letter->GetSummary()->m_header->GetBcc()->m_strfiled;
                    to_safty_xmlstring(strBCC);
                    strResp += "<bcc>";
                    strResp += strBCC;
                    strResp += "</bcc>";
                    
                    string strDate = Letter->GetSummary()->m_header->GetDate();
                    to_safty_xmlstring(strDate);
                    strResp += "<date>";
                    strResp += strDate;
                    strResp += "</date>";
                    
                    string strSubject = Letter->GetSummary()->m_header->GetDecodedSubject();
                    to_safty_xmlstring(strSubject);
                    strResp += "<subject>";
                    strResp += strSubject;
                    strResp += "</subject>";
				}
				string strTextBody = "";
				string strHTMLBody = "";
				string strCalendar = "";
				string strAttach = "";
				MailInfo(Letter->GetSummary()->m_mime, strTextBody, strHTMLBody, strCalendar, strAttach, lbuf);				
				
				to_safty_xmlstring(strTextBody);
				strResp += "<text_content>";
				strResp += strTextBody;
				strResp += "</text_content>";
				
				to_safty_xmlstring(strHTMLBody);
				strResp += "<html_content>";
				strResp += strHTMLBody;				
				strResp += "</html_content>";

				to_safty_xmlstring(strCalendar);
				strResp += "<calendar_content>";
				strResp += strCalendar;				
				strResp += "</calendar_content>";
				
				to_safty_xmlstring(strAttach);
				strResp += "<attach>";
				strResp += strAttach;
				strResp += "</attach>";
				
				int nID = atoi(strID.c_str());
				unsigned int status;
				m_mailStg->GetMailStatus(nID, status);
				status |= MSG_ATTR_SEEN;
				m_mailStg->SetMailStatus(username.c_str(),nID, status);
			}
			if(Letter)
				delete Letter;
			
			strResp += "</response></erisemail>";
			
			
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiSendMail : public doc, public storage
{
public:
	ApiSendMail(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiSendMail() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		string username, password;
        
		BOOL isOK = TRUE;

		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			vector<string> allAddr;
			vector<string> toAddr;
			
			vector<MailLetter *> vLetter;
			vector<Letter_Info*> vLetterInfo;
			string strToAddrs = "";
			string strCcAddrs = "";
			string strBccAddrs = "";
			string strSubject = "";
			string strContent = "";
			string strAttaches= "";
			
			////////////////////////////////////////////////////////
			//AJAX POST DATA is UTF-8 format
			m_session->parse_urlencode_value("TO_ADDRS", strToAddrs);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strToAddrs.c_str(), strToAddrs);
			
			m_session->parse_urlencode_value("CC_ADDRS", strCcAddrs);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strCcAddrs.c_str(), strCcAddrs);

			m_session->parse_urlencode_value("BCC_ADDRS", strBccAddrs);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strBccAddrs.c_str(), strBccAddrs);

			m_session->parse_urlencode_value("SUBJECT", strSubject);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strSubject.c_str(), strSubject);
			
			m_session->parse_urlencode_value("CONTENT", strContent);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strContent.c_str(), strContent);
			
			m_session->parse_urlencode_value("ATTACHFILES", strAttaches);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strAttaches.c_str(), strAttaches);

			strToAddrs += ";";
			strToAddrs += strCcAddrs;
			strToAddrs += ";";
			strToAddrs += strBccAddrs;
			
			vSplitString(strToAddrs, toAddr, ";, ",TRUE, 0x7FFFFFFFU);
			
			//Extend mail group
			for(int i = 0; i < toAddr.size(); i++)
			{
				string mailaddr = toAddr[i];
				strtrim(mailaddr);
					
				if(!strmatch("*?@?*",mailaddr.c_str()))
				{
					continue;	
				}
				
				if(strmatch("*<*@*>*",mailaddr.c_str()))
				{
					fnfy_strcut(mailaddr.c_str(), "<", " \t\r\n;,", " \t\r\n>;,", mailaddr);
				}
				
				string domain;
				string id;
				strcut(mailaddr.c_str(), "@", NULL, domain);
				strcut(mailaddr.c_str(), NULL, "@", id);
				if(CMailBase::Is_Local_Domain(domain.c_str()))
				{
					strcut(mailaddr.c_str(), NULL, "@", id);
					if(m_mailStg && m_mailStg->VerifyUser(id.c_str()) == 0)
					{
						string strTmp = id;
						strTmp += "@";
						strTmp += domain;
						allAddr.push_back(strTmp);
					}
					else if(m_mailStg && m_mailStg->VerifyGroup(id.c_str()) == 0)
					{
						vector<User_Info> uitbl;
						m_mailStg->ListMemberOfGroup(id.c_str(), uitbl);
						int vlen = uitbl.size();

						for(int x = 0; x < vlen; x++)
						{
							string strTmp = uitbl[x].username;
							strTmp += "@";
							strTmp += domain;
							allAddr.push_back(strTmp);
						}
					}
				}
				else
				{
					string strTmp = id;
					strTmp += "@";
					strTmp += domain;
					allAddr.push_back(strTmp);
				}
			}

			vector<string>::iterator x;
			vector<string>::iterator y;
			for(x = allAddr.begin(); x != allAddr.end(); x++)
			{
				for( y = x + 1; y != allAddr.end();)
				{
					if(strcasecmp((*x).c_str(), (*y).c_str()) == 0)
					{
						y = allAddr.erase(y);
					}
					else
					{
						y++;
					}
				}
			}
			
            string mailfrom = username;
            mailfrom += "@";
            mailfrom += CMailBase::m_email_domain;
                    
			for(int i = 0; i < allAddr.size(); i++)
			{
				string mailaddr = allAddr[i];
				
				if(!strmatch("*?@?*",mailaddr.c_str()))
				{
					continue;
				}
				
				if(strmatch("*<*@*>*",mailaddr.c_str()))
				{
					fnfy_strcut(mailaddr.c_str(), "<", " \t\r\n;,", " \t\r\n>;,", mailaddr);
				}
				
				string domain;
				string id;
				strcut(mailaddr.c_str(), "@", NULL, domain);
				char newuid[1024];
				MailLetter * pLetter = NULL;
				if(CMailBase::Is_Local_Domain(domain.c_str()))
				{
					strcut(mailaddr.c_str(), NULL, "@", id);
					if(m_mailStg && m_mailStg->VerifyUser(id.c_str()) == 0)
					{
						sprintf(newuid, "%08x_%08x_%016lx_%08x_%s", time(NULL), getpid(), pthread_self(), random(), CMailBase::m_localhostname.c_str());
						int DirID;
						m_mailStg->GetInboxID(id.c_str(), DirID);
						unsigned int mstatus = MSG_ATTR_RECENT;
						
						unsigned long long usermaxsize;
						if(m_mailStg && m_mailStg->GetUserSize(id.c_str(), usermaxsize) == -1)
						{
							usermaxsize = 5000*1024;
						}
						
                        string mail_host;
                
                        if(m_mailStg->GetHost(id.c_str(), mail_host) == -1)
                        {
                            mail_host = "";
                        }
                
						pLetter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), newuid, usermaxsize);

						Letter_Info* letter_info = new Letter_Info;
						if(pLetter && letter_info)
						{
                            letter_info->mail_from = mailfrom;
                            
                            if(mail_host == "" || strcasecmp(mail_host.c_str(), CMailBase::m_localhostname.c_str()) == 0)
                            {
                                letter_info->mail_to = "";
                                letter_info->mail_type = mtLocal;
                                letter_info->host = "";
                            }
                            else
                            {
                                letter_info->mail_to = mailaddr.c_str();
                                letter_info->mail_type = mtExtern;
                                letter_info->host = mail_host;
                            }
                            
							letter_info->mail_uniqueid = newuid;
							letter_info->mail_dirid = DirID;
							letter_info->mail_status = mstatus;
							letter_info->mail_time = time(NULL);
							letter_info->user_maxsize = usermaxsize;
							letter_info->mail_id = -1;

							vLetterInfo.push_back(letter_info);
							
							vLetter.push_back(pLetter);
						}
						else
						{
							if(pLetter) 
								delete pLetter;
							if(letter_info)
								delete letter_info;
						}
						
					}
				}
				else
				{
					sprintf(newuid, "%08x_%08x_%016lx_%08x_%s", time(NULL), getpid(), pthread_self(), random(), CMailBase::m_localhostname.c_str());
					int DirID;
					m_mailStg->GetInboxID(id.c_str(), DirID);
					unsigned int mstatus = MSG_ATTR_RECENT;
					
					unsigned long long usermaxsize;
					if(m_mailStg && m_mailStg->GetUserSize(username.c_str(), usermaxsize) == -1)
					{
						usermaxsize = 5000*1024;
					}

							
					pLetter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), newuid, usermaxsize);

					Letter_Info* letter_info = new Letter_Info;
					
					if(pLetter && letter_info)
					{
						letter_info->mail_from = mailfrom;
						letter_info->mail_to = mailaddr;
						letter_info->mail_type = mtExtern;
						letter_info->mail_uniqueid = newuid;
						letter_info->mail_dirid = -1;
						letter_info->mail_status = mstatus;
						letter_info->mail_time = time(NULL);
						letter_info->user_maxsize = usermaxsize;
						letter_info->mail_id = -1;

						vLetterInfo.push_back(letter_info);
						
						vLetter.push_back(pLetter);
					}
					else
					{
						if(pLetter) 
							delete pLetter;
						if(letter_info)
							delete letter_info;
					}
					
				}
			}
			
			//Storage the mail
			for(int v = 0; v < vLetter.size(); v++)
			{
				if(vLetter[v])
				{
					string strFromAddr;
					string strAlias;
					
					User_Info uinfo;
					
					if(m_mailStg && m_mailStg->GetID(username.c_str(), uinfo) == 0)
					{
						strAlias = uinfo.alias;
					}
					
					email eml(CMailBase::m_email_domain.c_str(), username.c_str(), strAlias.c_str());
					
					Replace(strToAddrs, ";", ",");
					eml.set_to_addrs(strToAddrs.c_str());
					
					Replace(strCcAddrs, ";", ",");
					eml.set_cc_addrs(strCcAddrs.c_str());
					
					eml.set_subject(strSubject.c_str());
					eml.set_content_type("html");
					eml.set_content(strContent.c_str());
					
					vector<string> vAtt;
					vSplitString(strAttaches, vAtt, "*", TRUE, 0x7FFFFFFFU);
					
					for(int z = 0; z < vAtt.size(); z++)
					{
						string path = CMailBase::m_private_path.c_str();
						path += "/tmp/";
						path += vAtt[z].c_str();
						eml.add_attach(path.c_str());
					}
					eml.output(m_session->m_clientip.c_str());
					
					char* mbuf = (char*)eml.m_buffer.c_buffer();
					int mlen = eml.m_buffer.length();
					int wlen = 0;
					while(1)
					{
						if(vLetter[v]->Write(mbuf + wlen, (mlen - wlen) > MEMORY_BLOCK_SIZE ? MEMORY_BLOCK_SIZE : (mlen - wlen)) < 0)
						{
							isOK = FALSE;
							break;
						}
						
						wlen += (mlen - wlen) > MEMORY_BLOCK_SIZE ? MEMORY_BLOCK_SIZE : (mlen - wlen);
						if(wlen >= mlen)
							break;
					}
					if(isOK)
					{
						Level_Info linfo;
						linfo.attachsizethreshold = 5000*1024;
						linfo.boxmaxsize = 5000*1024*1024*1024;
						linfo.enableaudit = eaFalse;
						linfo.ldefault = ldFalse;
						linfo.mailmaxsize = 5000*1024;
						linfo.mailsizethreshold = 5000*1024;
						
						if(m_mailStg && m_mailStg->GetUserLevel(username.c_str(), linfo) == -1)
						{
							m_mailStg->GetDefaultLevel(linfo);
							
						}
						if(vLetterInfo[v]->mail_type == mtExtern && linfo.enableaudit == eaTrue)
						{
							vLetter[v]->Flush();
							
							unsigned long attach_sumsize, attach_count;
							vLetter[v]->GetAttachSummary(attach_count, attach_sumsize);
							
							unsigned long mail_sumsize = vLetter[v]->GetSize();							
							if(mail_sumsize >= linfo.mailsizethreshold || attach_sumsize >= linfo.attachsizethreshold)
							{
								vLetterInfo[v]->mail_status |= MSG_ATTR_UNAUDITED;
							}
						}
						
						vLetter[v]->SetOK();
					}
					
					vLetter[v]->Close();
                    
                    if(isOK)
                    {
                        m_mailStg->InsertMailIndex(vLetterInfo[v]->mail_from.c_str(), vLetterInfo[v]->mail_to.c_str(), vLetterInfo[v]->host.c_str(), vLetterInfo[v]->mail_time,
                            vLetterInfo[v]->mail_type, vLetterInfo[v]->mail_uniqueid.c_str(), vLetterInfo[v]->mail_dirid, vLetterInfo[v]->mail_status,
                            vLetter[v]->GetEmlName(), vLetter[v]->GetSize(), vLetterInfo[v]->mail_id);
                        
                        if(vLetterInfo[v]->mail_type == mtExtern)
                        {
                            mta_post_notify();
                        }
                    }
					delete vLetterInfo[v];
					
					delete vLetter[v];
					
					if(isOK == FALSE)
						break;
				}
			}
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			if(isOK)
			{
				strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"Email size is too huge\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiUploadFile : public doc, public storage
{
public:
	ApiUploadFile(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiUploadFile() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;

		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			fbufseg segAttach;
			fbufseg segSelfName;
			fbufseg segUploaded;
			unsigned long long maxsize;
			
			if(m_mailStg && m_mailStg->GetUserSize(username.c_str(), maxsize) == -1)
			{
				maxsize = 5000*1024;
			}
			
			if( (m_session->parse_multipart_value("SELFPAGENAME", segSelfName) == 0) &&
				(m_session->parse_multipart_value("ATTACHFILEBODY", segAttach) == 0) )
			{
				string strSelfName;
				if(segSelfName.m_byte_end > segSelfName.m_byte_beg)
				{
					char* szSelfName = (char*)malloc(segSelfName.m_byte_end - segSelfName.m_byte_beg + 2);
					memset(szSelfName, 0, segSelfName.m_byte_end - segSelfName.m_byte_beg + 2);
					memcpy(szSelfName, m_session->GetFormData()->c_buffer() + segSelfName.m_byte_beg, segSelfName.m_byte_end - segSelfName.m_byte_beg + 1);
					strSelfName = szSelfName;
					free(szSelfName);
				}
				
				string filename, filetype;
				if(m_session->parse_multipart_filename("ATTACHFILEBODY", filename) == -1 || m_session->parse_multipart_type("ATTACHFILEBODY", filetype) ==  -1)
				{
					strResp = RSP_200_OK_NO_CACHE;
					string strHTTPDate;
					OutHTTPDateString(time(NULL), strHTTPDate);
					strResp += "Date: ";
					strResp += strHTTPDate;
					strResp += "\r\n";
					
					strResp += "\r\n";
					strResp += "<html><body><script language=\"JavaScript\">alert(\"";
					strResp += m_session->m_cache->m_language["uploaderror2"];
					strResp +="\");window.location=\"";
					strResp += strSelfName;
					strResp += "\";</script></body></html>";
					
					m_session->HttpSend(strResp.c_str(), strResp.length());
					return;
				}
				
				vector<string> vfilename;
				vSplitString(filename, vfilename, "\\/", TRUE, 0x7FFFFFFFU);
				filename = vfilename[vfilename.size() - 1];
				
				if( segAttach.m_byte_end >= segAttach.m_byte_beg && (segAttach.m_byte_end - segAttach.m_byte_beg) > maxsize )
				{
					strResp = RSP_200_OK_NO_CACHE;
					string strHTTPDate;
					OutHTTPDateString(time(NULL), strHTTPDate);
					strResp += "Date: ";
					strResp += strHTTPDate;
					strResp += "\r\n";
					
					strResp += "\r\n";
					strResp += "<html><body><script language=\"JavaScript\">alert(\"";
					strResp += m_session->m_cache->m_language["uploaderror1"];
					strResp +="\");window.location=\"";
					strResp += strSelfName;
					strResp += "\";</script></body></html>";
					m_session->HttpSend(strResp.c_str(), strResp.length());
					return;
					
				}
				else
				{
					char szFileName[1024];
					sprintf(szFileName, "%s_%08x_%08x_%016lx_%08x.att", username.c_str(), time(NULL), getpid(), pthread_self(), random());
					string strFilePath;
					strFilePath = CMailBase::m_private_path.c_str();
					strFilePath += "/tmp/";
					strFilePath += szFileName;
					ofstream* attachfd =  new ofstream(strFilePath.c_str(), ios_base::binary|ios::out|ios::trunc);
					string strTmp;		
					if(attachfd->is_open())
					{
						
						strTmp = "Content-Type: ";
						strTmp += filetype;
						strTmp += ";\r\n name=\"";
						strTmp += filename;
						strTmp += "\"\r\n";
						strTmp += "Content-Transfer-Encoding: base64\r\n";
						strTmp += "Content-Disposition: inline; filename=\"";
						strTmp += filename;
						strTmp += "\";\r\n\r\n";
						
						attachfd->write(strTmp.c_str(), strTmp.length());	
						strTmp = "";
						
						char szb64tmpbuf1[55];
						char szb64tmpbuf2[73];
						
						int nszb64tmplen2 = 72;
						char* buf = (char*)m_session->GetFormData()->c_buffer() + segAttach.m_byte_beg;
						int len = segAttach.m_byte_end - segAttach.m_byte_beg + 1;
						for(int p = 0; p < ((len % 54) == 0 ? len/54 : (len/54 + 1)) ; p++)
						{
							
							memset(szb64tmpbuf1, 0, 55);
							memset(szb64tmpbuf2, 0, 73);
							
							memcpy(szb64tmpbuf1, buf + p * 54, (len - 54 * p) > 54 ? 54 : len - 54 * p);
							CBase64::Encode(szb64tmpbuf1, (len - 54 * p) > 54 ? 54 : len - 54 * p, szb64tmpbuf2, &nszb64tmplen2);
							
							strTmp = szb64tmpbuf2;
							
							attachfd->write(strTmp.c_str(), strTmp.length());
							
							strTmp = "\r\n";
							attachfd->write(strTmp.c_str(), strTmp.length()); 
							
							strTmp = "";
						}
						attachfd->close();
						
					}
					if(attachfd)
						delete attachfd;
					
					char szSize[64];
					sprintf(szSize, "%u", segAttach.m_byte_end - segAttach.m_byte_beg + 1);
					
					string strSrc1, strSrc2;

					if(code_convert_ex(CMailBase::m_encoding.c_str(), "UTF-8", szFileName, strSrc1) != 0)
					{
						strSrc1 = szFileName;
					}
					if(code_convert_ex(CMailBase::m_encoding.c_str(), "UTF-8", filename.c_str(), strSrc2) != 0)
					{
						strSrc2 = filename;
					}
					
					string strDst1, strDst2;
					encodeURI((const unsigned char*)strSrc1.c_str(), strDst1);
					encodeURI((const unsigned char*)strSrc2.c_str(), strDst2);
					
					string strNewUploaded = strDst1;
					strNewUploaded += "|";
					strNewUploaded += strDst2;
					strNewUploaded += "|";
					strNewUploaded += szSize;
					
					strResp = RSP_301_MOVED_NO_CACHE;
					
					string strHTTPDate;
					OutHTTPDateString(time(NULL), strHTTPDate);
					strResp += "Date: ";
					strResp += strHTTPDate;
					strResp += "\r\n";
					
					strResp +="Location: ";
					strResp += strSelfName;
					strResp += "?UPLOADEDFILES=";
					strResp += strNewUploaded;
					strResp += "\r\n\r\n";
				}
			}
			else
			{
				strResp = RSP_200_OK_NO_CACHE;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp += "\r\n";
				strResp += "<html><body><script language=\"JavaScript\">alert(\"";
				strResp += m_session->m_cache->m_language["uploaderror2"];
				strResp +="\");history.go(-1);";
				strResp += "</script></body></html>";
				m_session->HttpSend(strResp.c_str(), strResp.length());
				return;
			}
		}
		else
		{
			strResp = RSP_404_NOT_FOUND;
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiDeleteUploadedFile : public doc, public storage
{
public:
	ApiDeleteUploadedFile(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiDeleteUploadedFile() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;

		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			string strUploadedFile;
			m_session->parse_urlencode_value("UPLOADEDFILE", strUploadedFile);
			string strFilePath;
			strFilePath = CMailBase::m_private_path;
			strFilePath += "/tmp/";
			strFilePath += strUploadedFile;
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			if(unlink(strFilePath.c_str()) == 0)
			{
				strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"Delete file error\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_404_NOT_FOUND;
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiCopyMail : public doc, public storage
{
public:
	ApiCopyMail(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiCopyMail() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strMailID, strFromDirID, strToDirID;
		m_session->parse_urlencode_value("MAILID", strMailID);
		m_session->parse_urlencode_value("TODIRS", strToDirID);
		
		string username, password;

		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			vector<string> vDirID;
			vSplitString(strToDirID, vDirID, "*", TRUE, 0x7FFFFFFFU);
			
			unsigned int nMailID = atoi(strMailID.c_str());
			
			for(int i = 0; i < vDirID.size(); i++)
			{
				unsigned int nToDirID = atoi(vDirID[i].c_str());
				
				string mail_owner;
				m_mailStg->GetMailOwner(nMailID, mail_owner);
				
				string todir_owner;
				m_mailStg->GetDirOwner(nToDirID, todir_owner);
				
				if((strcasecmp(mail_owner.c_str(), username.c_str()) == 0)&&
					(strcasecmp(todir_owner.c_str(), username.c_str()) == 0))
				{
					
					char newuid[1024];
					sprintf(newuid, "%08x_%08x_%016lx_%08x_%s", time(NULL), getpid(), pthread_self(), random(), CMailBase::m_localhostname.c_str());
					
					unsigned long long usermaxsize;
					if(m_mailStg && m_mailStg->GetUserSize(username.c_str(), usermaxsize) == -1)
					{
						usermaxsize = 5000*1024;
					}
					
					unsigned int mstatus = 0;
					
					m_mailStg->GetMailStatus(nMailID, mstatus);
					
					MailLetter* oldLetter, *newLetter;

					string emlfile;
					m_mailStg->GetMailIndex(nMailID, emlfile);
			
					oldLetter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), emlfile.c_str());
					newLetter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), newuid, usermaxsize /*m_mailStg, "", "", mtLocal,
						newuid, nToDirID, mstatus, (unsigned int)time(NULL), usermaxsize*/);

					Letter_Info letter_info;
					letter_info.mail_from = "";
					letter_info.mail_to = "";
                    letter_info.host = "";
					letter_info.mail_type = mtLocal;
					letter_info.mail_uniqueid = newuid;
					letter_info.mail_dirid = nToDirID;
					letter_info.mail_status = mstatus;
					letter_info.mail_time = time(NULL);
					letter_info.user_maxsize = usermaxsize;
					letter_info.mail_id = -1;

					BOOL isOK = TRUE;
										
					if(oldLetter->GetSize() > 0)
					{
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
					}
					if(isOK)
						newLetter->SetOK();
                    
					newLetter->Close();
                    if(isOK)
                    {
                        m_mailStg->InsertMailIndex(letter_info.mail_from.c_str(), letter_info.mail_to.c_str(), letter_info.host.c_str(), letter_info.mail_time,
                            letter_info.mail_type, letter_info.mail_uniqueid.c_str(), letter_info.mail_dirid, letter_info.mail_status,
                            newLetter->GetEmlName(), newLetter->GetSize(), letter_info.mail_id);
					}
					delete oldLetter;
					delete newLetter;
				}
			}
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
			
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiMoveMail : public doc, public storage
{
public:
	ApiMoveMail(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiMoveMail() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strMailID, strFromDirID, strToDirID;
		m_session->parse_urlencode_value("MAILID", strMailID);
		m_session->parse_urlencode_value("TODIRS", strToDirID);
		
		string username, password;

		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			vector<string> vDirID;
			vSplitString(strToDirID, vDirID, "*", TRUE, 0x7FFFFFFFU);
			
			unsigned int nMailID = atoi(strMailID.c_str());
			
			for(int i=0 ;i < vDirID.size(); i++)
			{
				unsigned int nToDirID = atoi(vDirID[i].c_str());
				
				string mail_owner;
				m_mailStg->GetMailOwner(nMailID, mail_owner);
				
				string todir_owner;
				m_mailStg->GetDirOwner(nToDirID, todir_owner);
				
				if((strcasecmp(mail_owner.c_str(), username.c_str()) == 0)&&
					(strcasecmp(todir_owner.c_str(), username.c_str()) == 0))
				{
					
					char newuid[1024];
					sprintf(newuid, "%08x_%08x_%016lx_%08x_%s", time(NULL), getpid(), pthread_self(), random(), CMailBase::m_localhostname.c_str());
					
					unsigned long long usermaxsize;
					if(m_mailStg && m_mailStg->GetUserSize(username.c_str(), usermaxsize) == -1)
					{
						usermaxsize = 5000*1024;
					}
					
					unsigned int mstatus = 0;
					
					m_mailStg->GetMailStatus(nMailID, mstatus);
					
					MailLetter* oldLetter, *newLetter;

					string emlfile;
					m_mailStg->GetMailIndex(nMailID, emlfile);
			
					oldLetter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), emlfile.c_str());
					
					newLetter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), newuid, usermaxsize);

					Letter_Info letter_info;
					letter_info.mail_from = "";
					letter_info.mail_to = "";
                    letter_info.host = "";
					letter_info.mail_type = mtLocal;
					letter_info.mail_uniqueid = newuid;
					letter_info.mail_dirid = nToDirID;
					letter_info.mail_status = mstatus;
					letter_info.mail_time = time(NULL);
					letter_info.user_maxsize = usermaxsize;
					letter_info.mail_id = -1;
					
					int llen;
					BOOL isOK = TRUE;
					if(oldLetter->GetSize() > 0)
					{
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
					}
					if(isOK)
						newLetter->SetOK();	
                    
					newLetter->Close();
					
                    if(newLetter->isOK())
					{
						m_mailStg->InsertMailIndex(letter_info.mail_from.c_str(), letter_info.mail_to.c_str(), letter_info.host.c_str(), letter_info.mail_time,
							letter_info.mail_type, letter_info.mail_uniqueid.c_str(), letter_info.mail_dirid, letter_info.mail_status,
							newLetter->GetEmlName(), newLetter->GetSize(), letter_info.mail_id);
					}
					
					delete oldLetter;
					delete newLetter;
				}
			}
			
			//delete from the system actually
			m_mailStg->ShiftDelMail(nMailID);
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiTrashMail : public doc, public storage
{
public:
	ApiTrashMail(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiTrashMail() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strMailID, strFromDirID, strToDirID;
		m_session->parse_urlencode_value("MAILID", strMailID);
		
		unsigned int nMailID = atoi(strMailID.c_str());
		
		
		string username, password;

		if(check_userauth_token(strauth.c_str(), username) == 0)
		{	
			int nToDirID = -1;
			m_mailStg->GetTrashID(username.c_str(), nToDirID);
			string mail_owner;
			m_mailStg->GetMailOwner(nMailID, mail_owner);
			
			if((strcasecmp(mail_owner.c_str(), username.c_str()) == 0))
			{
				
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				//move
				if(m_mailStg && m_mailStg->ChangeMailDir(nToDirID, nMailID) == 0)
				{
					strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
				}
				else
				{
					strResp += "<erisemail><response errno=\"1\" reason=\"Email size is too huge\"></response></erisemail>";
				}
				
				
			}
			else
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				
				strResp += "<erisemail><response errno=\"1\" reason=\"Copy Mail Failed\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};


class ApiDeleteMail : public doc, public storage
{
public:
	ApiDeleteMail(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiDeleteMail() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strMailID;
		m_session->parse_urlencode_value("MAILID", strMailID);
		unsigned int nMailID = atoi(strMailID.c_str());
		string username, password;
		
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			char szTmp[128];
			if(m_mailStg && m_mailStg->DelMail(username.c_str(), nMailID) == 0)
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				
				strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
				
			}
			else
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				
				strResp += "<erisemail><response errno=\"1\" reason=\"Delete Mail Failed.\"></response></erisemail>";
				
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiPassMail : public doc, public storage
{
public:
	ApiPassMail(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiPassMail() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strMailID;
		m_session->parse_urlencode_value("MAILID", strMailID);
		unsigned int nMailID = atoi(strMailID.c_str());
		string username, password;
		
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			char szTmp[128];
			unsigned int mstatus;
			if(m_mailStg && m_mailStg->GetMailStatus(nMailID, mstatus) == 0 && m_mailStg->SetMailStatus(username.c_str(), nMailID, mstatus&(~MSG_ATTR_UNAUDITED)) == 0)
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				
				strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
				
			}
			else
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				
				strResp += "<erisemail><response errno=\"1\" reason=\"Delete Mail Failed.\"></response></erisemail>";
				
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiRejectMail : public doc, public storage
{
public:
	ApiRejectMail(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiRejectMail() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strMailID;
		m_session->parse_urlencode_value("MAILID", strMailID);
		unsigned int nMailID = atoi(strMailID.c_str());
		string username, password;
        
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			string strFrom, strTo;
			if(m_mailStg && m_mailStg->GetMailFromAndTo(nMailID, strFrom, strTo) == 0)
			{
				if(strFrom != "")
				{
					BOOL isLocal = FALSE;
					int nToDirID = -1;
                    string strHost;
					string strDomain;
					string strUserID;
					strcut(strFrom.c_str(), "@", NULL, strDomain);
					strcut(strFrom.c_str(), NULL, "@", strUserID);
					if(CMailBase::Is_Local_Domain(strDomain.c_str()))
					{
                        string mail_host;
                
                        if(m_mailStg->GetHost(strUserID.c_str(), mail_host) == -1)
                        {
                            mail_host = "";
                        }
                        
                        if(mail_host == "" || strcasecmp(mail_host.c_str(), CMailBase::m_localhostname.c_str()) == 0)
                        {
                           strHost = "";
                           isLocal = TRUE;
                           m_mailStg->GetInboxID(strUserID.c_str(), nToDirID);
                        }
                        else
                        {
                            strHost = mail_host;
                            isLocal = FALSE;
                            nToDirID = -1;
                        }						
					}
					else
					{
						isLocal = FALSE;
						nToDirID = -1;
					}
					
					char szTmp[128];
					unsigned int mstatus;
					
					char newuid[1024];
					sprintf(newuid, "%08x_%08x_%016lx_%08x_%s", time(NULL), getpid(), pthread_self(), random(), CMailBase::m_localhostname.c_str());
					
					unsigned long long usermaxsize;
					if(m_mailStg && m_mailStg->GetUserSize(username.c_str(), usermaxsize) == -1)
					{
						usermaxsize = 5000*1024;
					}
					
					m_mailStg->GetMailStatus(nMailID, mstatus);
					
					string postmaster = "postermaster@";
					postmaster += CMailBase::m_email_domain;
					
					MailLetter* oldLetter, *newLetter;
					string emlfile;
					m_mailStg->GetMailIndex(nMailID, emlfile);
					
					oldLetter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), emlfile.c_str());
					newLetter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), newuid, usermaxsize);

					Letter_Info letter_info;
					letter_info.mail_from = postmaster.c_str();
					letter_info.mail_to = strFrom.c_str();
                    letter_info.host = strHost.c_str();
					letter_info.mail_type = isLocal ? mtLocal : mtExtern;
					letter_info.mail_uniqueid = newuid;
					letter_info.mail_dirid = nToDirID;
					letter_info.mail_status = mstatus & (~MSG_ATTR_UNAUDITED);
					letter_info.mail_time = time(NULL);
					letter_info.user_maxsize = usermaxsize;
					letter_info.mail_id = -1;
					
					string strLine;
					BOOL findSubject = FALSE;
					while(oldLetter->Line(strLine) == 0)
					{
						strtrim(strLine, "\r\n");
						if( !findSubject && strncasecmp(strLine.c_str(), "subject:", sizeof("subject:") - 1) == 0)
						{
							string strSubject;
							strcut(strLine.c_str(), ":", NULL, strSubject);
							strtrim(strSubject);
							
							string strDecodedSubject;
							DecodeMIMEString(strSubject.c_str(), strDecodedSubject);
							
							strSubject = "[";
							strSubject += m_session->m_cache->m_language["reject"];
							strSubject += "] - ";
							strSubject += strDecodedSubject;
							
							int nTmplen = BASE64_ENCODE_OUTPUT_MAX_LEN(strSubject.length()) + 1;
							char * szTmpbuf = (char*)malloc(nTmplen);
							CBase64::Encode((char*)strSubject.c_str(), strSubject.length(), szTmpbuf, &nTmplen);
							
							string strEncodedMsg;
							strEncodedMsg = "=?";
							strEncodedMsg += CMailBase::m_encoding;
							strEncodedMsg += "?B?";
							strEncodedMsg += szTmpbuf;
							strEncodedMsg += "?=";
							
							free(szTmpbuf);
							
							strLine = "Subject: ";
							strLine += strEncodedMsg;
							findSubject = TRUE;
						}
						strLine += "\r\n";
						newLetter->Write(strLine.c_str(), strLine.length());
					}
					
					newLetter->SetOK();	
					newLetter->Close();
					
					m_mailStg->InsertMailIndex(letter_info.mail_from.c_str(), letter_info.mail_to.c_str(), letter_info.host.c_str(), letter_info.mail_time,
						letter_info.mail_type, letter_info.mail_uniqueid.c_str(), letter_info.mail_dirid, letter_info.mail_status,
						newLetter->GetEmlName(), newLetter->GetSize(), letter_info.mail_id);
					
					delete oldLetter;
					delete newLetter;
					
					m_mailStg->DelMail(username.c_str(), nMailID);
					
					strResp = RSP_200_OK_XML;
					string strHTTPDate;
					OutHTTPDateString(time(NULL), strHTTPDate);
					strResp += "Date: ";
					strResp += strHTTPDate;
					strResp += "\r\n";
					
					strResp +="\r\n";
					strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
					
					strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
					
				}
				else
				{
					m_mailStg->DelMail(username.c_str(), nMailID);
				}
			}
			else
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				
				strResp += "<erisemail><response errno=\"1\" reason=\"Delete Mail Failed.\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiFlagMail : public doc, public storage
{
public:
	ApiFlagMail(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiFlagMail() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strID;
		string strFlag;
		m_session->parse_urlencode_value("ID", strID);
		m_session->parse_urlencode_value("FLAG", strFlag);
		unsigned int nMailID = atoi(strID.c_str());
		string username, password;
		
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			unsigned int status;
			if(m_mailStg && m_mailStg->GetMailStatus(nMailID, status) == 0)
			{
				if(strFlag == "yes")
				{
					status |= MSG_ATTR_FLAGGED;
				}
				else if(strFlag == "no")
				{
					status &= (~MSG_ATTR_FLAGGED);
				}
				m_mailStg->SetMailStatus(username.c_str(), nMailID, status);
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail>"
					"<response errno=\"0\" reason=\"\"></response></erisemail>";
			}
			else
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail>"
					"<response errno=\"1\" reason=\"Flag Mail Failed\"></response>"
					"</erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiSeenMail : public doc, public storage
{
public:
	ApiSeenMail(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiSeenMail() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strID;
		string strSeen;
		m_session->parse_urlencode_value("ID", strID);
		m_session->parse_urlencode_value("SEEN", strSeen);
		unsigned int nMailID = atoi(strID.c_str());
		string username, password;
        
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			unsigned int status;
			if(m_mailStg && m_mailStg->GetMailStatus(nMailID, status) == 0)
			{
				if(strSeen == "yes")
				{
					status |= MSG_ATTR_SEEN;
				}
				else if(strSeen == "no")
				{
					status &= (~MSG_ATTR_SEEN);
				}
				m_mailStg->SetMailStatus(username.c_str(), nMailID, status);
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail>"
					"<response errno=\"0\" reason=\"\"></response></erisemail>";
			}
			else
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail>"
					"<response errno=\"1\" reason=\"Flag Mail Failed\"></response>"
					"</erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiGetDirMailNum : public doc, public storage
{
public:
	ApiGetDirMailNum(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiGetDirMailNum() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strDirID;
		m_session->parse_urlencode_value("DIRID", strDirID);
		
		string username, password;
        
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			int nDirID;
			if(strDirID == "")
			{
				m_mailStg->GetDirID(username.c_str(), "INBOX", nDirID);
			}
			else
			{
				nDirID = atoi(strDirID.c_str());
			}
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail><response errno=\"0\" reason=\"\">";
			
			strResp +="<count>";
			
			char szTmp[64];
			unsigned int count = 0;
			m_mailStg->GetDirMailCount(username.c_str(), nDirID, count);
			sprintf(szTmp, "%u", count);
			strResp += szTmp;
			strResp += "</count></response></erisemail>";
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiGetUnauditedMailNum : public doc, public storage
{
public:
	ApiGetUnauditedMailNum(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiGetUnauditedMailNum() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
        
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail><response errno=\"0\" reason=\"\">";
			
			strResp +="<count>";
			
			char szTmp[64];
			unsigned int count = 0;
			m_mailStg->GetUnauditedExternMailCount(username.c_str(), count);
			sprintf(szTmp, "%u", count);
			strResp += szTmp;
			strResp += "</count></response></erisemail>";
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiAttachment : public doc, public storage
{
public:
	ApiAttachment(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiAttachment() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strID;
		string strFileName;
		string strContentID;
		m_session->parse_urlencode_value("ID", strID);
		m_session->parse_urlencode_value("FILENAME", strFileName);
		m_session->parse_urlencode_value("CONTENTID", strContentID);
		
		string strSrc = strFileName;
		code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strSrc.c_str(), strFileName);		
		unsigned int nMailID = atoi(strID.c_str());
		string username, password;
        
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			char szTmp[128];
			
			MailLetter * Letter;
			
			string emlfile;
			m_mailStg->GetMailIndex(nMailID, emlfile);
					
			Letter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), emlfile.c_str());
			if(Letter->GetSize() > 0)
			{	
				int llen = 0;
				dynamic_mmap& lbuf = Letter->Body(llen);
				
				strResp = RSP_200_OK_NO_CACHE;
				
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				
				string strType = "";
				if(strFileName != "")
				{					
					fbuffer fbuf;
					string dispos = "attachment";
					
					MailFileData(Letter->GetSummary()->m_mime, strFileName.c_str(), strType, dispos, fbuf, lbuf);					
					
					if((strType == "/") || (strType == ""))
					{
						strResp += "Content-Type: */*\r\n";
					}
					else
					{
						strResp += "Content-Type: ";
						strResp += strType;
						strResp += ";\r\n name=\"";
						strResp += strFileName;
						strResp += "\"";
						strResp += "\r\n";
					}
					
					strResp += "Content-Disposition: ";
					strResp += dispos;
					strResp += "; filename=\"";
					strResp += strFileName;
					strResp += "\";\r\n";
					
					char szLen[64];
					sprintf(szLen, "%u", fbuf.length());
					strResp += "Content-Length: ";
					strResp += szLen;
					strResp += "\r\n";
					
					strResp += "\r\n";
					
					m_session->HttpSend(strResp.c_str(), strResp.length());
					
					const char* pbuf = fbuf.c_buffer();
					unsigned int lbuf = fbuf.length();
					
					unsigned int nsend = 0;
					while(1)
					{
						if(nsend >= lbuf)
						{
							break;
						}
						else
						{
							m_session->HttpSend(pbuf + nsend, (lbuf - nsend) > 1448 ? 1448 : (lbuf - nsend));
							nsend += (lbuf - nsend) > 1448 ? 1448 : (lbuf - nsend);
						}
					}
					
				}
				else if(strContentID != "")
				{
					fbuffer fbuf;
					string dispos = "attachment";
					MailFileDataEx(Letter->GetSummary()->m_mime, strContentID.c_str(), strType, dispos, fbuf, lbuf);
					
					if((strType == "/") || (strType == ""))
					{
						strResp += "Content-Type: */*\r\n";
					}
					else
					{
						strResp += "Content-Type: ";
						strResp += strType;
						strResp += ";\r\n name=\"";
						strResp += strFileName;
						strResp += "\"";
						strResp += "\r\n";
					}
					
					strResp += "Content-Disposition: ";
					strResp += dispos;
					strResp += "; filename=\"";
					strResp += strFileName;
					strResp += "\";\r\n";
					
					char szLen[64];
					sprintf(szLen, "%u", fbuf.length());
					strResp += "Content-Length: ";
					strResp += szLen;
					strResp += "\r\n";
					
					strResp += "\r\n";
					
					m_session->HttpSend(strResp.c_str(), strResp.length());
					
					const char* pbuf = fbuf.c_buffer();
					unsigned int lbuf = fbuf.length();
					
					unsigned int nsend = 0;
					while(1)
					{
						if(nsend >= lbuf)
						{
							break;
						}
						else
						{
							m_session->HttpSend(pbuf + nsend, (lbuf - nsend) > 1448 ? 1448 : (lbuf - nsend));
							nsend += (lbuf - nsend) > 1448 ? 1448 : (lbuf - nsend);
						}
					}
					
				}
				else
				{
					strResp = RSP_404_NOT_FOUND;
					m_session->HttpSend(strResp.c_str(), strResp.length());
				}
			}
			if(Letter)
				delete Letter;	
		}
		else
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
		}
	}
	
};

class ApiTempFile : public doc, public storage
{
public:
	ApiTempFile(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiTempFile() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strTmpFileName;
		m_session->parse_urlencode_value("TMPFILENAME", strTmpFileName);
		
		string username, password;
        
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			if(strstr(strTmpFileName.c_str(), "..") == NULL)
			{
				string strRealFilePath = CMailBase::m_private_path;
				strRealFilePath += "/tmp/";
				strRealFilePath += strTmpFileName;
				
				ifstream tmpfd(strRealFilePath.c_str(), ios_base::binary);
				
				if(tmpfd.is_open())
				{					
					string strLine = "";
					string strCell = "";
					string strFileName1 = "";
					string strFileName2 = "";
					string strContentType = "application/*";
					string strTransferEncoding = "";
					string strDisposition = "attachment";
					
					//read header
					while(getline(tmpfd, strLine))
					{
						strLine += "\n";
						
						if(strLine == "\r\n" || strLine == "\n")
						{
							if(strncasecmp(strCell.c_str(), "Content-Type:", sizeof("Content-Type:") - 1) == 0)
							{
								fnfy_strcut(strCell.c_str(), "Content-Type:", " \t\r\n\"", "\r\n\";", strContentType);
								fnfy_strcut(strCell.c_str(), "name=", " \t\r\n\"", "\r\n\";", strFileName1);
							}
							else if(strncasecmp(strCell.c_str(), "Content-Disposition:", sizeof("Content-Disposition:") - 1) == 0)
							{
								
								fnfy_strcut(strCell.c_str(), "Content-Disposition:", " \t\"\r\n", "\";\r\n", strDisposition);
								fnfy_strcut(strCell.c_str(), "filename=", " \t\"\r\n", "\";\r\n", strFileName2);
							}
							else if(strncasecmp(strCell.c_str(), "Content-Transfer-Encoding:", sizeof("Content-Transfer-Encoding:") - 1) == 0)
							{
								fnfy_strcut(strCell.c_str(), "Content-Transfer-Encoding:", " \t\"\r\n", " \t\";\r\n", strTransferEncoding);
							}
							
							break;
						}
						else
						{
							if( strncasecmp(strLine.c_str(), " ", 1) == 0 || strncasecmp(strLine.c_str(), "\t", 1) == 0)
							{
								strCell += strLine;
							}
							else
							{
								if(strCell != "")
								{
									if(strncasecmp(strCell.c_str(), "Content-Type:", sizeof("Content-Type:") - 1) == 0)
									{
										fnfy_strcut(strCell.c_str(), "Content-Type:", " \t\r\n\"", "\r\n\";", strContentType);
										fnfy_strcut(strCell.c_str(), "name=", " \t\r\n\"", "\r\n\";", strFileName1);
									}
									else if(strncasecmp(strCell.c_str(), "Content-Disposition:", sizeof("Content-Disposition:") - 1) == 0)
									{
										
										fnfy_strcut(strCell.c_str(), "Content-Disposition:", " \t\"\r\n", "\";\r\n", strDisposition);
										fnfy_strcut(strCell.c_str(), "filename=", " \t\"\r\n", "\";\r\n", strFileName2);
									}
									else if(strncasecmp(strCell.c_str(), "Content-Transfer-Encoding:", sizeof("Content-Transfer-Encoding:") - 1) == 0)
									{
										fnfy_strcut(strCell.c_str(), "Content-Transfer-Encoding:", " \t\"\r\n", " \t\";\r\n", strTransferEncoding);
									}
								}
								
								strCell = strLine;
							}
						}
					}
					
					fbuffer fbuf;
					
					while(getline(tmpfd, strLine))
					{
						strLine += "\n";
						if(strcasecmp(strTransferEncoding.c_str(), "base64") == 0)
						{
							strtrim(strLine);
							if(strLine != "")
							{
								int clen = BASE64_DECODE_OUTPUT_MAX_LEN(strLine.length());
								char* cbuf = new char[clen + 1];
								CBase64::Decode((char*)strLine.c_str(), strLine.length(), cbuf, &clen);
								fbuf.bufcat(cbuf, clen);
								delete[] cbuf;
							}
						}
						else if((strcasecmp(strTransferEncoding.c_str(), "QP") == 0) || (strcasecmp(strTransferEncoding.c_str(), "quoted-printable") == 0))
						{
							strtrim(strLine);
							if(strLine != "")
							{
								int clen = strLine.length();
								char* cbuf = new char[clen + 1];
								DecodeQuoted((char*)strLine.c_str(), strLine.length()/3*3, (unsigned char*)cbuf);								
								fbuf.bufcat(cbuf, clen/3);
								
								delete[] cbuf;
							}
						}
						else
						{
							fbuf.bufcat(strLine.c_str(), strLine.length());
						}
					}
					
					//end read
					tmpfd.close();
					
					char szBackup[256];
					sprintf(szBackup, "Attachment_%08x", random());
					
					//read and send file data
					strResp = RSP_200_OK_NO_CACHE;
					string strHTTPDate;
					OutHTTPDateString(time(NULL), strHTTPDate);
					strResp += "Date: ";
					strResp += strHTTPDate;
					strResp += "\r\n";
					
					strResp += "Content-Type: ";
					strResp += strContentType;
					strResp += ";\r\n name=\"";
					strResp += strFileName1 == "" ? (strFileName2 == "" ? szBackup : strFileName2) : strFileName1;
					strResp += "\"\r\n";
					
					strResp += "Content-Disposition: ";
					strResp += strDisposition;
					strResp += "; filename=\"";
					strResp += strFileName1 == "" ? (strFileName2 == "" ? szBackup : strFileName2) : strFileName1;
					strResp += "\";\r\n";
					
					char szLen[64];
					sprintf(szLen, "%u", fbuf.length());
					strResp += "Content-Length: ";
					strResp += szLen;
					strResp += "\r\n";
					
					strResp += "\r\n";
					
					m_session->HttpSend(strResp.c_str(), strResp.length());
					
					const char* pbuf = fbuf.c_buffer();
					unsigned int lbuf = fbuf.length();
					
					unsigned int nsend = 0;
					while(1)
					{
						if(nsend >= lbuf)
						{
							break;
						}
						else
						{
							m_session->HttpSend(pbuf + nsend, (lbuf - nsend) > 1448 ? 1448 : (lbuf - nsend));
							nsend += (lbuf - nsend) > 1448 ? 1448 : (lbuf - nsend);
						}
					}
					
				}
				else
				{
					strResp = RSP_404_NOT_FOUND;
					m_session->HttpSend(strResp.c_str(), strResp.length());
				}
			}
			else
			{
				strResp = RSP_404_NOT_FOUND;
				m_session->HttpSend(strResp.c_str(), strResp.length());
			}
		}
		else
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
		}
	}
	
};

class ApiExtractAttach : public doc, public storage
{
public:
	ApiExtractAttach(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiExtractAttach() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strID;
		m_session->parse_urlencode_value("ID", strID);
		
		unsigned int nMailID = atoi(strID.c_str());
		string username, password;
        
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			char szTmp[128];
			
			MailLetter * Letter;

			string emlfile;
			m_mailStg->GetMailIndex(nMailID, emlfile);
			
			Letter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), emlfile.c_str());
			if(Letter->GetSize() > 0)
			{
				int llen = 0;
				dynamic_mmap& lbuf = Letter->Body(llen);
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				
				strResp +="\r\n";
				
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail><response errno=\"0\" reason=\"\">";
				
				vector<AttInfo> attlist;
				ExtractAttach(username.c_str(),Letter->GetSummary()->m_mime, attlist, lbuf);
				char tmpbuf[1024];
				for(int x = 0; x < attlist.size(); x++)
				{
					string strTmpname = attlist[x].tmpname;
					to_safty_xmlstring(strTmpname);
					string strFilename = attlist[x].filename;
					to_safty_xmlstring(strFilename);
					sprintf(tmpbuf, "<attach tmpname=\"%s\" filename=\"%s\" attsize=\"%u\" />", strTmpname.c_str() , strFilename.c_str(), attlist[x].size);
					strResp += tmpbuf;
				}
				strResp += "</response></erisemail>";
			}
			if(Letter)
				delete Letter;	
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiSaveDraft : public doc, public storage
{
public:
	ApiSaveDraft(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiSaveDraft() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
        
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			string strDraftID;
			string strFrom, strTo, strCc, strBcc, strSubject, strContent, strAttachFiles;
			unsigned int nDraftID;
			
			m_session->parse_urlencode_value("DRAFTID", strDraftID);
			
			m_session->parse_urlencode_value("TO_ADDRS", strTo);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strTo.c_str(), strTo);
			
			
			m_session->parse_urlencode_value("CC_ADDRS", strCc);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strCc.c_str(), strCc);
			
			m_session->parse_urlencode_value("BCC_ADDRS", strBcc);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strBcc.c_str(), strBcc);
			
			
			m_session->parse_urlencode_value("SUBJECT", strSubject);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strSubject.c_str(), strSubject);
			
			m_session->parse_urlencode_value("CONTENT", strContent);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strContent.c_str(), strContent);
			
			m_session->parse_urlencode_value("ATTACHFILES", strAttachFiles);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strAttachFiles.c_str(), strAttachFiles);
			
			
			char newuid[1024];
			sprintf(newuid, "%08x_%08x_%016lx_%08x_%s", time(NULL), getpid(), pthread_self(), random(), CMailBase::m_localhostname.c_str());
			int DraftID;
			if(strDraftID == "")
				DraftID = -1;
			else
				DraftID = atoi(strDraftID.c_str());
			
			int DirID = -1;
			if(DraftID  == -1)
			{
				m_mailStg->GetDirID(username.c_str(), "DRAFTS", DirID);
			}
			else
			{
				m_mailStg->GetMailDir(DraftID, DirID);
			}
			
			unsigned int mstatus = MSG_ATTR_RECENT;
			
			unsigned long long usermaxsize;
			if(m_mailStg && m_mailStg->GetUserSize(username.c_str(), usermaxsize) == -1)
			{
				usermaxsize = 5000*1024;
			}
			
			MailLetter* pLetter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), newuid, usermaxsize);

			Letter_Info letter_info;
			letter_info.mail_from = "";
			letter_info.mail_to = "";
            letter_info.host = "";
			letter_info.mail_type = mtLocal;
			letter_info.mail_uniqueid = newuid;
			letter_info.mail_dirid = DirID;
			letter_info.mail_status = mstatus;
			letter_info.mail_time = time(NULL);
			letter_info.user_maxsize = usermaxsize;
			letter_info.mail_id = DraftID;
					
			string strFromAddr;
			string strAlias;
			User_Info uinfo;
			
			if(m_mailStg && m_mailStg->GetID(username.c_str(), uinfo) == 0)
			{
				strAlias = uinfo.alias;
			}
			
			email eml(CMailBase::m_email_domain.c_str(), username.c_str(), strAlias.c_str());
			eml.set_to_addrs(strTo.c_str());
			eml.set_cc_addrs(strCc.c_str());
			eml.set_subject(strSubject.c_str());
			eml.set_content_type("html");
			eml.set_content(strContent.c_str());
			
			vector<string> vAtt;
			
			
			vSplitString(strAttachFiles, vAtt, "*", TRUE, 0x7FFFFFFFU);
			
			for(int z = 0; z < vAtt.size(); z++)
			{
				string path = CMailBase::m_private_path.c_str();
				path += "/tmp/";
				path += vAtt[z].c_str();
				eml.add_attach(path.c_str());
			}
			
			eml.output(m_session->m_clientip.c_str());
			
			char* mbuf = (char*)eml.m_buffer.c_buffer();
			int mlen = eml.m_buffer.length();
			int wlen = 0;
			BOOL isOK = TRUE;
			while(1)
			{
				if(pLetter->Write(mbuf + wlen, (mlen - wlen) > MEMORY_BLOCK_SIZE ? MEMORY_BLOCK_SIZE : (mlen - wlen)) < 0)
				{
					isOK = FALSE;
					break;
				}
				wlen += (mlen - wlen) > MEMORY_BLOCK_SIZE ? MEMORY_BLOCK_SIZE : (mlen - wlen);
				if(wlen >= mlen)
					break;
			}
			if(isOK)
				pLetter->SetOK();
			pLetter->Close();

			if(pLetter->isOK())
			{
				if(letter_info.mail_id == -1)
				{
					m_mailStg->InsertMailIndex(letter_info.mail_from.c_str(), letter_info.mail_to.c_str(), letter_info.host.c_str(), letter_info.mail_time,
						letter_info.mail_type, letter_info.mail_uniqueid.c_str(), letter_info.mail_dirid, letter_info.mail_status,
						pLetter->GetEmlName(), pLetter->GetSize(), letter_info.mail_id);
				}
				else
				{
					m_mailStg->UpdateMailIndex(letter_info.mail_from.c_str(), letter_info.mail_to.c_str(),letter_info.mail_time,
						letter_info.mail_type, letter_info.mail_uniqueid.c_str(), letter_info.mail_dirid, letter_info.mail_status,
						pLetter->GetEmlName(), pLetter->GetSize(), letter_info.mail_id);
				}
			}
			
			char szDraftID[64];
			sprintf(szDraftID, "%d", letter_info.mail_id);
			if(pLetter)
				delete pLetter;
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			if(isOK)
			{
				strResp += "<erisemail><response errno=\"0\" reason=\"\"><draftid>";
				strResp += szDraftID;
				strResp += "</draftid></response></erisemail>";
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"Mail size is too huge\"><draftid>";
				strResp += szDraftID;
				strResp += "</draftid></response></erisemail>";
			}			
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiSaveSent : public doc, public storage
{
public:
	ApiSaveSent(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiSaveSent() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
        
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			string strDraftID;
			string strFrom, strTo, strCc, strBcc, strSubject, strContent, strAttachFiles;
			unsigned int nDraftID;
			
			m_session->parse_urlencode_value("TO_ADDRS", strTo);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strTo.c_str(), strTo);
			
			
			m_session->parse_urlencode_value("CC_ADDRS", strCc);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strCc.c_str(), strCc);
			
			
			m_session->parse_urlencode_value("BCC_ADDRS", strBcc);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strBcc.c_str(), strBcc);
			
			m_session->parse_urlencode_value("SUBJECT", strSubject);
			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strSubject.c_str(), strSubject);
			
			m_session->parse_urlencode_value("CONTENT", strContent);

			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strContent.c_str(), strContent);
			
			m_session->parse_urlencode_value("ATTACHFILES", strAttachFiles);

			code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strAttachFiles.c_str(), strAttachFiles);
			
			char newuid[1024];
			sprintf(newuid, "%08x_%08x_%016lx_%08x_%s", time(NULL), getpid(), pthread_self(), random(), CMailBase::m_localhostname.c_str());
			int DraftID;
			if(strDraftID == "")
				DraftID = -1;
			else
				DraftID = atoi(strDraftID.c_str());
			
			int DirID = -1;
			if(DraftID  == -1)
			{
				m_mailStg->GetDirID(username.c_str(), "SENT", DirID);
			}
			else
			{
				m_mailStg->GetMailDir(DraftID, DirID);
			}
			
			unsigned int mstatus = MSG_ATTR_RECENT;
			
			unsigned long long usermaxsize;
			if(m_mailStg && m_mailStg->GetUserSize(username.c_str(), usermaxsize) == -1)
			{
				usermaxsize = 5000*1024;
			}
			
			MailLetter* pLetter = new MailLetter(m_mailStg, CMailBase::m_private_path.c_str(), CMailBase::m_encoding.c_str(), m_session->GetMemCached(), newuid, usermaxsize);

			Letter_Info letter_info;
			letter_info.mail_from = "";
			letter_info.mail_to = "";
            letter_info.host = "";
			letter_info.mail_type = mtLocal;
			letter_info.mail_uniqueid = newuid;
			letter_info.mail_dirid = DirID;
			letter_info.mail_status = mstatus;
			letter_info.mail_time = time(NULL);
			letter_info.user_maxsize = usermaxsize;
			letter_info.mail_id = DraftID;
					
			string strFromAddr;
			string strAlias;
			User_Info uinfo;
			
			if(m_mailStg && m_mailStg->GetID(username.c_str(), uinfo) == 0)
			{
				strAlias = uinfo.alias;
			}
			
			
			email eml(CMailBase::m_email_domain.c_str(), username.c_str(), strAlias.c_str());
			eml.set_to_addrs(strTo.c_str());
			eml.set_cc_addrs(strCc.c_str());
			eml.set_subject(strSubject.c_str());
			eml.set_content_type("html");
			eml.set_content(strContent.c_str());
			
			vector<string> vAtt;
			
			vSplitString(strAttachFiles, vAtt, "*", TRUE, 0x7FFFFFFFU);
			
			for(int z = 0; z < vAtt.size(); z++)
			{
				string path = CMailBase::m_private_path.c_str();
				path += "/tmp/";
				path += vAtt[z].c_str();
				eml.add_attach(path.c_str());
			}
			
			eml.output(m_session->m_clientip.c_str());
			
			char* mbuf = (char*)eml.m_buffer.c_buffer();
			int mlen = eml.m_buffer.length();
			int wlen = 0;
			BOOL isOK = TRUE;
			while(1)
			{
				if(pLetter->Write(mbuf + wlen, (mlen - wlen) > MEMORY_BLOCK_SIZE ? MEMORY_BLOCK_SIZE : (mlen - wlen)) < 0)
				{
					isOK = FALSE;
					break;
				}
				wlen += (mlen - wlen) > MEMORY_BLOCK_SIZE ? MEMORY_BLOCK_SIZE : (mlen - wlen);
				if(wlen >= mlen)
					break;
			}
			if(isOK)
				pLetter->SetOK();
			pLetter->Close();
			
			if(pLetter->isOK())
			{
				if(letter_info.mail_id == -1)
				{
					m_mailStg->InsertMailIndex(letter_info.mail_from.c_str(), letter_info.mail_to.c_str(), letter_info.host.c_str(), letter_info.mail_time,
						letter_info.mail_type, letter_info.mail_uniqueid.c_str(), letter_info.mail_dirid, letter_info.mail_status,
						pLetter->GetEmlName(), pLetter->GetSize(), letter_info.mail_id);
				}
				else
				{
					m_mailStg->UpdateMailIndex(letter_info.mail_from.c_str(), letter_info.mail_to.c_str(),letter_info.mail_time,
						letter_info.mail_type, letter_info.mail_uniqueid.c_str(), letter_info.mail_dirid, letter_info.mail_status,
						pLetter->GetEmlName(), pLetter->GetSize(), letter_info.mail_id);
				}
			}
			
			char szDraftID[64];
			sprintf(szDraftID, "%d", letter_info.mail_id);
			if(pLetter)
				delete pLetter;
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			if(isOK)
			{
				strResp += "<erisemail><response errno=\"0\" reason=\"\"><draftid>";
				strResp += szDraftID;
				strResp += "</draftid></response></erisemail>";
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"Mail size is too huge\"><draftid>";
				strResp += szDraftID;
				strResp += "</draftid></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};


class ApiLogin : public doc, public storage
{
public:
	ApiLogin(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiLogin() {}
	
	virtual void Response()
	{
		string strResp;
        string strXML;
		string username, password;
		if((m_session->parse_urlencode_value("USER_NAME", username) == -1) || (m_session->parse_urlencode_value("USER_PWD", password) == -1))
		{
            strXML= "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strXML += "<erisemail>"
                      "<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
                      "</erisemail>";
                
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
            
			char szContentLength[64];
            sprintf(szContentLength, "%u", strXML.length());
            strResp += "Content-Length: ";
            strResp += szContentLength;
            strResp += "\r\n";
                    
			strResp +="\r\n";

            strResp += strXML;
		}
		else
		{
		
			if(m_mailStg && m_mailStg->CheckLogin(username.c_str(), password.c_str()) == 0)
			{
				string name_and_role = username;
				name_and_role += ":";
				name_and_role += "U:";
				
                
                static char PWD_CHAR_TBL[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890=/~!@#$%^&*()_+=-?<>,.;:\"'\\|`";
        
                srand(time(NULL));
                
                char nonce[15];
                sprintf(nonce, "%c%c%c%08x%c%c%c",
                    PWD_CHAR_TBL[random()%(sizeof(PWD_CHAR_TBL)-1)],
                    PWD_CHAR_TBL[random()%(sizeof(PWD_CHAR_TBL)-1)],
                    PWD_CHAR_TBL[random()%(sizeof(PWD_CHAR_TBL)-1)],
                    (unsigned int)time(NULL),
                    PWD_CHAR_TBL[random()%(sizeof(PWD_CHAR_TBL)-1)],
                    PWD_CHAR_TBL[random()%(sizeof(PWD_CHAR_TBL)-1)],
                    PWD_CHAR_TBL[random()%(sizeof(PWD_CHAR_TBL)-1)]);
                    
                name_and_role += nonce;
                
				string strCookie, strEncoded, strOut;
				Security::Encrypt(name_and_role.c_str(), name_and_role.length(), strEncoded, CMailBase::DESKey());
				
                strCookie = username;
                strCookie += "#";
                strCookie += strEncoded;
                
                
				generate_cookie_content("AUTH_TOKEN", strCookie.c_str(), "/", strOut);
				
                strXML = "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strXML += "<erisemail>"
                          "<response errno=\"0\" reason=\"\"></response>"
                          "</erisemail>";

				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
                char szContentLength[64];
                sprintf(szContentLength, "%u", strXML.length());
                strResp += "Content-Length: ";
                strResp += szContentLength;
                strResp += "\r\n";
            
				strResp += "Set-Cookie: ";
				strResp += strOut;
				strResp += "\r\n\r\n";
				
                strResp += strXML;
			}
			else
			{
                				
				strXML = "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strXML += "<erisemail>"
                          "<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
                          "</erisemail>";
                    
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
                char szContentLength[64];
                sprintf(szContentLength, "%u", strXML.length());
                strResp += "Content-Length: ";
                strResp += szContentLength;
                strResp += "\r\n";
            
				strResp +="\r\n";
                    
                strResp += strXML;
			}
		}
        m_session->EnableKeepAlive(TRUE);
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiManLogin : public doc, public storage
{
public:
	ApiManLogin(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiManLogin() {}
	
	virtual void Response()
	{
		string strResp;
		string strXML;
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		
		string username, password;
		if((m_session->parse_urlencode_value("USER_NAME", username) == -1) || (m_session->parse_urlencode_value("USER_PWD", password) == -1))
		{
            strXML = "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strXML += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
                
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
            char szContentLength[64];
            sprintf(szContentLength, "%u", strXML.length());
            strResp += "Content-Length: ";
            strResp += szContentLength;
            strResp += "\r\n\r\n";
			strResp += strXML;
			
		}
		else
		{	
		
			if(m_mailStg && m_mailStg->CheckAdmin(username.c_str(), password.c_str()) == 0)
			{
                string name_and_role = username;
				name_and_role += ":";
				name_and_role += "A:";
				
                static char PWD_CHAR_TBL[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890=/~!@#$%^&*()_+=-?<>,.;:\"'\\|`";
        
                srand(time(NULL));
                
                char nonce[15];
                sprintf(nonce, "%c%c%c%08x%c%c%c",
                    PWD_CHAR_TBL[random()%(sizeof(PWD_CHAR_TBL)-1)],
                    PWD_CHAR_TBL[random()%(sizeof(PWD_CHAR_TBL)-1)],
                    PWD_CHAR_TBL[random()%(sizeof(PWD_CHAR_TBL)-1)],
                    (unsigned int)time(NULL),
                    PWD_CHAR_TBL[random()%(sizeof(PWD_CHAR_TBL)-1)],
                    PWD_CHAR_TBL[random()%(sizeof(PWD_CHAR_TBL)-1)],
                    PWD_CHAR_TBL[random()%(sizeof(PWD_CHAR_TBL)-1)]);
                    
                name_and_role += nonce;
                
                
				string strCookie, strEncoded, strOut;
				Security::Encrypt(name_and_role.c_str(), name_and_role.length(), strEncoded, CMailBase::DESKey());
				
                strCookie = username;
                strCookie += "#";
                strCookie += strEncoded;
                
                
				generate_cookie_content("AUTH_TOKEN", strCookie.c_str(), "/", strOut);
				
               				
                strXML = "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strXML += "<erisemail>"
					"<response errno=\"0\" reason=\"\"></response>"
					"</erisemail>";
                    
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
                char szContentLength[64];
                sprintf(szContentLength, "%u", strXML.length());
                strResp += "Content-Length: ";
                strResp += szContentLength;
                strResp += "\r\n";
                
				strResp += "Set-Cookie: ";
				strResp += strOut;
				strResp += "\r\n\r\n";
                strResp += strXML;
				
			}
			else
			{
                strXML = "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strXML += "<erisemail>"
					"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
					"</erisemail>";
                    
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
                char szContentLength[64];
                sprintf(szContentLength, "%u", strXML.length());
                strResp += "Content-Length: ";
                strResp += szContentLength;
                strResp += "\r\n\r\n";
				strResp += strXML;

			}
		}
        m_session->EnableKeepAlive(TRUE);
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiPasswd : public doc, public storage
{
public:
	ApiPasswd(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiPasswd() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strNewPwd;
		string strOldPwd;
		
		m_session->parse_urlencode_value("NEW_PWD", strNewPwd);
		m_session->parse_urlencode_value("OLD_PWD", strOldPwd);
		
		string username, password;
		if(check_userauth_token(strauth.c_str(), username) != 0) {
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
			m_session->HttpSend(strResp.c_str(), strResp.length());
			return;
		}
		if(m_mailStg && m_mailStg->CheckLogin(username.c_str(), strOldPwd.c_str()) == 0)
		{
            MailStorage* master_storage;
#ifdef _WITH_DIST_
            master_storage = new MailStorage(CMailBase::m_encoding.c_str(), CMailBase::m_private_path.c_str(), m_memcached);
            
            if(master_storage->Connect(CMailBase::m_master_db_host.c_str(), CMailBase::m_master_db_username.c_str(), CMailBase::m_master_db_password.c_str(),
                CMailBase::m_master_db_name.c_str(), CMailBase::m_master_db_port, CMailBase::m_master_db_sock_file.c_str()) != 0)
            {
                strResp = RSP_200_OK_XML;
                string strHTTPDate;
                OutHTTPDateString(time(NULL), strHTTPDate);
                strResp += "Date: ";
                strResp += strHTTPDate;
                strResp += "\r\n";
                
                strResp +="\r\n";
                
                strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
                
                strResp += "<erisemail>"
                    "<response errno=\"1\" reason=\"Couldn't connect master server\"></response>"
                    "</erisemail>";
            }
                
#else
            master_storage = m_mailStg;
#endif /* _WITH_DIST_ */
            if(master_storage && master_storage->Passwd(username.c_str(), strNewPwd.c_str()) == 0)
            {
                strResp = RSP_200_OK_XML;
                string strHTTPDate;
                OutHTTPDateString(time(NULL), strHTTPDate);
                strResp += "Date: ";
                strResp += strHTTPDate;
                strResp += "\r\n";
                
                strResp +="\r\n";
                strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
                strResp += "<erisemail>"
                    "<response errno=\"0\" reason=\"\"></response>"
                    "</erisemail>";
            }
            else
            {
                strResp = RSP_500_SYS_ERR;
                
            }
#ifdef _WITH_DIST_
            if(master_storage)
                delete master_storage;
#endif /* _WITH_DIST_ */
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiAlias : public doc, public storage
{
public:
	ApiAlias(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiAlias() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string strAlias;
		
		m_session->parse_urlencode_value("ALIAS", strAlias);
		
		string username, password;
		
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
            MailStorage* master_storage;
#ifdef _WITH_DIST_
            master_storage = new MailStorage(CMailBase::m_encoding.c_str(), CMailBase::m_private_path.c_str(), m_memcached);
            
            if(master_storage->Connect(CMailBase::m_master_db_host.c_str(), CMailBase::m_master_db_username.c_str(), CMailBase::m_master_db_password.c_str(), CMailBase::m_master_db_name.c_str(), CMailBase::m_master_db_port, CMailBase::m_master_db_sock_file.c_str()) != 0)
            {
                strResp = RSP_200_OK_XML;
                string strHTTPDate;
                OutHTTPDateString(time(NULL), strHTTPDate);
                strResp += "Date: ";
                strResp += strHTTPDate;
                strResp += "\r\n";
                
                strResp +="\r\n";
                
                strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
                
                strResp += "<erisemail>"
                    "<response errno=\"1\" reason=\"Couldn't connect master server\"></response>"
                    "</erisemail>";
            }
                
#else
            master_storage = m_mailStg;
#endif /* _WITH_DIST_ */
			if(master_storage && master_storage->Alias(username.c_str(), strAlias.c_str()) == 0)
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail>"
					"<response errno=\"0\" reason=\"\"></response>"
					"</erisemail>";
			}
			else
			{
				strResp = RSP_500_SYS_ERR;
				
			}
#ifdef _WITH_DIST_
            if(master_storage)
                delete master_storage;
#endif /* _WITH_DIST_ */
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiUserInfo : public doc, public storage
{
public:
	ApiUserInfo(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiUserInfo() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
		
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			User_Info uinfo; 
			Level_Info linfo;
							
			if(m_mailStg && m_mailStg->GetID(username.c_str(), uinfo) == 0 && m_mailStg->GetLevel(uinfo.level, linfo) == 0)
			{
                char szTmp[128];
                
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail><response errno=\"0\" reason=\"\">";
				strResp += "<user ";
				
				strResp += "alias=\"";
				strResp += uinfo.alias;
				strResp += "\" ";
				
				
				if(uinfo.role == 0)
					sprintf(szTmp, "%s", "Disabled");
				else if(uinfo.role == 1)
					sprintf(szTmp, "%s", "User");
				else if(uinfo.role == 2)
					sprintf(szTmp, "%s", "Administrator");
				
				strResp += "role=\"";
				strResp += szTmp;
				strResp += "\" ";
				
				sprintf(szTmp, "%llu", linfo.mailmaxsize);
				strResp += "mailsize=\"";
				strResp += szTmp;
				strResp += "\" ";

				sprintf(szTmp, "%llu", linfo.boxmaxsize);
				strResp += "boxsize=\"";
				strResp += szTmp;
				strResp += "\" ";

				strResp += "audit=\"";
				strResp += linfo.enableaudit == eaFalse ? "no" : "yes";
				strResp += "\" ";
					
				strResp += " />";
				strResp += "</response></erisemail>";
			}
			else
			{
				strResp = RSP_500_SYS_ERR;
			}
			
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiUnseenMail : public doc, public storage
{
public:
	ApiUnseenMail(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiUnseenMail() {}
	
	void TravelUnSeenMail(const char* username, int pid, string & strXML)
	{
		vector<Dir_Info> listtbl;	

		m_mailStg->ListSubDir(username, pid, listtbl);
		for(int i = 0; i < listtbl.size(); i++)
		{
			int num = 0;
			m_mailStg->GetUnseenMail(listtbl[i].did, num);
			
			char szTmp[256];
			sprintf(szTmp, "<unseen did=\"%d\" num=\"%d\" />", listtbl[i].did, num);
			strXML += szTmp;
			TravelUnSeenMail(username, listtbl[i].did, strXML);
		}
		
	}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;

		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			
			string strXML = "";
			TravelUnSeenMail(username.c_str(), -1, strXML);
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail><response errno=\"0\" reason=\"\">";
			strResp += strXML;
			strResp += "</response></erisemail>";
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiEmptyTrash : public doc, public storage
{
public:
	ApiEmptyTrash(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiEmptyTrash() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
        
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			int nDirID = -1;
			m_mailStg->GetTrashID(username.c_str(), nDirID);
			
			int num = 0;
			if(m_mailStg && m_mailStg->EmptyDir(username.c_str(), nDirID) < 0)
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				strResp += "<erisemail>"
					"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
					"</erisemail>";
			}
			else
			{
				strResp = RSP_200_OK_XML;
				string strHTTPDate;
				OutHTTPDateString(time(NULL), strHTTPDate);
				strResp += "Date: ";
				strResp += strHTTPDate;
				strResp += "\r\n";
				
				strResp +="\r\n";
				
				strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
				
				strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiCreateLabel : public doc, public storage
{
public:
	ApiCreateLabel(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiCreateLabel() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
        
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			string strNewLabel;
			string strDirId;
			m_session->parse_urlencode_value("NEW_LABEL", strNewLabel);
			m_session->parse_urlencode_value("DIRID", strDirId);
			
			int parentid;
			if(strDirId == "")
				parentid = -1;
			else
				parentid = atoi(strDirId.c_str());
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			string strTmp = strNewLabel;
			strTmp += "pad"; //for pad for convent safety
			
			utf8_to_utf7_ex(strTmp.c_str(), strTmp);
			
			strTmp.erase(strTmp.length() - 3, 3);
			
			utf7_standard_to_modified_ex(strTmp.c_str(), strTmp);
			
			if(m_mailStg && m_mailStg->CreateDir(username.c_str(), strTmp.c_str(), parentid) == 0)
			{
				strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiDeleteLabel : public doc, public storage
{
public:
	ApiDeleteLabel(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiDeleteLabel() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
        
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{
			string strDirId;
			m_session->parse_urlencode_value("DIRID", strDirId);
			
			int dirid;
			if(strDirId == "")
				dirid = -1;
			else
				dirid = atoi(strDirId.c_str());
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			if(m_mailStg && m_mailStg->DeleteDir(username.c_str(), dirid) == 0)
			{
				strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiLoadConfigFile : public doc, public storage
{
public:
	ApiLoadConfigFile(CHttp* session) : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiLoadConfigFile() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
        
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			string strConfigName;
			m_session->parse_urlencode_value("CONFIG_NAME", strConfigName);
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			if(strConfigName == "GLOBAL_REJECT_LIST")
			{
				sem_t* plock = sem_open(ERISEMAIL_GLOBAL_REJECT_LIST, O_CREAT | O_RDWR, 0644, 1);
				if(plock != SEM_FAILED)
					sem_wait(plock);
				
				ifstream filein(CMailBase::m_reject_list_file.c_str(), ios_base::binary);
				string strline;
				if(!filein.is_open())
				{
					strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
				}
				else
				{
					strResp += "<erisemail><response errno=\"0\" reason=\"\">";
					strResp +="<list>";
					while(getline(filein, strline))
					{
						strline += "\n";
						to_safty_xmlstring(strline);
						strResp += strline;
					}
					strResp += "</list>";
					strResp +="</response></erisemail>";
				}
				filein.close();
				
				if(plock != SEM_FAILED)
					sem_post(plock);
				
				if(plock != SEM_FAILED)
					sem_close(plock);
			}
			else if(strConfigName == "GLOBAL_PERMIT_LIST")
			{
				sem_t* plock = sem_open(ERISEMAIL_GLOBAL_PERMIT_LIST, O_CREAT | O_RDWR, 0644, 1);
				if(plock != SEM_FAILED)
					sem_wait(plock);
				
				ifstream filein(CMailBase::m_permit_list_file.c_str(), ios_base::binary);
				string strline;
				if(!filein.is_open())
				{
					strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
				}
				else
				{
					strResp += "<erisemail><response errno=\"0\" reason=\"\">";
					strResp +="<list>";
					while(getline(filein, strline))
					{
						strline += "\n";
						to_safty_xmlstring(strline);
						strResp += strline;
					}
					strResp += "</list>";
					strResp +="</response></erisemail>";
				}
				filein.close();
				
				if(plock != SEM_FAILED)
					sem_post(plock);
				
				if(plock != SEM_FAILED)
					sem_close(plock);
			}
			else if(strConfigName == "DOMAIN_PERMIT_LIST")
			{
				sem_t* plock = sem_open(ERISEMAIL_DOMAIN_PERMIT_LIST, O_CREAT | O_RDWR, 0644, 1);
				if(plock != SEM_FAILED)
					sem_wait(plock);
				
				ifstream filein(CMailBase::m_domain_list_file.c_str(), ios_base::binary);
				string strline;
				if(!filein.is_open())
				{
					strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
				}
				else
				{
					strResp += "<erisemail><response errno=\"0\" reason=\"\">";
					strResp +="<list>";
					while(getline(filein, strline))
					{
						strline += "\n";
						to_safty_xmlstring(strline);
						strResp += strline;
					}
					strResp += "</list>";
					strResp +="</response></erisemail>";
				}
				filein.close();
				
				if(plock != SEM_FAILED)
					sem_post(plock);
				
				if(plock != SEM_FAILED)
					sem_close(plock);
			}
			else if(strConfigName == "WEBADMIN_PERMIT_LIST")
			{
				sem_t* plock = sem_open(ERISEMAIL_WEBADMIN_PERMIT_LIST, O_CREAT | O_RDWR, 0644, 1);
				if(plock != SEM_FAILED)
					sem_wait(plock);
				
				ifstream filein(CMailBase::m_webadmin_list_file.c_str(), ios_base::binary);
				string strline;
				if(!filein.is_open())
				{
					strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
				}
				else
				{
					strResp += "<erisemail><response errno=\"0\" reason=\"\">";
					strResp +="<list>";
					while(getline(filein, strline))
					{
						strline += "\n";
						to_safty_xmlstring(strline);
						strResp += strline;
					}
					strResp += "</list>";
					strResp +="</response></erisemail>";
				}
				filein.close();
				
				if(plock != SEM_FAILED)
					sem_post(plock);
				
				if(plock != SEM_FAILED)
					sem_close(plock);
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiSaveConfigFile : public doc, public storage
{
public:
	ApiSaveConfigFile(CHttp* session) : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiSaveConfigFile() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
        
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			string strConfigName;
			m_session->parse_urlencode_value("CONFIG_NAME", strConfigName);
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			if(strConfigName == "GLOBAL_REJECT_LIST")
			{
				string strText;
				m_session->parse_urlencode_value("GLOBAL_REJECT_LIST", strText);

				code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strText.c_str(), strText);

				sem_t* plock = sem_open(ERISEMAIL_GLOBAL_REJECT_LIST, O_CREAT | O_RDWR, 0644, 1);
				if(plock != SEM_FAILED)
					sem_wait(plock);
				
				ofstream fileout(CMailBase::m_reject_list_file.c_str(), ios_base::binary|ios::out|ios::trunc);
				if(!fileout.is_open())
				{
					strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
				}
				else
				{
					fileout.write(strText.c_str(), strText.length());
					
					strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
				}
				fileout.close();
				
				if(plock != SEM_FAILED)
					sem_post(plock);
				
				if(plock != SEM_FAILED)
					sem_close(plock);
				
				Service mda_svr;
				mda_svr.ReloadList();
				
			}
			else if(strConfigName == "GLOBAL_PERMIT_LIST")
			{
				string strText;
				m_session->parse_urlencode_value("GLOBAL_PERMIT_LIST", strText);

				code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strText.c_str(), strText);

				sem_t* plock = sem_open(ERISEMAIL_GLOBAL_PERMIT_LIST, O_CREAT | O_RDWR, 0644, 1);
				if(plock != SEM_FAILED)
					sem_wait(plock);
				
				ofstream fileout(CMailBase::m_permit_list_file.c_str(), ios_base::binary|ios::out|ios::trunc);
				if(!fileout.is_open())
				{
					strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
				}
				else
				{
					fileout.write(strText.c_str(), strText.length());
					
					strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
				}
				fileout.close();
				
				if(plock != SEM_FAILED)
					sem_post(plock);
				
				if(plock != SEM_FAILED)
					sem_close(plock);
				
				Service mda_svr;
				mda_svr.ReloadList();
			}
			else if(strConfigName == "DOMAIN_PERMIT_LIST")
			{
				string strText;
				m_session->parse_urlencode_value("DOMAIN_PERMIT_LIST", strText);

				code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strText.c_str(), strText);
				
				sem_t* plock = sem_open(ERISEMAIL_DOMAIN_PERMIT_LIST, O_CREAT | O_RDWR, 0644, 1);
				if(plock != SEM_FAILED)
					sem_wait(plock);
				
				ofstream fileout(CMailBase::m_domain_list_file.c_str(), ios_base::binary|ios::out|ios::trunc);
				if(!fileout.is_open())
				{
					strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
				}
				else
				{
					fileout.write(strText.c_str(), strText.length());
					
					strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
				}
				fileout.close();
				
				if(plock != SEM_FAILED)
					sem_post(plock);
				
				if(plock != SEM_FAILED)
					sem_close(plock);
				
				Service mda_svr;
				mda_svr.ReloadList();
			}
			else if(strConfigName == "WEBADMIN_PERMIT_LIST")
			{
				string strText;
				m_session->parse_urlencode_value("WEBADMIN_PERMIT_LIST", strText);

				code_convert_ex("UTF-8", CMailBase::m_encoding.c_str(), strText.c_str(), strText);
				
				sem_t* plock = sem_open(ERISEMAIL_WEBADMIN_PERMIT_LIST, O_CREAT | O_RDWR, 0644, 1);
				if(plock != SEM_FAILED)
					sem_wait(plock);
				
				ofstream fileout(CMailBase::m_webadmin_list_file.c_str(), ios_base::binary|ios::out|ios::trunc);
				if(!fileout.is_open())
				{
					strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
				}
				else
				{
					fileout.write(strText.c_str(), strText.length());
					
					strResp += "<erisemail><response errno=\"0\" reason=\"\"></response></erisemail>";
				}
				fileout.close();
				
				if(plock != SEM_FAILED)
					sem_post(plock);
				
				if(plock != SEM_FAILED)
					sem_close(plock);
				
				Service mda_svr;
				mda_svr.ReloadList();
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"\"></response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiListLogFile : public doc, public storage
{
public:
	ApiListLogFile(CHttp* session) : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiListLogFile() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			
			return;
		}
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
        
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			string strConfigName;
			m_session->parse_urlencode_value("CONFIG_NAME", strConfigName);
			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			struct dirent * dirp;
			DIR *dp = opendir("/var/log/erisemail/");
			if(dp)
			{
				strResp += "<erisemail><response errno=\"0\" reason=\"\">";
				while( (dirp = readdir(dp)) != NULL)
				{
					if((strcmp(dirp->d_name, "..") == 0) || (strcmp(dirp->d_name, ".") == 0))
					{
						continue;
					}
					else
					{
						if(strmatch(dirp->d_name, "*.log") == 0)
						{
							string strpath = "/var/log/erisemail/";
							strpath += dirp->d_name;
							
							struct stat file_stat;
							stat(strpath.c_str(), &file_stat);					
							char szSize[64];
							sprintf(szSize, "%d", file_stat.st_size);
							
							strResp += "<log name=\"";
							strResp += dirp->d_name;
							strResp += "\" size=\"";
							strResp += szSize;
							strResp +="\" />";
						}
					}
				}
				closedir(dp);
				
				strResp += "</response></erisemail>";
			}
			else
			{
				strResp += "<erisemail><response errno=\"1\" reason=\"list log error\">";
				strResp += "</response></erisemail>";
			}
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiGetLogFile : public doc, public storage
{
public:
	ApiGetLogFile(CHttp* session) : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiGetLogFile() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			return;
		}
		
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
        
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{
			string strLogName;
			m_session->parse_urlencode_value("LOG_NAME", strLogName);
			
			if(strmatch("*.log", strLogName.c_str())   && strstr(strLogName.c_str(), "\\") == NULL && strstr(strLogName.c_str(), "/") == NULL && strstr(strLogName.c_str(), "../") == NULL && strstr(strLogName.c_str(), "..\\") == NULL)
			{
				string strPath = "/var/log/erisemail/";
				strPath += strLogName;
				
				ifstream filein(strPath.c_str(), ios_base::binary);
				string strline;
				if(filein.is_open())
				{
					strResp = RSP_200_OK_NO_CACHE_PLAIN;
					
					struct stat file_stat;
					stat(strPath.c_str(), &file_stat);					
					char szSize[64];
					sprintf(szSize, "%d", file_stat.st_size);
					
					strResp += "Content-Length: ";
					strResp += szSize;
					strResp += "\r\n";
					
					
					strResp += "Content-Disposition: inline; filename=\"";
					strResp += strLogName;
					strResp += ".txt";
					strResp += "\";\r\n";
					
					strResp +="\r\n";
					
					m_session->HttpSend(strResp.c_str(), strResp.length());
					
					char rbuf[1449];
					while(1)
					{
						if(filein.eof())
						{
							break;
						}
						//if(filein.read(rbuf, 1448) < 0)
						//{
						//	break;
						//}
                        filein.read(rbuf, 1448);
                        
						int rlen = filein.gcount();
						rbuf[rlen] = '\0';
						m_session->HttpSend(rbuf, rlen);
					}
					filein.close();
				}
				else
				{
					strResp = RSP_404_NOT_FOUND;
					m_session->HttpSend(strResp.c_str(), strResp.length());
				}
			}
			else
			{
				strResp = RSP_404_NOT_FOUND;
				m_session->HttpSend(strResp.c_str(), strResp.length());
			}
			return;
		}
		else
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			return;
		}
	}
	
};


class ApiSystemStatus : public doc, public storage
{
public:
	ApiSystemStatus(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiSystemStatus() {}
	
	virtual void Response()
	{
		string strResp;
		
		//Check Admin IPs
		BOOL access_result = FALSE;
		for(int x = 0; x < CMailBase::m_webadmin_list.size(); x++)
		{
			if(strmatch(CMailBase::m_webadmin_list[x].c_str(), m_session->m_clientip.c_str()) == TRUE)
			{
				access_result = TRUE;
				break;
			}
		}
		if(CMailBase::m_webadmin_list.size() == 0)
			access_result = TRUE;
		if(!access_result)
		{
			strResp = RSP_404_NOT_FOUND;
			m_session->HttpSend(strResp.c_str(), strResp.length());
			return;
		}
		
		
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
		
		if(check_adminauth_token(strauth.c_str(), username) == 0)
		{			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail><response errno=\"0\" reason=\"\">";
			
			unsigned int commonMailNumber;
			unsigned int deletedMailNumber;
			unsigned int commonMailSize;
			unsigned int deletedMailSize;
			m_mailStg->GetGlobalStorage(commonMailNumber, deletedMailNumber, commonMailSize, deletedMailSize);
			
			char szTmp[64];
			
			sprintf(szTmp, "%d", commonMailNumber);
			strResp += "<commonMailNumber>";
			strResp += szTmp;
			strResp += "</commonMailNumber>";
			
			sprintf(szTmp, "%d", deletedMailNumber);
			strResp += "<deletedMailNumber>";
			strResp += szTmp;
			strResp += "</deletedMailNumber>";
			
			sprintf(szTmp, "%d", commonMailSize);
			strResp += "<commonMailSize>";
			strResp += szTmp;
			strResp += "</commonMailSize>";
			
			sprintf(szTmp, "%d", deletedMailSize);
			strResp += "<deletedMailSize>";
			strResp += szTmp;
			strResp += "</deletedMailSize>";
			
			strResp += "<Encoding>";
			strResp += CMailBase::m_encoding;
			strResp += "</Encoding>";
			
			strResp += "<EmailDomainName>";
			strResp += CMailBase::m_email_domain;
			strResp += "</EmailDomainName>";
			
			strResp += "<LocalHostName>";
			strResp += CMailBase::m_localhostname;
			strResp += "</LocalHostName>";
			
			strResp += "<HostIP>";
			strResp += CMailBase::m_hostip;
			strResp += "</HostIP>";
			
			strResp += "<DNSServer>";
			strResp += CMailBase::m_dns_server;
			strResp += "</DNSServer>";
			
			sprintf(szTmp, "%d", CMailBase::m_mda_max_conn);
			strResp += "<MaxConnPerProtocal>";
			strResp += szTmp;
			strResp += "</MaxConnPerProtocal>";
			
			sprintf(szTmp, "%d", CMailBase::m_mta_relaythreadnum);
			strResp += "<RelayTaskNum>";
			strResp += szTmp;
			strResp += "</RelayTaskNum>";
			
			strResp += "<EnableSMTPTLS>";
			strResp += CMailBase::m_enablesmtptls ? "yes" : "no";
			strResp += "</EnableSMTPTLS>";
			
			strResp += "<EnableRelay>";
			strResp += CMailBase::m_enablerelay ? "yes" : "no";;
			strResp += "</EnableRelay>";
			
			strResp += "<SMTPHostNameCheck>";
			strResp += CMailBase::m_enablesmtphostnamecheck ? "yes" : "no";;
			strResp += "</SMTPHostNameCheck>";
			
			strResp += "<POP3Enable>";
			strResp += CMailBase::m_enablepop3 ? "yes" : "no";;
			strResp += "</POP3Enable>";
			
			sprintf(szTmp, "%d", CMailBase::m_pop3port);
			strResp += "<POP3Port>";
			strResp += szTmp;
			strResp += "</POP3Port>";
			
			strResp += "<EnablePOP3TLS>";
			strResp += CMailBase::m_enablepop3tls ? "yes" : "no";;
			strResp += "</EnablePOP3TLS>";
			
			strResp += "<IMAPEnable>";
			strResp += CMailBase::m_enableimap ? "yes" : "no";;
			strResp += "</IMAPEnable>";
			
			sprintf(szTmp, "%d", CMailBase::m_imapport);
			strResp += "<IMAPPort>";
			strResp += szTmp;
			strResp += "</IMAPPort>";
			
			strResp += "<EnableIMAPTLS>";
			strResp += CMailBase::m_enableimaptls ? "yes" : "no";;
			strResp += "</EnableIMAPTLS>";
			
			strResp += "<HTTPEnable>";
			strResp += CMailBase::m_enablehttp ? "yes" : "no";;
			strResp += "</HTTPEnable>";
			
			sprintf(szTmp, "%d", CMailBase::m_httpport);
			strResp += "<HTTPPort>";
			strResp += szTmp;
			strResp += "</HTTPPort>";
			
			strResp += "<XMPPEnable>";
			strResp += CMailBase::m_enablexmpp ? "yes" : "no";;
			strResp += "</XMPPEnable>";
			
			sprintf(szTmp, "%d", CMailBase::m_xmppport);
			strResp += "<XMPPPort>";
			strResp += szTmp;
			strResp += "</XMPPPort>";
            
            sprintf(szTmp, "%d", CMailBase::m_encryptxmpp);
			strResp += "<EncryptXMPP>";
			strResp += szTmp;
			strResp += "</EncryptXMPP>";
            
            sprintf(szTmp, "%d", CMailBase::m_xmpp_worker_thread_num);
			strResp += "<XMPPWorkerThreadNum>";
			strResp += szTmp;
			strResp += "</XMPPWorkerThreadNum>";
            
			strResp += "<SMTPSEnable>";
			strResp += CMailBase::m_enablesmtps ? "yes" : "no";;
			strResp += "</SMTPSEnable>";
			
			sprintf(szTmp, "%d", CMailBase::m_smtpsport);
			strResp += "<SMTPSPort>";
			strResp += szTmp;
			strResp += "</SMTPSPort>";
			
			strResp += "<POP3SEnable>";
			strResp += CMailBase::m_enablepop3s ? "yes" : "no";;
			strResp += "</POP3SEnable>";
			
			sprintf(szTmp, "%d", CMailBase::m_pop3sport);
			strResp += "<POP3SPort>";
			strResp += szTmp;
			strResp += "</POP3SPort>";
			
			strResp += "<IMAPSEnable>";
			strResp += CMailBase::m_enableimaps ? "yes" : "no";;
			strResp += "</IMAPSEnable>";
			
			sprintf(szTmp, "%d", CMailBase::m_imapsport);
			strResp += "<IMAPSPort>";
			strResp += szTmp;
			strResp += "</IMAPSPort>";
			
			strResp += "<HTTPSEnable>";
			strResp += CMailBase::m_enablehttps ? "yes" : "no";;
			strResp += "</HTTPSEnable>";
			
			sprintf(szTmp, "%d", CMailBase::m_httpsport);
			strResp += "<HTTPSPort>";
			strResp += szTmp;
			strResp += "</HTTPSPort>";

            strResp += "<LDAPEnable>";
			strResp += CMailBase::m_enable_local_ldap ? "yes" : "no";;
			strResp += "</LDAPEnable>";
			
			sprintf(szTmp, "%d", CMailBase::m_local_ldapport);
			strResp += "<LDAPPort>";
			strResp += szTmp;
			strResp += "</LDAPPort>";
            
            sprintf(szTmp, "%d", CMailBase::m_local_ldapsport);
			strResp += "<LDAPSPort>";
			strResp += szTmp;
			strResp += "</LDAPSPort>";
            
            sprintf(szTmp, "%d", CMailBase::m_encrypt_local_ldap);
			strResp += "<EncryptLDAP>";
			strResp += szTmp;
			strResp += "</EncryptLDAP>";
            
			strResp += "</response></erisemail>";
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiCurrentUsername : public doc, public storage
{
public:
	ApiCurrentUsername(CHttp* session)  : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiCurrentUsername() {}
	
	virtual void Response()
	{
		string strResp;
		string strauth;
		m_session->parse_cookie_value("AUTH_TOKEN", strauth);
		
		string username, password;
        
		if(check_userauth_token(strauth.c_str(), username) == 0)
		{			
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			
			strResp += "<erisemail><response errno=\"0\" reason=\"\">";
			strResp += "<loginusername>";
			strResp += username;
			strResp += "</loginusername>";
			strResp += "</response></erisemail>";
		}
		else
		{
			strResp = RSP_200_OK_XML;
			string strHTTPDate;
			OutHTTPDateString(time(NULL), strHTTPDate);
			strResp += "Date: ";
			strResp += strHTTPDate;
			strResp += "\r\n";
			
			strResp +="\r\n";
			strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};

class ApiLogout : public doc, public storage
{
public:
	ApiLogout(CHttp* session) : doc(session), storage(session->GetStorageEngine())
	{}
	
	virtual ~ApiLogout() {}
	
	virtual void Response()
	{
		string strResp;
		string strOut;
		generate_cookie_content("AUTH_TOKEN", "", "/", strOut);
		strResp = RSP_200_OK_XML;
		string strHTTPDate;
		OutHTTPDateString(time(NULL), strHTTPDate);
		strResp += "Date: ";
		strResp += strHTTPDate;
		strResp += "\r\n";
		
		strResp += "Set-Cookie: ";
		strResp += strOut;
		strResp += "\r\n\r\n";
		strResp += "<?xml version='1.0' encoding='" + CMailBase::m_encoding + "'?>";
		
		strResp += "<erisemail>"
			"<response errno=\"0\" reason=\"\"></response>"
			"</erisemail>";
		
		m_session->HttpSend(strResp.c_str(), strResp.length());
	}
	
};


class Webmail
{
protected:
	CHttp* m_session; 
	
public:
	Webmail(CHttp* session)
	{
		m_session = session;
	}
	virtual ~Webmail()
	{
		
	}
	void Response();
};

#endif /* _WEBMAIL_H_ */

