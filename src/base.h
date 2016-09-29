#ifndef _MAILSYS_H_
#define _MAILSYS_H_

#define CONFIG_FILTER_PATH	"/etc/erisemail/mfilter.xml"
#define CONFIG_FILE_PATH	"/etc/erisemail/erisemail.conf"
#define PERMIT_FILE_PATH	"/etc/erisemail/permit.list"
#define REJECT_FILE_PATH	"/etc/erisemail/reject.list"
#define DOMAIN_FILE_PATH	"/etc/erisemail/domain.list"
#define WEBADMIN_FILE_PATH	"/etc/erisemail/webadmin.list"

#include <openssl/rsa.h>     
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/engine.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <mqueue.h>
#include <semaphore.h>
#include <fstream>
#include <list>
#include <map>
#include <pthread.h>
#include <libmemcached/memcached.h>

#include "util/general.h"
#include "storage.h"
#include "util/base64.h"
#include "stgengine.h"

using namespace std;

#define ERISE_DES_KEY	"1001101001"

#define MAX_USERNAME_LEN	16
#define MAX_PASSWORD_LEN	16
#define MAX_EMAIL_LEN	5000 //about 5M attachment file

#define STATUS_ORIGINAL				0
#define STATUS_HELOED				(1<<0)
#define STATUS_EHLOED				(1<<1)
#define STATUS_AUTH_STEP1			(1<<2)
#define STATUS_AUTH_STEP2			(1<<3)
#define STATUS_AUTH_STEP3			(1<<4)
#define STATUS_AUTH_STEP4			(1<<5)
#define STATUS_AUTH_STEP5			(1<<6)
#define STATUS_AUTHED				(1<<7)
#define STATUS_MAILED				(1<<8)
#define STATUS_RCPTED				(1<<9)
#define STATUS_SUPOSE_DATAING		(1<<10)
#define STATUS_DATAED				(1<<11)
#define STATUS_SELECT				(1<<12)

#define MSG_EXIT			0xFF
#define MSG_GLOBAL_RELOAD	0xFE
#define MSG_FORWARD			0xFD
#define MSG_REJECT			0xFC
#define MSG_LIST_RELOAD		0xFB

typedef struct
{
	unsigned char aim;
	unsigned char cmd;
	union
	{
		char spool_uid[256];
		char reject_ip[256];
	} data;
}stQueueMsg;

typedef struct
{
	string ip;
	time_t expire;
}stReject;

class linesock
{
public:
	linesock(int fd)
	{
		dbufsize = 0;
		sockfd = fd;
		dbuf = (char*)malloc(4096);
		if(dbuf)
		{
			dbufsize = 4096;
		}
		dlen = 0;
	}
	
	virtual ~linesock()
	{
		if(dbuf)
			free(dbuf);
	}

	int drecv(char* pbuf, int blen)
	{
		int rlen = 0;
		
		if(blen <= dlen)
		{
			memcpy(pbuf, dbuf, blen);

			memmove(dbuf + blen, dbuf, dlen - blen);
			dlen = dlen - blen;
			
			rlen = blen;
		}
		else
		{
			
			memcpy(pbuf, dbuf, dlen);
			rlen = dlen;
			dlen = 0;
			
			int len = Recv(sockfd, pbuf + dlen, blen - dlen);
			if(len > 0)
			{
				rlen = rlen + len;	
			}
		}

		return rlen;
	}
	
