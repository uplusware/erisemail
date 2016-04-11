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
#include "htdoc.h"
#include "fstring.h"
#include "http.h"

#define MAX_APPLICATION_X_WWW_FORM_URLENCODED_LEN (1024*512)
#define MAX_MULTIPART_FORM_DATA_LEN (1024*1024*4)

CHttp::CHttp(int sockfd, const char* clientip, StorageEngine* storage_engine, memory_cache* ch, memcached_st * memcached, BOOL isSSL)
{
	m_cache = ch;

	m_sockfd = sockfd;
	m_clientip = clientip;

	m_lsockfd = NULL;
	m_lssl = NULL;
	
	m_storageEngine = storage_engine;
	m_memcached = memcached;
	
	m_content_length = 0;
	
	m_ssl = NULL;
	m_ssl_ctx = NULL;

	m_data_ex = NULL;
	m_formdata = NULL;
	m_isSSL = isSSL;
	m_data = "";
	m_http_method = hmUser;

	if(isSSL)
	{	
		X509* client_cert;
		SSL_METHOD* meth;
		SSL_load_error_strings();
		OpenSSL_add_ssl_algorithms();
		meth = (SSL_METHOD*)SSLv3_server_method();
		m_ssl_ctx = SSL_CTX_new(meth);
		if(!m_ssl_ctx)
		{
			printf("SSL_CTX_use_certificate_file: %s\n", ERR_error_string(ERR_get_error(),NULL));
			goto clean_ssl;
		}

		SSL_CTX_set_verify(m_ssl_ctx, SSL_VERIFY_PEER, NULL);
		
		SSL_CTX_load_verify_locations(m_ssl_ctx, m_ca_crt_root.c_str(), NULL);
		if(SSL_CTX_use_certificate_file(m_ssl_ctx, m_ca_crt_server.c_str(), SSL_FILETYPE_PEM) <= 0)
		{
			printf("SSL_CTX_use_certificate_file: %s\n", ERR_error_string(ERR_get_error(),NULL));
			goto clean_ssl;
		}
		SSL_CTX_set_default_passwd_cb_userdata(m_ssl_ctx, (char*)m_ca_password.c_str());
		if(SSL_CTX_use_PrivateKey_file(m_ssl_ctx, m_ca_key_server.c_str(), SSL_FILETYPE_PEM) <= 0)
		{
			printf("SSL_CTX_use_certificate_file: %s\n", ERR_error_string(ERR_get_error(),NULL));
			goto clean_ssl;

		}
		if(!SSL_CTX_check_private_key(m_ssl_ctx))
		{
			printf("SSL_CTX_use_certificate_file: %s\n", ERR_error_string(ERR_get_error(),NULL));
			goto clean_ssl;
		}
		
		SSL_CTX_set_cipher_list(m_ssl_ctx, "RC4-MD5");
		SSL_CTX_set_mode(m_ssl_ctx, SSL_MODE_AUTO_RETRY);

		m_ssl = SSL_new(m_ssl_ctx);
		if(!m_ssl)
		{
			printf("SSL_new: %s\n", ERR_error_string(ERR_get_error(),NULL));
			goto clean_ssl;
		}
		SSL_set_fd(m_ssl, m_sockfd);
		if(SSL_accept(m_ssl) == -1)
		{
			printf("SSL_accept: %s\n", ERR_error_string(ERR_get_error(),NULL));
			goto clean_ssl;
		}
		if(m_enableclientcacheck)
		{
			X509* client_cert;
			client_cert = SSL_get_peer_certificate(m_ssl);
			if (client_cert != NULL)
			{
				X509_free (client_cert);
			}
			else
			{
				printf("SSL_get_peer_certificate: %s\n", ERR_error_string(ERR_get_error(),NULL));
				goto clean_ssl;
			}
		}

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
	
	m_global_uid++;

	m_content_type = application_x_www_form_urlencoded;

	return;	//DON'T Delete it

clean_ssl:
	if(m_ssl)
		SSL_shutdown(m_ssl);
	if(m_ssl)
		SSL_free(m_ssl);
	if(m_ssl_ctx)
		SSL_CTX_free(m_ssl_ctx);
	throw new string(ERR_error_string(ERR_get_error(), NULL));
}

CHttp::~CHttp()
{
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

	if(m_data_ex)
		delete m_data_ex;

	if(m_formdata)
		delete m_formdata;

	if(m_lsockfd)
		delete m_lsockfd;

	if(m_lssl)
		delete m_lssl;
	
}

int CHttp::HttpSend(const char* buf, int len)
{
	if(m_isSSL)
		if(m_ssl)
			return SSLWrite(m_sockfd, m_ssl, buf, len);
		else
			return -1;
	else
		return Send( m_sockfd, buf, len);
}

int CHttp::HttpRecv(char* buf, int len)
{
	if(m_isSSL)
		if(m_ssl)
			return m_lssl->drecv(buf, len);
		else
			return -1;
	else
		return m_lsockfd->drecv(buf, len);
}

int CHttp::ProtRecv(char* buf, int len)
{
	if(m_isSSL)
		if(m_ssl)
			return m_lssl->lrecv(buf, len);//SSLReadOneLine(m_ssl, buf, len);
		else
			return -1;
	else
	{
		return m_lsockfd->lrecv(buf, len);
	}
}

BOOL CHttp::Parse(char* text)
{
	string strtext = text;
	strtrim(strtext);
	if(strncasecmp(strtext.c_str(),"GET ", 4) == 0)
	{
		//printf("%s\n", strtext.c_str());
		m_http_method = hmGet;

		m_pagename = strtext.c_str();

		strcut(strtext.c_str(), "GET /", " ", m_pagename);
		strcut(m_pagename.c_str(), NULL, "?", m_pagename);
		
		strcut(strtext.c_str(), "?", " ", m_data);
		m_data +="&";
	}
	else if(strncasecmp(strtext.c_str(), "POST ", 5) == 0)
	{
		m_http_method = hmPost;

		strcut(strtext.c_str(), "POST /", " ", m_pagename);
		strcut(m_pagename.c_str(), NULL, "?", m_pagename);

		strcut(strtext.c_str(), "?", " ", m_data);
		m_data +="&";
		
	}
	else if(strncasecmp(strtext.c_str(), "PUT ", 4) == 0)
	{
		m_http_method = hmPut;

		strcut(strtext.c_str(), "PUT /", " ", m_pagename);
		strcut(m_pagename.c_str(), NULL, "?", m_pagename);

		strcut(strtext.c_str(), "?", " ", m_data);
		m_data +="&";
	}
	else if(strncasecmp(strtext.c_str(), "HEAD ", 5) == 0)
	{
		m_http_method = hmHead;

		strcut(strtext.c_str(), "HEAD /", " ", m_pagename);
		strcut(m_pagename.c_str(), NULL, "?", m_pagename);

		strcut(strtext.c_str(), "?", " ", m_data);
		m_data +="&";
	}
	else if(strncasecmp(strtext.c_str(), "DELETE ", 7) == 0)
	{
		m_http_method = hmDelete;

		strcut(strtext.c_str(), "DELETE /", " ", m_pagename);
		strcut(m_pagename.c_str(), NULL, "?", m_pagename);

		strcut(strtext.c_str(), "?", " ", m_data);
		m_data +="&";
	}
	else if(strncasecmp(strtext.c_str(), "OPTIONS ", 8) == 0)
	{
		m_http_method = hmOptions;

		strcut(strtext.c_str(), "OPTIONS /", " ", m_pagename);
		strcut(m_pagename.c_str(), NULL, "?", m_pagename);

		strcut(strtext.c_str(), "?", " ", m_data);
		m_data +="&";
	}
	else if(strncasecmp(strtext.c_str(), "TRACE ", 6) == 0)
	{
		m_http_method = hmTrace;

		strcut(strtext.c_str(), "TRACE /", " ", m_pagename);
		strcut(m_pagename.c_str(), NULL, "?", m_pagename);

		strcut(strtext.c_str(), "?", " ", m_data);
		m_data +="&";
	}
	else if(strncasecmp(strtext.c_str(), "PATCH ", 6) == 0)
	{
		m_http_method = hmPatch;

		strcut(strtext.c_str(), "PATCH /", " ", m_pagename);
		strcut(m_pagename.c_str(), NULL, "?", m_pagename);

		strcut(strtext.c_str(), "?", " ", m_data);
		m_data +="&";
	}
	else if(strncasecmp(strtext.c_str(), "MOVE ", 5) == 0)
	{
		m_http_method = hmMove;

		strcut(strtext.c_str(), "MOVE /", " ", m_pagename);
		strcut(m_pagename.c_str(), NULL, "?", m_pagename);

		strcut(strtext.c_str(), "?", " ", m_data);
		m_data +="&";
	}
	else if(strncasecmp(strtext.c_str(), "COPY ", 5) == 0)
	{
		m_http_method = hmCopy;

		strcut(strtext.c_str(), "COPY /", " ", m_pagename);
		strcut(m_pagename.c_str(), NULL, "?", m_pagename);

		strcut(strtext.c_str(), "?", " ", m_data);
		m_data +="&";
	}
	else if(strncasecmp(strtext.c_str(), "LINK ", 5) == 0)
	{
		m_http_method = hmLink;

		strcut(strtext.c_str(), "LINK /", " ", m_pagename);
		strcut(m_pagename.c_str(), NULL, "?", m_pagename);

		strcut(strtext.c_str(), "?", " ", m_data);
		m_data +="&";
	}
	else if(strncasecmp(strtext.c_str(), "UNLINK ", 7) == 0)
	{
		m_http_method = hmUnlink;

		strcut(strtext.c_str(), "UNLINK /", " ", m_pagename);
		strcut(m_pagename.c_str(), NULL, "?", m_pagename);

		strcut(strtext.c_str(), "?", " ", m_data);
		m_data +="&";
	}
	else if(strncasecmp(strtext.c_str(), "WRAPPED ", 8) == 0)
	{
		m_http_method = hmWrapped;

		strcut(strtext.c_str(), "WRAPPED /", " ", m_pagename);
		strcut(m_pagename.c_str(), NULL, "?", m_pagename);

		strcut(strtext.c_str(), "?", " ", m_data);
		m_data +="&";
	}
	else if(strncasecmp(strtext.c_str(),"Content-Length:", 15) == 0)
	{
		sscanf(strtext.c_str(), "Content-Length: %d", &m_content_length);
	}
	else if(strncasecmp(strtext.c_str(),"Content-Type:", 13) == 0)
	{
		if(strtext.find("application/x-www-form-urlencoded", 0, strlen("application/x-www-form-urlencoded")) != string::npos)
		{
			m_content_type = application_x_www_form_urlencoded;
		}
		else if(strtext.find("multipart/form-data", 0, strlen("multipart/form-data")) != string::npos) 
		{
			m_content_type = multipart_form_data;
			strcut(strtext.c_str(), "boundary=", NULL, m_boundary);
			strtrim(m_boundary);
		}
		else
		{
			m_content_type = application_x_www_form_urlencoded;
		}
	}
	else if(strncasecmp(strtext.c_str(), "Cookie:", 7) == 0)
	{
		strcut(strtext.c_str(), "Cookie:", NULL, m_cookie);
		strtrim(m_cookie);
	}
	else if(strncasecmp(strtext.c_str(), "Host:",5) == 0)
	{
		string host_port;
		strcut(strtext.c_str(), "Host: ", NULL, host_port);
		strcut(host_port.c_str(), NULL, ":", host_port);
		m_host_addr = host_port;
		strcut(host_port.c_str(), ":", NULL, host_port);
		m_host_port = (unsigned short)atoi(host_port.c_str());
	}
	else if(strncasecmp(strtext.c_str(), "Authorization:",14) == 0)
	{
		string strauth;
		strcut(strtext.c_str(), "Authorization: Basic ", NULL, strauth);		

		int outlen = BASE64_DECODE_OUTPUT_MAX_LEN(strauth.length());
		char * user_pwd_out = (char*)malloc(outlen);
		memset(user_pwd_out, 0, outlen);
		
		CBase64::Decode((char*)strauth.c_str(), strauth.length(), user_pwd_out, &outlen);
		strauth = user_pwd_out;
		free(user_pwd_out);

		strcut(strauth.c_str(), NULL, ":", m_username);
		strcut(strauth.c_str(), ":", NULL, m_password);
	}
	else if(strcasecmp(strtext.c_str(), "") == 0)
	{
		if(m_http_method == hmPost)
		{
			if(m_content_type == application_x_www_form_urlencoded)
			{
				if(m_content_length == 0)
				{
					while(1)
					{
						if(m_data.length() > MAX_APPLICATION_X_WWW_FORM_URLENCODED_LEN)
							break;
						char rbuf[1449];
						int rlen = HttpRecv(rbuf, 1448);
						if(rlen > 0)
						{
							rbuf[rlen] = '\0';
							m_data += rbuf;
						}
						else
							break;
					}
				}
				else
				{
					if(m_content_length < MAX_APPLICATION_X_WWW_FORM_URLENCODED_LEN)
					{
						char* post_data = (char*)malloc(m_content_length + 1);
						int nlen = HttpRecv(post_data, m_content_length);
						if( nlen > 0)
						{
							post_data[nlen] = '\0';
							m_data += post_data;
						}
						free(post_data);
					}
				}
			}
			else if(m_content_type == multipart_form_data)
			{
				m_data_ex = new fbuffer();
				if(m_content_length == 0)
				{
					while(1)
					{
						char rbuf[1449];
						memset(rbuf, 0, 1449);
						int rlen = HttpRecv(rbuf, 1448);
						if(rlen > 0)
						{
							m_data_ex->bufcat(rbuf, rlen);
						}
						else
						{
							break;
						}
					}
				}
				else
				{
					int nRecv = 0;
					while(1)
					{
						if(nRecv == m_content_length)
							break;
						char rbuf[1449];
						memset(rbuf, 0, 1449);
						int rlen = HttpRecv(rbuf, (m_content_length - nRecv) > 1448 ? 1448 : ( m_content_length - nRecv));
						if(rlen > 0)
						{
							nRecv += rlen;
							m_data_ex->bufcat(rbuf, rlen);
						}
						else
						{
							break;
						}
					}
				}
			}	
		}
		
		if(m_content_type == multipart_form_data)
		{
			m_formdata = new formdata(m_data_ex->c_buffer(), m_data_ex->length(), m_boundary.c_str());
		}
		
		if(m_pagename == "")
		{
			m_pagename = "index.html";
		}

		Htdoc *doc = new Htdoc(this);
		doc->Response();
			
		delete doc;

		return FALSE;
	}
	return TRUE;
}

int CHttp::parse_urlencode_value(const char* szKey, string& strOut)
{
	if(m_content_type == application_x_www_form_urlencoded)
	{
		const char* szVars = m_data.c_str();
		char* szTemp = (char*)malloc(strlen(szVars)+1);

		char* p = NULL;
				
		p = (char*)strstr(szVars, szKey);
		
		while(1)
		{
			if(p == NULL)
			{
				free(szTemp);
				return -1;
			}
		
			if((p == szVars || *(p - 1) == '&') && *(p + strlen(szKey)) == '=')
			{
				break;
			}
			else
			{
				p = (char*)strstr(p + strlen(szKey), szKey);
			}
		}

		int i = 0;
		while(1)
		{
			if((*(p + strlen(szKey) + i + 1) != '&' )&& (*(p + strlen(szKey) + i + 1) != '\0'))
			{
				szTemp[i] = *(p + strlen(szKey) + i + 1);
				i++;
			}
			else
			{
				break;
			}
		}
		szTemp[i]='\0';
		decodeURI((unsigned char*)szTemp, strOut);
		free(szTemp);
	}
	return 0;
}

int CHttp::parse_multipart_value(const char* szKey, fbufseg & seg)
{
	if(m_content_type == multipart_form_data)
	{
		for(int x = 0; x < m_formdata->m_paramters.size(); x++)
		{
			char* szHeader = (char*)malloc(m_formdata->m_paramters[x].m_header.m_byte_end - m_formdata->m_paramters[x].m_header.m_byte_beg + 2);
			memcpy(szHeader, m_formdata->c_buffer() + m_formdata->m_paramters[x].m_header.m_byte_beg, m_formdata->m_paramters[x].m_header.m_byte_end - m_formdata->m_paramters[x].m_header.m_byte_beg + 1);
			szHeader[m_formdata->m_paramters[x].m_header.m_byte_end - m_formdata->m_paramters[x].m_header.m_byte_beg + 1] = '\0';
			string name;
			fnfy_strcut(szHeader, " name=", " \t\r\n\"", "\r\n\";", name);
			if(name == "")
				fnfy_strcut(szHeader, "\tname=", " \t\r\n\"", "\r\n\";", name);
				if(name == "")
					fnfy_strcut(szHeader, ";name=", " \t\r\n\"", "\r\n\";", name);
					if(name == "")
						fnfy_strcut(szHeader, "name=", " \t\r\n\"", "\r\n\";", name);
			if(strcmp(name.c_str(), szKey) == 0)
			{
				seg.m_byte_beg = m_formdata->m_paramters[x].m_data.m_byte_beg;
				seg.m_byte_end = m_formdata->m_paramters[x].m_data.m_byte_end;
				free(szHeader);
				return 0;
			}
			else
			{
				
				free(szHeader);
			}
		}
	}
	return -1;
}

int CHttp::parse_multipart_filename(const char* szKey, string& filename)
{
	if(m_content_type == multipart_form_data)
	{
		for(int x = 0; x < m_formdata->m_paramters.size(); x++)
		{			
			char* szHeader = (char*)malloc(m_formdata->m_paramters[x].m_header.m_byte_end - m_formdata->m_paramters[x].m_header.m_byte_beg + 2);
			memcpy(szHeader, m_formdata->c_buffer() + m_formdata->m_paramters[x].m_header.m_byte_beg, m_formdata->m_paramters[x].m_header.m_byte_end - m_formdata->m_paramters[x].m_header.m_byte_beg + 1);
			szHeader[m_formdata->m_paramters[x].m_header.m_byte_end - m_formdata->m_paramters[x].m_header.m_byte_beg + 1] = '\0';
			string name;
			fnfy_strcut(szHeader, " name=", " \t\r\n\"", "\r\n\";", name);
			if(name == "")
				fnfy_strcut(szHeader, "\tname=", " \t\r\n\"", "\r\n\";", name);
				if(name == "")
					fnfy_strcut(szHeader, ";name=", " \t\r\n\"", "\r\n\";", name);
					if(name == "")
						fnfy_strcut(szHeader, "name=", " \t\r\n\"", "\r\n\";", name);
			if(strcmp(name.c_str(), szKey) == 0)
			{
				fnfy_strcut(szHeader, "filename=", " \t\r\n\"", "\r\n\";", filename);
				free(szHeader);
				return 0;
			}
			else
			{
				free(szHeader);
			}
		}
	}
	return -1;
}

int CHttp::parse_multipart_type(const char* szKey, string & type)
{
	if(m_content_type == multipart_form_data)
	{
		for(int x = 0; x < m_formdata->m_paramters.size(); x++)
		{
			
			char* szHeader = (char*)malloc(m_formdata->m_paramters[x].m_header.m_byte_end - m_formdata->m_paramters[x].m_header.m_byte_beg + 2);
			memcpy(szHeader, m_formdata->c_buffer() + m_formdata->m_paramters[x].m_header.m_byte_beg, m_formdata->m_paramters[x].m_header.m_byte_end - m_formdata->m_paramters[x].m_header.m_byte_beg + 1);
			szHeader[m_formdata->m_paramters[x].m_header.m_byte_end - m_formdata->m_paramters[x].m_header.m_byte_beg + 1] = '\0';
			
			string name;
			fnfy_strcut(szHeader, " name=", " \t\r\n\"", "\r\n\";", name);
			if(name == "")
				fnfy_strcut(szHeader, "\tname=", " \t\r\n\"", "\r\n\";", name);
				if(name == "")
					fnfy_strcut(szHeader, ";name=", " \t\r\n\"", "\r\n\";", name);
					if(name == "")
						fnfy_strcut(szHeader, "name=", " \t\r\n\"", "\r\n\";", name);
			if(strcmp(name.c_str(), szKey) == 0)
			{
				fnfy_strcut(szHeader, "Content-Type:", " \t\r\n\"", "\r\n\";", type);
				return 0;
			}
			else
			{
				free(szHeader);
			}
		}
	}
	return -1;
}

int CHttp::parse_cookie_value(const char* szKey, string& strOut)
{
	const char* szCookie = m_cookie.c_str();
	string strk = szKey;
	strk += "=";
	fnfy_strcut(szCookie, strk.c_str(), " \t\r\n\"", ";\r\n\"\t", strOut);
	return 0;
}

