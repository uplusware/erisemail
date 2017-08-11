/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

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

#ifdef _WITH_GSSAPI_
    #include <gss.h>
    #define GSS_SEC_LAYER_NONE          0x1
    #define GSS_SEC_LAYER_INTEGRITY     0x2
    #define GSS_SEC_LAYER_PRIVACY       0x4

    #ifndef __attribute__
    /* This feature is available in gcc versions 2.5 and later.  */
    # if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
    #  define __attribute__(Spec)	/* empty */
    # endif
    #endif

    static void fail (const char *format, ...)
      __attribute__ ((format (printf, 1, 2)));
    static void success (const char *format, ...)
      __attribute__ ((format (printf, 1, 2)));

    static int debug = 0;
    static int error_count = 0;
    static int break_on_error = 0;

    static void
    fail (const char *format, ...)
    {
      va_list arg_ptr;

      va_start (arg_ptr, format);
      vfprintf (stderr, format, arg_ptr);
      va_end (arg_ptr);
      error_count++;
      if (break_on_error)
        exit (EXIT_FAILURE);
    }

    static void
    success (const char *format, ...)
    {
      va_list arg_ptr;

      va_start (arg_ptr, format);
      if (debug)
        vfprintf (stdout, format, arg_ptr);
      va_end (arg_ptr);
    }

    static void
    display_status_1 (const char *m, OM_uint32 code, int type)
    {
      OM_uint32 maj_stat, min_stat;
      gss_buffer_desc msg;
      OM_uint32 msg_ctx;

      msg_ctx = 0;
      do
        {
          maj_stat = gss_display_status (&min_stat, code,
                         type, GSS_C_NO_OID, &msg_ctx, &msg);
          if (GSS_ERROR (maj_stat))
        printf ("GSS-API display_status failed on code %d type %d\n",
            code, type);
          else
        {
          printf ("GSS-API error %s (%s): %.*s\n",
              m, type == GSS_C_GSS_CODE ? "major" : "minor",
              (int) msg.length, (char *) msg.value);

          gss_release_buffer (&min_stat, &msg);
        }
        }
      while (!GSS_ERROR (maj_stat) && msg_ctx);
    }

    static void
    display_status (const char *msg, OM_uint32 maj_stat, OM_uint32 min_stat)
    {
      display_status_1 (msg, maj_stat, GSS_C_GSS_CODE);
      display_status_1 (msg, min_stat, GSS_C_MECH_CODE);
    }
#endif /* _WITH_GSSAPI_ */