	int lrecv(char* pbuf, int blen)
	{
		int taketime = 0;
		int res;
		fd_set mask; 
		struct timeval timeout; 
		char* p = NULL;
		int len;
		unsigned int nRecv = 0;

		int left;
		int right;
		p = dlen > 0 ? (char*)memchr(dbuf, '\n', dlen) : NULL;
		if(p != NULL)
		{
			left = p - dbuf + 1;
			right = dlen - left;
		
			if(blen >= left)
			{
				memcpy(pbuf, dbuf, left);
				memmove(dbuf, p + 1, right);
				dlen = right;
				pbuf[left] = '\0';
				return left;
			}
			else
			{
				memcpy(pbuf, dbuf, blen);
				memmove(dbuf, dbuf + blen, dlen - blen);
				dlen = dlen - blen;
				return -2;
			}
		}
		else
		{
			if(blen >= dlen)
			{
				memcpy(pbuf, dbuf, dlen);
				nRecv = dlen;
				dlen = 0;
			}
			else
			{
				memcpy(pbuf, dbuf, blen);
				memmove(dbuf, dbuf + blen, dlen - blen);
				dlen = dlen - blen;
				return -2;
			}
		}

		p = NULL;
		
		while(1)
		{
			if(nRecv >= blen)
				return -2;
			
			timeout.tv_sec = 1; 
			timeout.tv_usec = 0;
					
			FD_ZERO(&mask);
			FD_SET(sockfd, &mask);
			res = select(sockfd + 1, &mask, NULL, NULL, &timeout);
			
			if( res == 1) 
			{
				taketime = 0;
				len = recv(sockfd, pbuf + nRecv, blen - nRecv, 0);
				//printf("len: %d\n", len);
				if(len == 0)
                {
                    close(sockfd);
                    return -1;
                }
                else if(len <= 0)
                {
                    if( errno == EAGAIN)
                        continue;
                    close(sockfd);
                    return -1;
                }
				nRecv = nRecv + len;
				p = (char*)memchr(pbuf, '\n', nRecv);
				if(p != NULL)
				{
					left = p - pbuf + 1;
					right = nRecv - left;
				
					if(right > dbufsize)
					{
						if(dbuf)
							free(dbuf);
						dbuf = (char*)malloc(right);
						dbufsize = right;
					}
					memcpy(dbuf, p + 1, right);
					dlen = right;
					nRecv = left;
					pbuf[nRecv] = '\0';
					break;
				}
			}
			else if(res == 0)
			{
				
				taketime = taketime + 1;
				if(taketime > MAX_TRY_TIMEOUT)
				{
					close(sockfd);
					return -1;
				}
				continue;
			}
			else
			{
				return -1;
			}
			
		}
		
		return nRecv;
	}

private:
	int sockfd;
public:
	char* dbuf;
	int dlen;
	int dbufsize;
};

class linessl
{
public:
	linessl(int fd, SSL* ssl)
	{
		dbufsize = 0;
		sockfd = fd;
		sslhd = ssl;
		dbuf = (char*)malloc(4096);
		if(dbuf)
		{
			dbufsize = 4096;
		}
		dlen = 0;
	}
	
	virtual ~linessl()
	{
		if(dbuf)
			free(dbuf);
	}

	int drecv(char* pbuf, int blen)
	{
		int rlen = 0;
		
		if(blen <= dlen)
		{
			memcpy(pbuf, dbuf, blen);

			memmove(dbuf + blen, dbuf, dlen - blen);
			dlen = dlen - blen;
			
			rlen = blen;
		}
		else
		{
			
			memcpy(pbuf, dbuf, dlen);
			rlen = dlen;
			dlen = 0;
			
			int len = SSLRead(sockfd, sslhd, pbuf + dlen, blen - dlen);
			if(len > 0)
			{
				rlen = rlen + len;	
			}
		}

		return rlen;
	}
	
