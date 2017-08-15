/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _HTTP_H_
#define _HTTP_H_

#include <libmemcached/memcached.h>

#include "base.h"
#include "fstring.h"
#include "cache.h"

typedef enum
{
	application_x_www_form_urlencoded = 0,
	multipart_form_data
} Post_Content_Type;

typedef enum 
{
	hmGet = 0,
	hmPost,
	hmPut,
	hmDelete,
	hmHead,
	hmOptions,
	hmTrace,
	hmPatch,
	hmMove,
	hmCopy,
	hmLink,
	hmUnlink,
	hmWrapped,
    hmConnect,
	hmUser
}Http_Method;

class formparamter
{
public:
	fbufseg m_seg;
	
	fbufseg m_header;
	fbufseg	m_data;
};

class formdata
{
private:
	const char* m_buf;
	const char* m_boundary;
public:
	vector<formparamter> m_paramters;

	const char* c_buffer() { return m_buf; }
	
	formdata(const char* buf, int buflen, const char* boundary)
	{
		m_buf = buf;
		m_boundary = boundary;
		string sboundary = "--";
		sboundary += boundary;
		sboundary += "\r\n";
		
		string eboundary = "--";
		eboundary += boundary;
		eboundary += "--\r\n";

		formparamter parmeter;
		parmeter.m_seg.m_byte_beg = 0;
		parmeter.m_seg.m_byte_end = 0;
		parmeter.m_header.m_byte_beg = 0;
		parmeter.m_header.m_byte_end = 0;
		parmeter.m_data.m_byte_beg = 0;
		parmeter.m_data.m_byte_end = 0;
		BOOL isHeader = FALSE;
		int x = 0;
		while(x < buflen)
		{
			if((memcmp(buf + x, sboundary.c_str(), sboundary.length()) == 0) && (memcmp(buf + x, eboundary.c_str(), eboundary.length()) != 0))
			{
				if((parmeter.m_seg.m_byte_beg == 0) && (parmeter.m_seg.m_byte_end == 0))
				{
					parmeter.m_seg.m_byte_beg = x + sboundary.length();
					parmeter.m_header.m_byte_beg = parmeter.m_seg.m_byte_beg;
				}
				else
				{
					parmeter.m_seg.m_byte_end = x - 1;
					parmeter.m_data.m_byte_end = parmeter.m_seg.m_byte_end - 2;
					
					m_paramters.push_back(parmeter);
					
					parmeter.m_seg.m_byte_beg = x + sboundary.length();
					parmeter.m_header.m_byte_beg = parmeter.m_seg.m_byte_beg;
				}
				x += sboundary.length();
				isHeader = TRUE;
			}
			else if(memcmp(buf + x, eboundary.c_str(), eboundary.length()) == 0)
			{
				parmeter.m_seg.m_byte_end = x - 1;
				parmeter.m_data.m_byte_end = parmeter.m_seg.m_byte_end - 2;				
				m_paramters.push_back(parmeter);
				
				x += eboundary.length();
				break;
			}
			else
			{
				if(isHeader == TRUE)
				{
					if(strncmp(buf + x, "\r\n\r\n", 4) == 0)
					{
						parmeter.m_header.m_byte_end = x - 1;
						parmeter.m_data.m_byte_beg = x  + 4;
						isHeader = FALSE;
						x += 4;
					}
					else
					{
						x++;
					}
				}
				else
				{
					x++;
				}
			}
		}
	}

	virtual ~formdata()
	{

	}
};

class CHttp : public CMailBase
{
public:
	CHttp(int sockfd, SSL * ssl, SSL_CTX * ssl_ctx, const char* clientip,
        StorageEngine* storage_engine, memory_cache* ch, memcached_st * memcached, BOOL isSSL = FALSE);
	virtual ~CHttp();

	virtual BOOL Parse(char* text);
	virtual int ProtRecv(char* buf, int len);
    virtual int ProtSend(const char* buf, int len) { return HttpSend(buf, len); };

	int HttpSend(const char* buf, int len);
	int HttpRecv(char* buf, int len);
	
	int parse_cookie_value(const char*szKey, string& strOut);
	int parse_urlencode_value(const char* szKey, string& strOut);
	int parse_multipart_value(const char* szKey, fbufseg & seg);
	int parse_multipart_filename(const char* szKey, string& filename);
	int parse_multipart_type(const char* szKey, string& type);

	
	const char* GetQueryPage() { return m_pagename.c_str(); }
	const char* GetUserName() { return m_username.c_str(); }
	const char* GetPassword() { return m_password.c_str(); }
	
	formdata* GetFormData() { return m_formdata; }

	void SetPagename(const char* pagename) { m_pagename = pagename; }

	memory_cache* m_cache;

	string m_clientip;
	Http_Method GetMethod() { return m_http_method; };

	StorageEngine* GetStorageEngine() { return m_storageEngine; }
	memcached_st * GetMemCached() { return m_memcached; }
    
    const char* GetIfModifiedSince() { return m_if_modified_since.c_str(); }
    
    virtual BOOL IsKeepAlive() { return m_keep_alive; }
    
    BOOL EnableKeepAlive(BOOL keep_alive) { m_enabled_keep_alive = keep_alive; }
    
    virtual BOOL IsEnabledKeepAlive() { return m_enabled_keep_alive; }
    
protected:
	BOOL m_isSSL;
	SSL* m_ssl;
	SSL_CTX* m_ssl_ctx;
	
	int m_sockfd;
	linesock* m_lsockfd;
	linessl * m_lssl;

	Http_Method m_http_method;
	string m_pagename;
	string m_data;
	fbuffer* m_data_ex;
	formdata* m_formdata;
	unsigned int m_content_length;

	Post_Content_Type m_content_type;
    
    string m_if_modified_since;
    
    BOOL m_keep_alive;
    BOOL m_enabled_keep_alive;
    
	string m_boundary;

	string m_host_addr;
	unsigned short m_host_port;

	string m_username;
	string m_password;

	string m_cookie;

	StorageEngine* m_storageEngine;
	memcached_st * m_memcached;
};

#endif /* _HTTP_H_ */