#include "util/trace.h"
#include "util/general.h"
#include "storage.h"
#include "util/base64.h"
#include "stgengine.h"
#include "sslapi.h"

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
		char mta_uid[256];
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

			memmove(dbuf, dbuf + blen , dlen - blen);
			dlen = dlen - blen;

			rlen = blen;
		}
		else
		{

			memcpy(pbuf, dbuf, dlen);
			rlen = dlen;
			dlen = 0;

			int len = Recv(sockfd, pbuf + rlen, blen - rlen);
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
				return blen;
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
				return blen;
			}
		}

		p = NULL;

		while(1)
		{
			if(nRecv >= blen)
				break;

			timeout.tv_sec = MAX_SOCKET_TIMEOUT;
			timeout.tv_usec = 0;

			FD_ZERO(&mask);
			FD_SET(sockfd, &mask);
			res = select(sockfd + 1, &mask, NULL, NULL, &timeout);

			if( res == 1)
			{
				taketime = 0;
				len = recv(sockfd, pbuf + nRecv, blen - nRecv, 0);
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
			else
			{
                close(sockfd);
				return -1;
			}

		}

		return nRecv;
	}

  int xrecv(char* pbuf, int blen)
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
		p = dlen > 0 ? (char*)memchr(dbuf, '>', dlen) : NULL;
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
				return blen;
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
				return blen;
			}
		}

		p = NULL;

		while(1)
		{
			if(nRecv >= blen)
				break;

			timeout.tv_sec = 1;
			timeout.tv_usec = 0;

			FD_ZERO(&mask);
			FD_SET(sockfd, &mask);
			res = select(sockfd + 1, &mask, NULL, NULL, &timeout);

			if( res > 0)
			{
				taketime = 0;
				len = recv(sockfd, pbuf + nRecv, blen - nRecv, 0);
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
				p = (char*)memchr(pbuf, '>', nRecv);
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
				if(taketime > MAX_SOCKET_TIMEOUT)
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

			memmove(dbuf, dbuf + blen, dlen - blen);
			dlen = dlen - blen;

			rlen = blen;
		}
		else
		{

			memcpy(pbuf, dbuf, dlen);
			rlen = dlen;
			dlen = 0;

			int len = SSLRead(sockfd, sslhd, pbuf + rlen, blen - rlen);
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
				return blen;
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
				return blen;
			}
		}

		p = NULL;
        
		while(1)
		{
			if(nRecv == blen)
				break;
            
            len = SSL_read(sslhd, pbuf + nRecv, blen - nRecv);
            if(len == 0)
            {
                ret = SSL_get_error(sslhd, len);
                if(ret == SSL_ERROR_ZERO_RETURN)
                {
                    printf("SSL_read: shutdown by the peer\n");
                }
                else if(ret == SSL_ERROR_SYSCALL)
                {
                    if(ERR_get_error() == 0)
                    {
                        printf("SSL_read: shutdown by the peer\n");
                    }
                    else
                        printf("SSL_read: %s\n", ERR_error_string(ERR_get_error(),NULL));
                }
                close(sockfd);
                return -1;
            }
            else if(len < 0)
            {
                ret = SSL_get_error(sslhd, len);
                if(ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
                {
                    timeout.tv_sec = MAX_SOCKET_TIMEOUT;
                    timeout.tv_usec = 0;

                    FD_ZERO(&mask);
                    FD_SET(sockfd, &mask);
                    
                    res = select(sockfd + 1, &mask, NULL, NULL, &timeout);

                    if( res == 1)
                    {
                        continue;
                    }
                    else if(res == 0)
                    {
                        close(sockfd);
                        return -1;
                    }
                    else
                    {
                        close(sockfd);
                        return -1;
                    }
                }
                else
                {
                    if(ret == SSL_ERROR_ZERO_RETURN)
                    {
                        printf("SSL_read: shutdown by the peer\n");
                    }
                    else if(ret == SSL_ERROR_SYSCALL)
                    {
                        if(ERR_get_error() == 0)
                        {
                            printf("SSL_read: shutdown by the peer\n");
                        }
                        else
                            printf("SSL_read: %s\n", ERR_error_string(ERR_get_error(),NULL));
                    }
                    close(sockfd);
                    return -1;
                }
            }
            else
            {
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
		}
		return nRecv;
	}

  int xrecv(char* pbuf, int blen)
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
		p = dlen > 0 ? (char*)memchr(dbuf, '>', dlen) : NULL;
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
				return blen;
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
				return blen;
			}
		}

		p = NULL;
        
		while(1)
		{
			if(nRecv == blen)
				break;
            
            len = SSL_read(sslhd, pbuf + nRecv, blen - nRecv);
            if(len == 0)
            {
                ret = SSL_get_error(sslhd, len);
                if(ret == SSL_ERROR_ZERO_RETURN)
                {
                    printf("SSL_read: shutdown by the peer\n");
                }
                else if(ret == SSL_ERROR_SYSCALL)
                {
                    if(ERR_get_error() == 0)
                    {
                        printf("SSL_read: shutdown by the peer\n");
                    }
                    else
                        printf("SSL_read: %s\n", ERR_error_string(ERR_get_error(),NULL));
                }
                close(sockfd);
                return -1;
            }
            else if(len < 0)
            {
                ret = SSL_get_error(sslhd, len);
                if(ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
                {
                    timeout.tv_sec = MAX_SOCKET_TIMEOUT;
                    timeout.tv_usec = 0;

                    FD_ZERO(&mask);
                    FD_SET(sockfd, &mask);
                    
                    res = select(sockfd + 1, &mask, NULL, NULL, &timeout);

                    if( res == 1)
                    {
                        continue;
                    }
                    else /* timeout or error */
                    {
                        close(sockfd);
                        return -1;
                    }
                }
                else
                {
                    if(ret == SSL_ERROR_ZERO_RETURN)
                    {
                        printf("SSL_read: shutdown by the peer\n");
                    }
                    else if(ret == SSL_ERROR_SYSCALL)
                    {
                        if(ERR_get_error() == 0)
                        {
                            printf("SSL_read: shutdown by the peer\n");
                        }
                        else
                            printf("SSL_read: %s\n", ERR_error_string(ERR_get_error(),NULL));
                    }
                    close(sockfd);
                    return -1;
                }
            }
            else
            {
                nRecv = nRecv + len;
                p = (char*)memchr(pbuf, '>', nRecv);
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
		}
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


/* 0: Non-encrypt or TLS optional; 1: TLS required; 2: Old-SSL-based */
#define XMPP_TLS_OPTIONAL   0
#define XMPP_TLS_REQUIRED   1
#define XMPP_OLDSSL_BASED   2

class CMailBase
{
public:
	static string	m_sw_version;

    static BOOL     m_close_stderr;
	static string	m_private_path;
	static string 	m_html_path;
	static string	m_localhostname;
	static string	m_encoding;
	static string	m_email_domain;
	static string	m_hostip;
	static string	m_dns_server;
	static map<string, int> m_memcached_list;

    static BOOL m_enablesmtp;
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

    static BOOL		m_enablexmpp;
	static unsigned short	m_xmppport;
    static unsigned int		m_encryptxmpp; //0: Non-encrypted or TLS optional; 1: TLS required; 2: SSL-based

	static BOOL 	m_enableclientcacheck;
	static string	m_ca_crt_root;
	static string	m_ca_crt_server;
	static string	m_ca_key_server;
	static string	m_ca_crt_client;
	static string	m_ca_key_client;
	static string	m_ca_password;

    static string   m_krb5_ktname;

	static string	m_db_host;
    static unsigned short	m_db_port;
	static string	m_db_name;
	static string	m_db_username;
	static string	m_db_password;
    static string	m_db_sock_file;
	static unsigned int m_db_max_conn;

    static BOOL m_enablemta;
	static unsigned int 	m_mta_relaytasknum;
	static unsigned int     m_mta_relaycheckinterval;

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

    //Non-pure virtual function
    virtual BOOL IsKeepAlive() { return FALSE; }
    virtual BOOL IsEnabledKeepAlive() { return FALSE; }
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

typedef enum
{
	stSMTP = 1,
	stPOP3,
	stIMAP,
	stHTTP,
    stXMPP,
	stMTA
} Service_Type;

#define MTA_SERVICE_NAME        "MTA"
#define MDA_SERVICE_NAME        "MDA"
#define WATCHER_SERVICE_NAME    "WATCHER"
#define XMPP_SERVICE_NAME       "XMPP"

#endif /* _MAILSYS_H_ */