	int lrecv(char* pbuf, int blen)
	{
		int taketime = 0;
		int res;
		fd_set mask; 
		struct timeval timeout; 
		char* p = NULL;
		int len;
		unsigned int nRecv = 0;
		int ret;
		
		int left;
		int right;
		p = dlen > 0 ? (char*)memchr(dbuf, '\n', dlen) : NULL;
		if(p != NULL)
		{
			left = p - dbuf + 1;
			right = dlen - left;
		
			if(blen >= left)
			{
				memcpy(pbuf, dbuf, left);
				memmove(dbuf, p + 1, right);
				dlen = right;
				pbuf[left] = '\0';
				return left;
			}
			else
			{
				memcpy(pbuf, dbuf, blen);
				memmove(dbuf, dbuf + blen, dlen - blen);
				dlen = dlen - blen;
				return -2;
			}
		}
		else
		{
			if(blen >= dlen)
			{
				memcpy(pbuf, dbuf, dlen);
				nRecv = dlen;
				dlen = 0;
			}
			else
			{
				memcpy(pbuf, dbuf, blen);
				memmove(dbuf, dbuf + blen, dlen - blen);
				dlen = dlen - blen;
				return -2;
			}
		}

		p = NULL;

#if	1	
		while(1)
		{
			if(nRecv >= blen)
				return -2;
			
			timeout.tv_sec = 1; 
			timeout.tv_usec = 0;
					
			FD_ZERO(&mask);
			FD_SET(sockfd, &mask);
			res = select(sockfd + 1, &mask, NULL, NULL, &timeout);

			if( res == 1) 
			{
				taketime = 0;
				len = SSL_read(sslhd, pbuf + nRecv, blen - nRecv);
                if(len == 0)
                {
                    close(sockfd);
					return -1;
                }
				else if(len < 0)
				{
					ret = SSL_get_error(sslhd, len);
					if(ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
					{
						continue;
					}
					else
					{
						close(sockfd);
						return -1;
					}
				}
				nRecv = nRecv + len;
				p = (char*)memchr(pbuf, '\n', nRecv);
				if(p != NULL)
				{
					left = p - pbuf + 1;
					right = nRecv - left;
				
					if(right > dbufsize)
					{
						if(dbuf)
							free(dbuf);
						dbuf = (char*)malloc(right);
						dbufsize = right;
					}
					memcpy(dbuf, p + 1, right);
					dlen = right;
					nRecv = left;
					pbuf[nRecv] = '\0';
					break;
				}
			}
			else if(res == 0)
			{
				taketime = taketime + 1;
				if(taketime > MAX_TRY_TIMEOUT)
				{
					close(sockfd);
					return -1;
				}
				continue;
			}
			else
			{
				return -1;
			}
			
		}
#else
		while(1)
		{
			if(nRecv >= blen)
				return -2;
			
			len = SSL_read(sslhd, pbuf + nRecv, blen - nRecv);
			if(len <= 0)
			{
				close(sockfd);
				return -1;
			}
			nRecv = nRecv + len;
			p = (char*)memchr(pbuf, '\n', nRecv);
			if(p != NULL)
			{
				left = p - pbuf + 1;
				right = nRecv - left;
			
				if(right > dbufsize)
				{
					if(dbuf)
						free(dbuf);
					dbuf = (char*)malloc(right);
					dbufsize = right;
				}
				memcpy(dbuf, p + 1, right);
				dlen = right;
				nRecv = left;
				pbuf[nRecv] = '\0';
				break;
			}
		}
#endif
		return nRecv;
	}

private:
	int sockfd;
	SSL* sslhd;
	
public:
	char* dbuf;
	int dlen;
	int dbufsize;
};

class CMailBase
{
public:
	static string	m_sw_version;

	static string	m_private_path;
	static string 	m_html_path;
	static string	m_localhostname;
	static string	m_encoding;
	static string	m_email_domain;
	static string	m_hostip;
	static string	m_dns_server;
	static map<string, int> m_memcached_list;
	
	static unsigned short	m_smtpport;
	static BOOL		m_enablesmtptls;
	static BOOL		m_enablerelay;
	
	static BOOL		m_enablepop3;
	static unsigned short	m_pop3port;
	static BOOL		m_enablepop3tls;

	static BOOL		m_enableimap;
	static unsigned short	m_imapport;
	static BOOL		m_enableimaptls;

	static BOOL		m_enablesmtps;
	static unsigned short	m_smtpsport;

	static BOOL		m_enablepop3s;
	static unsigned short	m_pop3sport;

	static BOOL		m_enableimaps;
	static unsigned short	m_imapsport;

	static BOOL		m_enablehttp;
	static unsigned short	m_httpport;
	
	static BOOL		m_enablehttps;
	static unsigned short	m_httpsport;
	
	static BOOL 	m_enableclientcacheck;
	static string	m_ca_crt_root;
	static string	m_ca_crt_server;
	static string	m_ca_key_server;
	static string	m_ca_crt_client;
	static string	m_ca_key_client;

	static string	m_ca_password;
	
	static volatile unsigned int	m_global_uid;
	
	static string	m_db_host;
	static string	m_db_name;
	static string	m_db_username;
	static string	m_db_password;
	static unsigned int m_db_max_conn;
	static unsigned int 	m_relaytasknum;
	
	static BOOL		m_enablesmtphostnamecheck;
	static unsigned int	m_connect_num;
	static unsigned int	m_max_conn;
	
	static unsigned int	m_runtime;

	static string	m_config_file;
	static string	m_permit_list_file;
	static string	m_reject_list_file;
	static string	m_domain_list_file;
	static string	m_webadmin_list_file;
	

	static vector<stReject> m_reject_list;
	static vector<string> m_permit_list;
	static vector<string> m_domain_list;
	static vector<string> m_webadmin_list;

	static BOOL Is_Local_Domain(const char* domain);
    
    static char m_des_key[9];
    
    static BOOL m_userpwd_cache_updated;
	
public:	
	CMailBase();
	virtual ~CMailBase();
	
	static void SetConfigFile(const char* config_file, const char* permit_list_file, const char* reject_list_file, const char* domain_list_file, const char* webadmin_list_file);

