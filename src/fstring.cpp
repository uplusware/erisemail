#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include "fstring.h"
#include <pthread.h>
fpool::fpool(const char* srcfile)
{
	m_srcfile = srcfile;
	m_fd = 0;
	m_string = NULL;
	m_fd = open(srcfile, O_RDONLY);
	if(m_fd > 0)
	{
  		struct stat file_stat;
  		fstat( m_fd, &file_stat);
		m_length = file_stat.st_size;
		
		char* pstring = (char*)mmap(NULL, m_length, PROT_READ, MAP_SHARED , m_fd, 0);
		if(pstring)
		{
			if(m_length > MAX_MEMORY_THRESHOLD)
			{
				m_string = pstring;
				m_isMap = TRUE;
			}
			else
			{
				m_string = (char*)malloc(m_length + 1);
				memset(m_string, 0, m_length + 1);
				memcpy(m_string, pstring, m_length);
				munmap(pstring, m_length);
				m_isMap = FALSE;
			}
			
		}
	}
}

fpool::fpool(const char* buf, int len)
{
	m_srcfile = "";
	m_length = len;
	if(len > MAX_MEMORY_THRESHOLD)
	{
		char srcfile[1024];
		srand(time(NULL));
		sprintf(srcfile, "%08x_%08x_%016lx_%08x.fpl", time(NULL), getpid(), pthread_self(), random());
		m_srcfile = srcfile;
		ofstream* srcfd =  new ofstream(m_srcfile.c_str(), ios_base::binary|ios::out|ios::trunc);
		srcfd->write(buf, len);
		delete srcfd;

		m_fd = 0;
		m_string = NULL;
		m_fd = open(m_srcfile.c_str(), O_RDONLY);
		if(m_fd > 0)
		{
	  		struct stat file_stat;
	  		fstat( m_fd, &file_stat);
			m_length = file_stat.st_size;
			
			char* pstring = (char*)mmap(NULL, m_length, PROT_READ, MAP_SHARED , m_fd, 0);
			if(pstring)
			{
				m_string = pstring;
				m_isMap = TRUE;
			}
		}	
	}
	else
	{
		m_string = (char*)malloc(m_length + 1);
		memset(m_string, 0, m_length + 1);
		memcpy(m_string, buf, m_length);
		m_isMap = FALSE;
	}
}

fpool::~fpool()
{
	if(m_isMap)
	{
		if(m_string)
			munmap(m_string, m_length);
	}
	else
	{
		if(m_string)
			free(m_string);
	}
	if(m_fd)
		close(m_fd);
	if(m_srcfile != "")
		unlink(m_srcfile.c_str());
}

void fpool::part(unsigned int beg, unsigned int size, string& strdst)
{
	if(m_string)
	{
		char* tmpbuf = (char*)malloc(size + 1);
		memset(tmpbuf, 0, size + 1);
		memcpy(tmpbuf, m_string + beg, size);
		strdst = tmpbuf;
		free(tmpbuf);
	}
	else
	{
		strdst = "";
	}
}

char* fpool::c_str()
{
	return m_string;
}

fstring::fstring(fpool* pool,unsigned int beg, unsigned int size)
{
	m_pool = pool;
	m_length = size;
	m_beg = beg;
	
}

fstring::~fstring()
{
	
}

const char* fstring::c_str(string& dststr)
{
	m_pool->part(m_beg, m_length, dststr);
	return dststr.c_str();
}

//////////////////////////////////////////////////////////////////////////////////////
//buffer based on filesystem
fbuffer::fbuffer()
{
	m_tmpfd = NULL;
	m_buf = NULL;
	m_buf_len = 0;
	m_mapfd = -1;
	m_buf_real_len = 0;
}

