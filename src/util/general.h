/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _ERISE_GENERAL_H_
#define _ERISE_GENERAL_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <string>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include <iconv.h>
#include <semaphore.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <openssl/rsa.h>     
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/opensslv.h>

#define MAX_MEMORY_THRESHOLD (1024*1024*2)
#define MEMORY_BLOCK_SIZE (1024*64)

#define MAX_STORAGE_CONN	32

#define MAX_SOCKET_TIMEOUT 120

using namespace std;

typedef unsigned int BOOL;
#define FALSE   0
#define TRUE   1

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;

typedef void* HANDLE;

#define _CSL_(s)   (sizeof(s)/sizeof(char) - 1)

#define MAX(a,b) ((a)>(b)?(a):(b))
#define HICH(c) (( (c) >= 'a' )&&((c) <= 'z')) ? ((c)+'A'-'a'):(c)
#define LOCH(c) (( (c) >= 'A' )&&((c) <= 'Z')) ? ((c)-'A'+'a'):(c)

#if ( defined _SOLARIS_OS_) || (defined _CYGWIN_)
__inline__ char* strcasestr(const char *s, const char* f)
{
	char c, sc;
	size_t len;
	if ((c = *f++) != 0)
	{
		c = tolower((unsigned char)c);
		len = strlen(f);
		do {
			do
			{
				if ((sc = *s++) == 0)
        			return (NULL);
      		} while ((char)tolower((unsigned char)sc) != c);
    	} while (strncasecmp(s, f, len) != 0);
    	s--;
	}
	return ((char *)s);
}
#endif /* _SOLARIS_OS_ */

int __inline__ checkip(const char *addr)
{
	int q[4];
	char *p;
	int i;
	
	if(sscanf(addr, "%d.%d.%d.%d", &(q[0]), &(q[1]), &(q[2]), &(q[3])) != 4)
	{
		return -1;
	}
	
	if(q[0] > 255 || q[0] < 0 ||
		q[1] > 255 || q[1] < 0 ||
		q[2] > 255 || q[2] < 0 ||
		q[3] > 255 || q[3] < 0)
	{
		return -1;
	}
	
	/* we know there are 3 dots */
	p = (char*)addr;
	if(p != NULL) { p = strchr(p, '.'); p++; }
	if(p != NULL) { p = strchr(p, '.'); p++; }
	if(p != NULL) { p = strchr(p, '.'); }
	for(i=0; *p != '\0' && i<4; i++, p++);
	if(*p != '\0')
	{
		return -1;
	}
	
	return 0;
}

void __inline__ strcut(const char* text, const char* begstring, const char* endstring ,string& strDest)
{
	string strText;
	strText = text;
	string::size_type ops1, ops2;
	if(begstring == NULL)
		ops1 = 0;
	else
		ops1 = strText.find(begstring, 0, strlen(begstring));

	if(endstring == NULL)
		ops2 = strText.length();
	else
		ops2 = strText.find(endstring, begstring == NULL ? 0 : ops1 + strlen(begstring), strlen(endstring));

	string::size_type subfirst, sublen;
	if(ops1 == string::npos)
		subfirst = strText.length();
	else
		subfirst = ops1 + (begstring ==  NULL ? 0 : strlen(begstring));
	
	if(ops2 == string::npos)
		sublen = strText.length() - subfirst;
	else
		sublen = ops2- subfirst;
	
	if(sublen <= 0)
		strDest = "";
	else
		strDest = strText.substr(subfirst, sublen);
}

void __inline__ fnfy_strcut(const char* text, const char* begstring, const char* notbegset, const char* endset ,string& strDest)
{
	string strText;
	strText = text;
	string::size_type ops0, ops1, ops2;
	if(begstring == NULL)
		ops0 = 0;
	else
		ops0 = strText.find(begstring, 0, strlen(begstring));

	if(ops0 == string::npos)
	{
		strDest = "";
		return;
	}
	ops1 = strText.find_first_not_of(notbegset, ops0 + (begstring == NULL ? 0: strlen(begstring)));
	ops2 = strText.find_first_of(endset, ops1);

	string::size_type subfirst, sublen;
	
	if(ops1 == string::npos)
		subfirst = strText.length();
	else
		subfirst = ops1;
	
	if(ops2 == string::npos)
		sublen = strText.length() - subfirst;
	else
		sublen = ops2 - subfirst;
	
	if(sublen <= 0)
		strDest = "";
	else
		strDest = strText.substr(subfirst, sublen);
}