	static BOOL LoadConfig();
	static BOOL UnLoadConfig();

	static BOOL LoadList();
	static BOOL UnLoadList();
	
    static const char* DESKey() { return m_des_key; }
    
	/* Pure virual function	*/
	virtual BOOL Parse(char* text) = 0;
	virtual int ProtRecv(char* buf, int len) = 0;
};

#define write_lock(fd, offset, whence, len) lock_reg(fd, F_SETLK, F_WRLCK, offset, whence, len)   

int inline lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)   
{   
	struct   flock   lock;   
	lock.l_type = type;   
	lock.l_start = offset;   
	lock.l_whence = whence;   
	lock.l_len = len;
	return (fcntl(fd,cmd, &lock));   
}   

int inline check_single_on(const char* pflag)   
{
	int fd, val;   
	char buf[12];   
	
    if((fd = open(pflag, O_WRONLY|O_CREAT, 0644)) < 0)
	{
		return 1;  
	}   
	
	/* try and set a write lock on the entire file   */   
	if(write_lock(fd, 0, SEEK_SET, 0) < 0)
	{   
		if((errno == EACCES) || (errno == EAGAIN))
		{   
            close(fd);
			return 1;   
		}
		else
		{   
			close(fd);   
			return 1;
		}   
	}   
	
	/* truncate to zero length, now that we have the lock   */   
	if(ftruncate(fd, 0) < 0)
	{   
		close(fd);               
		return 1;
	}   
	
	/*   write   our   process   id   */   
	sprintf(buf, "%d\n", ::getpid());   
	if(write(fd, buf, strlen(buf)) != strlen(buf))
	{   
		close(fd);               
		return 1;
	}   
	
	/*   set close-on-exec flag for descriptor   */   
	if((val = fcntl(fd, F_GETFD, 0) < 0 ))
	{   
		close(fd);   
		return 1;
	}   
	val |= FD_CLOEXEC;   
	if(fcntl(fd, F_SETFD, val) <0 )
	{   
		close(fd);   
		return 1;
	}
	/* leave file open until we terminate: lock will be held   */   
	return 0;   
} 

int inline try_single_on(const char* pflag)   
{
	int fd, val;   
	char buf[12];   
	
    if((fd = open(pflag, O_WRONLY|O_CREAT, 0644))   <0   )
	{
		return 1;  
	}   
	
	/* try and set a write lock on the entire file   */   
	if(write_lock(fd, 0, SEEK_SET, 0) < 0)
	{   
		if((errno == EACCES) || (errno == EAGAIN))
		{   
            close(fd);
			return 1;   
		}
		else
		{   
			close(fd);   
			return 1;
		}   
	}   
	
	/* truncate to zero length, now that we have the lock   */   
	if(ftruncate(fd, 0) < 0)
	{   
		close(fd);               
		return 1;
	}   
	
	/*   write   our   process   id   */   
	sprintf(buf, "%d\n", ::getpid());   
	if(write(fd, buf, strlen(buf)) != strlen(buf))
	{   
		close(fd);               
		return 1;
	}   
	
	/*   set close-on-exec flag for descriptor   */   
	if((val = fcntl(fd, F_GETFD, 0) < 0 ))
	{   
		close(fd);   
		return 1;
	}   
	val |= FD_CLOEXEC;   
	if(fcntl(fd, F_SETFD, val) <0 )
	{   
		close(fd);   
		return 1;
	}
	close(fd);
	return 0;   
}