fbuffer::~fbuffer()
{
	if(m_tmpfd)
	{
		if(m_tmpfd->is_open())
			m_tmpfd->close();
		delete m_tmpfd;
	}

	if(m_mapfd > 0)
	{
		munmap(m_buf, m_buf_len);
		close(m_mapfd);
		m_mapfd = -1;
	}
	else
	{
		if(m_buf)
			delete m_buf;
	}
	
	if(m_tmpfile != "")
	{
		if(unlink(m_tmpfile.c_str()) == 0)
		{
			m_tmpfile = "";
		}
	}
}

void fbuffer::bufcat(const char* buf, int len)
{
	if((m_tmpfd) && (m_tmpfd->is_open()))
	{
		m_tmpfd->write(buf, len);
	}
	else
	{
		if((m_buf_len + len) <= MAX_MEMORY_THRESHOLD)
		{
			int elen = len > MEMORY_BLOCK_SIZE ? len : MEMORY_BLOCK_SIZE;
			
			if(!m_buf)
			{
				for(int t = 0; t < 10; t++)
				{
					m_buf = new char[elen + 1];
					if(m_buf)
						break;
				}
				
				if(!m_buf)
					return;
				
				m_buf_real_len = elen;
				memcpy(m_buf, buf, len);
			}
			else
			{
				if((m_buf_len + len) <= m_buf_real_len)
				{
					memcpy(m_buf + m_buf_len, buf, len);
				}
				else
				{					
					char* tbuf = NULL;
					for(int t = 0; t < 10; t++)
					{				
						tbuf = new char[m_buf_real_len + elen + 1];
						if(tbuf)
							break;
					}
					
					if(!tbuf)
						return;

					m_buf_real_len += elen;
					
					memcpy(tbuf, m_buf, m_buf_len);
					memcpy(tbuf + m_buf_len, buf, len);
					delete m_buf;
					m_buf = tbuf;
				}
			}
		}
		else
		{
			char tfile[256];
			sprintf(tfile, "%s/tmp/%08x_%08x_%016lx_%08x.fbf", CMailBase::m_private_path.c_str(), time(NULL), getpid(), pthread_self(), random());
			m_tmpfile = tfile;
			
			m_tmpfd =  new ofstream(m_tmpfile.c_str(), ios_base::binary|ios::out|ios::trunc);
			int wlen = 0;
			while(1)
			{
				if(m_tmpfd->write(m_buf + wlen, (m_buf_len - wlen) > MEMORY_BLOCK_SIZE ? MEMORY_BLOCK_SIZE : (m_buf_len - wlen)) < 0)
					break;
				wlen += (m_buf_len - wlen) > MEMORY_BLOCK_SIZE ? MEMORY_BLOCK_SIZE : (m_buf_len - wlen);
				if(wlen >= m_buf_len)
					break;
			}
			m_tmpfd->write(buf, len);
			
			delete m_buf;
			m_buf = NULL;
			m_buf_real_len = 0;
		}
	}
	m_buf_len += len;
}

const char* fbuffer::c_buffer()
{
	if((m_tmpfd) && (m_tmpfd->is_open()))
	{
		m_tmpfd->flush();
		if(m_mapfd > 0)
		{
			munmap(m_buf, m_buf_len);
			close(m_mapfd);
			m_mapfd = -1;
		}
		m_mapfd = open(m_tmpfile.c_str(), O_RDONLY);
		if(m_mapfd > 0)
		{
	  		struct stat file_stat;
	  		fstat(m_mapfd, &file_stat);
			m_buf_len= file_stat.st_size;
			m_buf = (char*)mmap(NULL, m_buf_len, PROT_READ, MAP_SHARED , m_mapfd, 0);
		}
	}
	return m_buf;
}

fbuffer * fbuffer::operator += (const char* str)
{
	bufcat(str, strlen(str));
	return this;
}

fbuffer * fbuffer::operator += (string* str)
{
	bufcat(str->c_str(), str->length());
	return this;
}

fbuffer * fbuffer::operator += (string& str)
{
	bufcat(str.c_str(), str.length());
	return this;
}