void __inline__ fnln_strcut(const char* text, const char* begstring, const char* notbegset, const char* notendset ,string& strDest)
{
	string strText;
	strText = text;
	string::size_type ops0, ops1, ops2;
	if(begstring == NULL)
		ops0 = 0;
	else
		ops0 = strText.find(begstring, 0, strlen(begstring));

	if(ops0 == string::npos)
	{
		strDest = "";
		return;
	}
	
	ops1 = strText.find_first_not_of(notbegset, ops0 + (begstring == NULL ? 0: strlen(begstring)));
	ops2 = strText.find_last_not_of(notendset);

	string::size_type subfirst, sublen;
	
	if(ops1 == string::npos)
		subfirst = strText.length();
	else
		subfirst = ops1;
	
	if(ops2 == string::npos)
		sublen = strText.length() - subfirst;
	else
		sublen = ops2 - subfirst + 1;
	
	if(sublen <= 0)
		strDest = "";
	else
		strDest = strText.substr(subfirst, sublen);
}

int __inline__ Recv(int sockfd, char* buf, unsigned int buf_len)
{
	int taketime = 0;
	int res;
	fd_set mask; 
	struct timeval timeout; 
	
	int len;
	unsigned int nRecv = 0;
	while(1)
	{
		timeout.tv_sec = MAX_SOCKET_TIMEOUT; 
		timeout.tv_usec = 0;

		FD_ZERO(&mask);
		FD_SET(sockfd, &mask);
		res = select(sockfd + 1, &mask, NULL, NULL, &timeout);
		
		if( res == 1) 
		{
			taketime = 0;
			len = recv(sockfd, buf + nRecv, buf_len - nRecv, 0);
			if(len == 0)
            {
                close(sockfd);
                return nRecv; 
            }
			else if(len < 0)
			{
                if( errno == EAGAIN)
                    continue;
				close(sockfd);
				return nRecv;
			}
			nRecv = nRecv + len;
			if(nRecv == buf_len)
				return nRecv;
		}
		else /* timeout or error */
		{
			close(sockfd);
			return -1;
		}
		
	}
	return nRecv;
}
	
int __inline__ RecvTimed(int sockfd, char* buf, unsigned int buf_len, unsigned int time_sec = 1)
{
	int taketime = 0;
	int res;
	fd_set mask; 
	struct timeval timeout; 
	
	int len = 0;

    timeout.tv_sec = time_sec; 
    timeout.tv_usec = 0;

    FD_ZERO(&mask);
    FD_SET(sockfd, &mask);
    res = select(sockfd + 1, &mask, NULL, NULL, &timeout);
    
    if( res == 1) 
    {
        taketime = 0;
        len = recv(sockfd, buf, buf_len, 0);
        if(len == 0)
        {
            close(sockfd);
            return -1; 
        }
        else if(len < 0)
        {
            if( errno == EAGAIN)
                return 0;
            close(sockfd);
            return -1;
        }
        return len;
    }
    else if(res == 0) /* timeout, do nothing */
    {
        return 0;
    }
    else
    {
        close(sockfd);
        return -1;
    }
		
	return len;
}