BOOL inline create_ssl(int sockfd, 
    const char* ca_crt_root,
    const char* ca_crt_server,
    const char* ca_password,
    const char* ca_key_server,
    BOOL enableclientcacheck,
    SSL** pp_ssl, SSL_CTX** pp_ssl_ctx)
{
    int ssl_rc = -1;
    BOOL bSSLAccepted;
    X509* client_cert;
	SSL_METHOD* meth;
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
	meth = (SSL_METHOD*)SSLv23_server_method();
	*pp_ssl_ctx = SSL_CTX_new(meth);
	if(!*pp_ssl_ctx)
	{
		printf("SSL_CTX_use_certificate_file: %s\n", ERR_error_string(ERR_get_error(),NULL));
		goto clean_ssl3;
	}

	if(enableclientcacheck)
	{
    	SSL_CTX_set_verify(*pp_ssl_ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    	SSL_CTX_set_verify_depth(*pp_ssl_ctx, 4);
	}
	
	SSL_CTX_load_verify_locations(*pp_ssl_ctx, ca_crt_root, NULL);
	if(SSL_CTX_use_certificate_file(*pp_ssl_ctx, ca_crt_server, SSL_FILETYPE_PEM) <= 0)
	{
		printf("SSL_CTX_use_certificate_file: %s\n", ERR_error_string(ERR_get_error(),NULL));
		goto clean_ssl3;
	}

	SSL_CTX_set_default_passwd_cb_userdata(*pp_ssl_ctx, (char*)ca_password);
	if(SSL_CTX_use_PrivateKey_file(*pp_ssl_ctx, ca_key_server, SSL_FILETYPE_PEM) <= 0)
	{
		printf("SSL_CTX_use_certificate_file: %s\n", ERR_error_string(ERR_get_error(),NULL));
		goto clean_ssl3;

	}
	if(!SSL_CTX_check_private_key(*pp_ssl_ctx))
	{
		printf("SSL_CTX_use_certificate_file: %s\n", ERR_error_string(ERR_get_error(),NULL));
		goto clean_ssl3;
	}
	
	ssl_rc = SSL_CTX_set_cipher_list(*pp_ssl_ctx, "ALL");
    if(ssl_rc == 0)
    {
        printf("SSL_CTX_set_cipher_list: %s\n", ERR_error_string(ERR_get_error(),NULL));
        goto clean_ssl3;
    }
	SSL_CTX_set_mode(*pp_ssl_ctx, SSL_MODE_AUTO_RETRY);

	*pp_ssl = SSL_new(*pp_ssl_ctx);
	if(!*pp_ssl)
	{
		printf("SSL_new: %s\n", ERR_error_string(ERR_get_error(),NULL));
		goto clean_ssl2;
	}
	ssl_rc = SSL_set_fd(*pp_ssl, sockfd);
    if(ssl_rc == 0)
    {
        printf("SSL_set_fd: %s\n", ERR_error_string(ERR_get_error(),NULL));
        goto clean_ssl2;
    }
    ssl_rc = SSL_set_cipher_list(*pp_ssl, "ALL");
    if(ssl_rc == 0)
    {
        printf("SSL_set_cipher_list: %s\n", ERR_error_string(ERR_get_error(),NULL));
        goto clean_ssl2;
    }
    ssl_rc = SSL_accept(*pp_ssl);
	if(ssl_rc < 0)
	{
        printf("SSL_accept: %s\n", ERR_error_string(ERR_get_error(),NULL));
		goto clean_ssl2;
	}
    else if(ssl_rc = 0)
	{
		goto clean_ssl1;
	}

    bSSLAccepted = TRUE;
    
    if(enableclientcacheck)
    {
        ssl_rc = SSL_get_verify_result(*pp_ssl);
        if(ssl_rc != X509_V_OK)
        {
            fprintf(stderr, "SSL_get_verify_result: %s\n", ERR_error_string(ERR_get_error(),NULL));
            goto clean_ssl1;
        }
    }
        
	if(enableclientcacheck)
	{
		X509* client_cert;
		client_cert = SSL_get_peer_certificate(*pp_ssl);
		if (client_cert != NULL)
		{
			X509_free (client_cert);
		}
		else
		{
			printf("SSL_get_peer_certificate: %s\n", ERR_error_string(ERR_get_error(),NULL));
			goto clean_ssl1;
		}
	}

    return TRUE;

clean_ssl1:
    //printf("clean ssl1\n");
	if(*pp_ssl && bSSLAccepted)
    {
		SSL_shutdown(*pp_ssl);
        bSSLAccepted = FALSE;
    }
clean_ssl2:
    //printf("clean ssl2\n");
	if(*pp_ssl)
    {
		SSL_free(*pp_ssl);
        *pp_ssl = NULL;
    }
clean_ssl3:
    //printf("clean ssl3\n");
	if(*pp_ssl_ctx)
    {
		SSL_CTX_free(*pp_ssl_ctx);
        *pp_ssl_ctx = NULL;
    }
    return FALSE;
}

BOOL inline close_ssl(SSL* p_ssl, SSL_CTX* p_ssl_ctx)
{
	if(p_ssl)
    {
		SSL_shutdown(p_ssl);
        SSL_free(p_ssl);
        //printf("free ssl\n");
    }
	if(p_ssl_ctx)
    {
		SSL_CTX_free(p_ssl_ctx);
        //printf("free ssl ctx\n");
    }
}
#endif /* _MAILSYS_H_ */