int __inline__ Send(int sockfd, const char * buf, unsigned int buf_len)
{	
	if(buf_len == 0)
		return 0;
	int taketime = 0;
	int res;
	fd_set mask; 
	struct timeval timeout; 
	
	unsigned int nSend = 0;
	
	while(1)
	{
		timeout.tv_sec = MAX_SOCKET_TIMEOUT; 
		timeout.tv_usec = 0;

		FD_ZERO(&mask);
		FD_SET(sockfd, &mask);
		
		res = select(sockfd + 1, NULL, &mask, NULL, &timeout);
		if( res == 1) 
		{
			taketime = 0;
			int len = send(sockfd, buf + nSend, buf_len - nSend, 0);

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
			nSend = nSend + len;
			if(nSend == buf_len)
				return 0;
		}
		else /* timeout or error */
		{
			close(sockfd);
			return -1;
		}
	}
	return 0;
}

int __inline__ SSLRead(int sockfd, SSL* ssl, char * buf, unsigned int buf_len)
{
#if 0
	int len, ret;
	unsigned int nRecv = 0;
	while(1)
	{
		len = SSL_read(ssl, buf + nRecv, buf_len - nRecv);
		if(len == 0)
        {
            close(sockfd);
            return -1;
        }
        else if(len < 0)
        {
            ret = SSL_get_error(ssl, len);
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
		if(nRecv == buf_len)
			return nRecv;	
	}
#else
	if(buf_len == 0)
		return 0;
	
	int taketime = 0;
	int res;
	int ret;
	fd_set mask; 
	struct timeval timeout; 
	
	int len;
	unsigned int nRecv = 0;
	FD_ZERO(&mask);
	while(1)
	{
        len = SSL_read(ssl, buf + nRecv, buf_len - nRecv);
        
        if(len == 0)
        {
            close(sockfd);
            return -1;
        }
        else if(len < 0)
        {
            ret = SSL_get_error(ssl, len);
            if(ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
            {
                timeout.tv_sec = MAX_SOCKET_TIMEOUT; 
                timeout.tv_usec = 0;

                FD_SET(sockfd, &mask);
                res = select(sockfd + 1, ret == SSL_ERROR_WANT_READ ? &mask : NULL, ret == SSL_ERROR_WANT_WRITE ? &mask : NULL, NULL, &timeout);
                
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
                close(sockfd);
                return -1;
            }
        }
        nRecv = nRecv + len;
        if(nRecv == buf_len)
            return nRecv;
	}
#endif
	return nRecv;
}

int __inline__ SSLReadTimed(int sockfd, SSL* ssl, char * buf, unsigned int buf_len, unsigned int time_sec = 1)
{
	if(buf_len == 0)
		return 0;
	
	int taketime = 0;
	int res;
	int ret;
	fd_set mask; 
	struct timeval timeout; 
	
	int len;
	FD_ZERO(&mask);
	
    len = SSL_read(ssl, buf, buf_len);
    
    if(len == 0)
    {
        close(sockfd);
        return -1;
    }
    else if(len < 0)
    {
        ret = SSL_get_error(ssl, len);
        if(ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
        {
            timeout.tv_sec = time_sec;
            timeout.tv_usec = 0;

            FD_SET(sockfd, &mask);
            res = select(sockfd + 1, &mask, NULL, NULL, &timeout);
            
            if( res == 1 || res == 0) 
            {
                return 0;
            }
            else /* timeout or error */
            {
                close(sockfd);
                return -1;
            }
        }
        else
        {
            close(sockfd);
            return -1;
        }
    }
    
	return len;
}

int __inline__ SSLWrite(int sockfd, SSL* ssl, const char * buf, unsigned int buf_len)
{
#if 0
	if(buf_len == 0)
		return 0;
	unsigned int nSend = 0;
    int len, ret;
	while(1)
	{
		len = SSL_write(ssl, buf + nSend, buf_len - nSend);
		if(len == 0)
        {
            close(sockfd);
            return -1;
        }
        else if(len < 0)
        {
            ret = SSL_get_error(ssl, len);
            if(ret == SSL_ERROR_WANT_WRITE || ret == SSL_ERROR_WANT_READ)
            {
                continue;
            }
            else
            {
                close(sockfd);
                return -1;
            }

        }
		nSend = nSend + len;
		if(nSend == buf_len)
			break;
	}
	return 0;
#else
	if(buf_len == 0)
		return 0;
	int taketime = 0;
	int res;
	fd_set mask; 
	struct timeval timeout; 
	int ret;
	unsigned int nSend = 0;
	FD_ZERO(&mask);
	
	while(1)
	{
        int len = SSL_write(ssl, buf + nSend, buf_len - nSend);
        if(len == 0)
        {
            close(sockfd);
            return -1;
        }
        else if(len < 0)
        {
            ret = SSL_get_error(ssl, len);
            if(ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
            {
                timeout.tv_sec = MAX_SOCKET_TIMEOUT; 
                timeout.tv_usec = 0;

                FD_SET(sockfd, &mask);
                
                res = select(sockfd + 1, ret == SSL_ERROR_WANT_READ ? &mask : NULL, ret == SSL_ERROR_WANT_WRITE ? &mask : NULL, NULL, &timeout);
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
                close(sockfd);
                return -1;
            }

        }
        nSend = nSend + len;
        if(nSend == buf_len)
            return 0;
	}
	return 0;
#endif
}

void __inline__ Replace(string& src, const char* dst, const char* rpl)
{
	string::size_type pos = 0 - strlen(rpl);
	while( (pos = src.find(dst, pos + strlen(rpl)) ) != string::npos )
	{
		src.replace(pos, strlen(dst), rpl);
	}
	
}

void __inline__ _strdelete_(string& src, const char* str)
{
	int slen = strlen(str);
	char* tmpstr = (char*)malloc(src.length() + 1);
	memcpy(tmpstr, src.c_str(), src.length());
	tmpstr[src.length()] = '\0';
	
	while(1)
	{
		char* p = strstr(tmpstr, str);
		
		if(p != NULL)
		{
			memmove(p, p + slen, (tmpstr + strlen(tmpstr)) - (p + slen) + 1);
		}
		else
		{
			break;
		}
	}

	src = tmpstr;
	free(tmpstr);
}

void __inline__ strtrim(string &src)
{
	string::size_type ops1, ops2, ops3, ops4;
	BOOL bExit = FALSE;
	while(!bExit)
	{
		bExit = TRUE;
		ops1 = src.find_first_not_of(" ");
		if((ops1 != string::npos)&&(ops1 != 0))
		{
			src.erase(0, ops1);
			bExit = FALSE;
		}
		else if(ops1 == string::npos)
		{
			src.erase(0, src.length());
		}
		ops2 = src.find_first_not_of("\t");
		if((ops2 != string::npos)&&(ops2 != 0))
		{
			src.erase(0, ops2);
		}
		else if(ops2 == string::npos)
		{
			src.erase(0, src.length());
		}
		ops3 = src.find_first_not_of("\r");
		if((ops3 != string::npos)&&(ops3 != 0))
		{
			src.erase(0, ops3);
		}
		else if(ops3 == string::npos)
		{
			src.erase(0, src.length());
		}
			
		ops4 = src.find_first_not_of("\n");
		if((ops4 != string::npos)&&(ops4 != 0))
		{
			src.erase(0, ops4);
		}
		else if(ops4 == string::npos)
		{
			src.erase(0, src.length());
		}
	}
	bExit = FALSE;
	while(!bExit)
	{
		bExit = TRUE;
		ops1 = src.find_last_not_of(" ");
		if((ops1 != string::npos)&&(ops1 != src.length() - 1 ))
		{
			src.erase(ops1 + 1, src.length() - ops1 - 1);
			bExit = FALSE;
		}
		
		ops2 = src.find_last_not_of("\t");
		if((ops2 != string::npos)&&(ops2 != src.length() - 1 ))
		{
			src.erase(ops2 + 1, src.length() - ops2 - 1);
			bExit = FALSE;
		}
		ops3 = src.find_last_not_of("\r");
		if((ops3 != string::npos)&&(ops3 != src.length() - 1 ))
		{
			src.erase(ops3 + 1, src.length() - ops3 - 1);
			bExit = FALSE;
		}
			
		ops4 = src.find_last_not_of("\n");
		if((ops4 != string::npos)&&(ops4 != src.length() - 1 ))
		{
			src.erase(ops4 + 1, src.length() - ops4 - 1);
			bExit = FALSE;
		}
	}
}

void __inline__ strtrim(string &src, const char* aim)
{
	string::size_type ops1;
	ops1 = src.find_first_not_of(aim);
	if((ops1 != string::npos)&&(ops1 != 0))
	{
		src.erase(0, ops1);
	}
	else if(ops1 == string::npos)
	{
		src.erase(0, src.length());
	}
	
	ops1 = src.find_last_not_of(aim);
	if((ops1 != string::npos)&&(ops1 != src.length() - 1 ))
	{
		src.erase(ops1 + 1, src.length() - ops1 - 1);
	}
}

void __inline__ strtolower(char* str)
{
	int slen = strlen(str);
	for(int x = 0; x < slen; x++)
		str[x] = tolower(str[x]);
}

void __inline__ strtoupper(char* str)
{
	int slen = strlen(str);
	for(int x = 0; x < slen; x++)
		str[x] = toupper(str[x]);
}

BOOL __inline__ strmatch(const char* aPattern, const char* aSource)
{
	char* StringPtr, *PatternPtr;
	char* StringRes, *PatternRes;
	BOOL Result = FALSE;
	BOOL isTransfed = FALSE;
	StringPtr = (char*)aSource;
	PatternPtr = (char*)aPattern;
	StringRes = NULL;
	PatternRes = NULL;
	while(1)
	{
		while(1)
		{
			if(*PatternPtr == '\0')
			{
				Result = (*StringPtr == '\0' ? TRUE : FALSE);
				if (Result || (StringRes == NULL) || (PatternRes == NULL))
				{
					return Result;
				}
				StringPtr = StringRes;
				PatternPtr = PatternRes;
				break;
			}
			else if(*PatternPtr == '*')
			{
				PatternPtr++;
				PatternRes = PatternPtr;
				break;
			}
			else if(*PatternPtr == '?')
			{
				if (*StringPtr == '0')
					return Result;
				StringPtr++;
				PatternPtr++;
			}
			else
			{
				if(*StringPtr == '\0')
					return Result;
				if (*StringPtr != *PatternPtr)
				{
					if ((StringRes == NULL) || (PatternRes == NULL))
						return Result;
					StringPtr = StringRes;
					PatternPtr = PatternRes;
					break;
				}
				else
				{
					StringPtr++;
					PatternPtr++;
				}
			}
		}
		while(1)
		{
			if( *PatternPtr == '\0')
			{
				Result = TRUE;
				return Result;
			}
			else if(*PatternPtr == '*')
			{
				PatternPtr++;
				PatternRes = PatternPtr;
			}
			else if(*PatternPtr == '?')
			{
				if (*StringPtr == '\0')
					return Result;
				StringPtr++;
				PatternPtr++;
			}
			else
			{
				while(1)
				{
					if (*StringPtr == '\0')
						return Result;
					if (*StringPtr == *PatternPtr)
						break;
					StringPtr++;
				}
				StringPtr++;
				StringRes = StringPtr;
				PatternPtr++;
				break;
			}
		}
	}
    
    return Result;
}

//match any character of separator
void __inline__ vSplitString( string strSrc ,vector<string>& vecDest ,const char* szSeparator, BOOL emptyignore = FALSE, unsigned int count = 0x7FFFFFFF )
{
    if( strSrc.empty() )
        return ;
      vecDest.clear();
      string::size_type size_pos = 0;
      string::size_type size_prev_pos = 0;
        
      while( (size_pos = strSrc.find_first_of( szSeparator ,size_pos)) != string::npos)
      {
      	   if(count <=0)
		   		break;
           string strTemp=  strSrc.substr(size_prev_pos , size_pos-size_prev_pos );
           if((strTemp == "")&&(emptyignore))
		   {

		   }
		   else
		   {
           	vecDest.push_back(strTemp);
           	//cout << "string = ["<< strTemp <<"]"<< endl;
		   }
           size_prev_pos =++ size_pos;
		   count--;
      }

	  if((strcmp(&strSrc[size_prev_pos], "") == 0)&&(emptyignore))
	  {

	  }
	  else
	  {
      	vecDest.push_back(&strSrc[size_prev_pos]);
      	//cout << "end string = ["<< &strSrc[size_prev_pos] <<"]"<< endl;
	  }
};

//match whole separator
void __inline__ vSplitStringEx( string strSrc ,vector<string>& vecDest ,const char* szSeparator, BOOL emptyignore = FALSE, unsigned int count = 0x7FFFFFFF )
{
    if( strSrc.empty() )
        return ;
      vecDest.clear();
      string::size_type size_pos = 0;
      string::size_type size_prev_pos = 0;
      
      while( (size_pos = strSrc.find( szSeparator ,size_pos, strlen(szSeparator))) != string::npos)
      {
      	   if(count <=0)
		   		break;
           string strTemp=  strSrc.substr(size_prev_pos , size_pos - size_prev_pos );
           if((strTemp == "")&&(emptyignore))
		   {

		   }
		   else
		   {
           	vecDest.push_back(strTemp);
           	//cout << "string = ["<< strTemp <<"]"<< endl;
		   }
		   size_pos += strlen(szSeparator);
           size_prev_pos = size_pos;
		   count--;
      }

	  if((strcmp(&strSrc[size_prev_pos], "") == 0)&&(emptyignore))
	  {

	  }
	  else
	  {
      	vecDest.push_back(&strSrc[size_prev_pos]);
      	//cout << "end string = ["<< &strSrc[size_prev_pos] <<"]"<< endl;
	  }
};

void __inline__ Toupper( string & str)
{
    for(int i=0;i<str.size();i++)
        str.at(i)=toupper(str.at(i));
}

void __inline__ OutTimeString(unsigned int nTime, string & strTime)
{
	char buf[128];
	time_t clock = nTime;
	struct tm *tm;
	tm = localtime(&clock);

	strftime(buf, sizeof(buf), "%a, %d %h %Y %H:%M:%S %z (%Z)", tm);

	strTime = buf;

}

void __inline__ OutHTTPDateString(unsigned int nTime, string & strTime)
{
	char buf[128];
	time_t clock = nTime;
	struct tm *tm;
	tm = localtime(&clock);

	strftime(buf, sizeof(buf), "%a, %d %h %Y %H:%M:%S %Z", tm);

	strTime = buf;

}

void __inline__ OutSimpleTimeString(unsigned int nTime, string & strTime)
{
	char buf[128];
	time_t clock = nTime;
	struct tm *tm;
	tm = localtime(&clock);

	strftime(buf, sizeof(buf), "%y-%m-%d %H:%M", tm);

	strTime = buf;
	
}


int __inline__ getmonthnumber(const char* mstr)
{
	//"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"

	if(strcasecmp(mstr, "Jan") == 0)
	{
		return 1;
	}
	else if(strcasecmp(mstr, "Feb") == 0)
	{
		return 2;
	}
	else if(strcasecmp(mstr, "Mar") == 0)
	{
		return 3;
	}
	else if(strcasecmp(mstr, "Apr") == 0)
	{
		return 4;
	}
	else if(strcasecmp(mstr, "May") == 0)
	{
		return 5;
	}
	else if(strcasecmp(mstr, "Jun") == 0)
	{
		return 6;
	}
	else if(strcasecmp(mstr, "Jul") == 0)
	{
		return 7;
	}
	else if(strcasecmp(mstr, "Aug") == 0)
	{
		return 8;
	}
	else if(strcasecmp(mstr, "Sep") == 0)
	{
		return 9;
	}
	else if(strcasecmp(mstr, "Oct") == 0)
	{
		return 10;
	}
	else if(strcasecmp(mstr, "Nov") == 0)
	{
		return 11;
	}
	else if(strcasecmp(mstr, "Dec") == 0)
	{
		return 12;
	}
	else
	{
		return atoi(mstr);
	}
}

int __inline__ datecmp(unsigned int y1, unsigned int m1, unsigned int d1,unsigned int y2, unsigned int m2, unsigned int d2)
{
	if(y1 > y2)
		return 1;
	else if (y1 == y2)
	{
		if(m1 > m2)
			return 1;
		else if(m1 == m2)
		{
			if(d1 > d2)
				return 1;
			else if(d1 == d2)
			{
				return 0;
			}
			else
				return -1;
		}
		else
			return -1;
	}
	else
		return -1;
}

unsigned int __inline__ getcharnum(const char* src, char ch)
{
	unsigned int num = 0;
	for(int x = 0; x < strlen(src); x++)
	{
		if(src[x] == ch)
		{
			num++;
		}
	}
	return num;
}

void __inline__ lowercase(const char* szSrc, string &strDst)
{
	strDst.clear();
	int len = strlen(szSrc);
	for(int x = 0; x < len; x++)
	{
		strDst.push_back(LOCH(szSrc[x]));
	}
	strDst.push_back('\0');
}

void __inline__ highercase(const char* szSrc, string &strDst)
{
	strDst.clear();
	int len = strlen(szSrc);
	for(int x = 0; x < len; x++)
	{
		strDst.push_back(HICH(szSrc[x]));
	}
	strDst.push_back('\0');
}

void __inline__ get_extend_name(const char* filename, string & extname)
{
	vector<string> vTmp;
	vSplitString(filename, vTmp, ".");
	if(vTmp.size() > 1)
	{
		extname = vTmp[vTmp.size() - 1];
	}
	else
	{
		extname = "";
	}
}

int __inline__ code_convert(const char *from_charset, const char *to_charset, const char *inbuf, size_t inlen, char *outbuf, size_t outlen)   
{
	iconv_t cd;   
	int rc;   
	char **pin = (char**)&inbuf;   
	char **pout = &outbuf;
	cd = iconv_open(to_charset, from_charset);
	if(cd == (iconv_t)-1)
		return -1;
	memset(outbuf, 0, outlen);

#ifdef _SOLARIS_OS_
	if(iconv(cd, (const char**)pin, &inlen, pout, &outlen) == -1)
	
#else
	if(iconv(cd, pin, &inlen, pout, &outlen) == -1)
#endif /* SOLARIS */
	{
		return -1;
	}
	iconv_close(cd);
	
	return 0;   
}

int __inline__ code_convert_ex(const char *from_charset, const char *to_charset, const char *inbuf, string& strout)   
{
	//printf("%s => %s\n", from_charset, to_charset);
	if(strcasecmp(from_charset, to_charset) == 0)
	{
		strout = inbuf;
		return 0;
	}

	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	if(!outbuf)
		return -1;
	int ret = code_convert(from_charset, to_charset, inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

int __inline__ utf8_to_gb2312_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("UTF-8", "GB2312", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

int __inline__ gb2312_to_utf8_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("GB2312", "UTF-8", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}


int __inline__ gb2312_to_ucs2_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("GB2312", "UCS2", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

int __inline__ UCS2_to_gb2312_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("UCS2", "GB2312", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

int __inline__ utf8_to_ucs2_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("GB2312", "UCS2", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

int __inline__ UCS2_to_utf8_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("UCS2", "GB2312", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

int __inline__ utf8_to_utf7_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	
	int ret = code_convert("UTF-8", "UTF-7", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;	
	}
	delete[] outbuf;
	return ret;
}

int __inline__ utf7_to_utf8_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("UTF-7", "UTF-8", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

int __inline__ utf7_to_gb2312_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("UTF-7", "GB2312", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;	
	}
	delete[] outbuf;
	return ret;
}

int __inline__ gb2312_to_utf7_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("GB2312", "UTF-7", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

//convent the modified UTF-7 to standard UTF-7
int __inline__ utf7_modified_to_standard(const char* inbuf, int inlen, char* outbuf, int outlen)
{
	int x = 0;
	int y = 0;

	BOOL isUTF7 = FALSE;
	while(1)
	{
		if(inbuf[x] == '\0')
		{
			break;
		}
		if(y >= outlen)
		{
			return -1;
		}
		else if(inbuf[x] == '&')
		{
			if(inbuf[x + 1] != '\0')
			{
				if(inbuf[x + 1] == '-')
				{
					outbuf[y] = '&';
					x += 2;
					y++;
				}
				else
				{
					outbuf[y] = '+';
					x++;
					y++;

					isUTF7 = TRUE;
				}
			}
			else /* the last character */
			{
				outbuf[y] = '&';
				x++;
				y++;
			}
		}
		else if(inbuf[x] == ',')
		{
			if(isUTF7)
			{
				outbuf[y] = '/';
				x++;
				y++;
			}
			else
			{
				outbuf[y] = ',';
				x++;
				y++;
			}
		}
		else if(inbuf[x] == '+')
		{
			outbuf[y] = '+';
			outbuf[y + 1] = '-';
			x++;
			y += 2;
		}
		else if(inbuf[x] == '-')
		{
			outbuf[y] = '-';
			x++;
			y++;
			
			if(isUTF7)
			{
				isUTF7 = FALSE;
			}
		}
		else
		{
			outbuf[y] = inbuf[x];
			x++;
			y++;
		}
	}

	outbuf[y] = '\0';

	return 0;
}

int __inline__ utf7_modified_to_standard_ex(const char* inbuf, string& outlen)
{
	int tmplen = strlen(inbuf) * 4 + 1;
	char* tmpbuf = new char[tmplen + 1];
	int ret = utf7_modified_to_standard(inbuf, strlen(inbuf), tmpbuf, tmplen);

	if(ret == 0)
		outlen = tmpbuf;

	delete [] tmpbuf;
	return ret;
}

//convent the standard UTF-7 to modified UTF-7
int __inline__ utf7_standard_to_modified(const char* inbuf, int inlen, char* outbuf, int outlen)
{
	int x = 0;
	int y = 0;

	BOOL isUTF7 = FALSE;
	while(1)
	{
		if(inbuf[x] == '\0')
		{
			break;
		}
		if(y >= outlen)
		{
			return -1;
		}
		else if(inbuf[x] == '+')
		{
			if(inbuf[x + 1] != '\0')
			{
				if(inbuf[x + 1] == '-')
				{
					outbuf[y] = '+';
					x += 2;
					y++;
				}
				else
				{
					outbuf[y] = '&';
					x++;
					y++;

					isUTF7 = TRUE;
				}
			}
			else /* the last character */
			{
				outbuf[y] = '+';
				x++;
				y++;
			}
		}
		else if(inbuf[x] == '/')
		{
			if(isUTF7)
			{
				outbuf[y] = ',';
				x++;
				y++;
			}
			else
			{
				outbuf[y] = '/';
				x++;
				y++;
			}
		}
		else if(inbuf[x] == '&')
		{
			outbuf[y] = '&';
			outbuf[y + 1] = '-';
			x++;
			y += 2;
		}
		else if(inbuf[x] == '-')
		{
			outbuf[y] = '-';
			x++;
			y++;
			
			if(isUTF7)
			{
				isUTF7 = FALSE;
			}
		}
		else
		{
			outbuf[y] = inbuf[x];
			x++;
			y++;
		}
	}

	outbuf[y] = '\0';

	return 0;
}

int __inline__ utf7_standard_to_modified_ex(const char* inbuf, string& outlen)
{
	int tmplen = strlen(inbuf) * 4 + 1;
	char* tmpbuf = new char[tmplen + 1];
	int ret = utf7_standard_to_modified(inbuf, strlen(inbuf), tmpbuf, tmplen);

	if(ret == 0)
		outlen = tmpbuf;

	delete [] tmpbuf;
	return ret;
}


unsigned long long __inline__ atollu(const char* str)
{
	unsigned long long num;
	sscanf(str, "%llu", &num);
	return num;
}

#endif /* _ERISE_GENERAL_H_ */


