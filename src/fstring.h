#ifndef _FSTRING_H_
#define _FSTRING_H_

#include "util/general.h"
#include <fstream>
#include "base.h"

using namespace std;

class fpool
{
public:
	fpool(const char* srcfile);
	fpool(const char* buf, int len);
	virtual ~fpool();

	void part(unsigned int beg, unsigned int size, string& strdst);

	char* c_str();
	unsigned int length() { return m_length;}
protected:
	BOOL m_isMap;
	char* m_string;
	int m_fd;
	unsigned m_length;

	string m_srcfile;
};

class fstring
{
public:
	fstring(fpool* pool,unsigned int beg, unsigned int size);
	virtual ~fstring();

	const char* c_str(string& dststr);
	unsigned int length() { return m_length;}
	
protected:
	fpool* m_pool;
	unsigned int m_length;
	unsigned int m_beg;
};

class fbuffer
{
public:
	fbuffer();
	virtual ~fbuffer();

	void bufcat(const char* buf, int len);
	
	fbuffer * operator += (string* str);
	fbuffer * operator += (string& str);
	fbuffer * operator += (const char* str);

	const char * c_buffer();
	
	unsigned int length() { return m_buf_len;}
protected:
	string m_tmpfile;
	ofstream * m_tmpfd;
	int m_mapfd;
	char* m_buf;
	unsigned int m_buf_len;
	unsigned int m_buf_real_len;

};

class fbufseg
{
public:
	int m_byte_beg;
	int m_byte_end;
	fbufseg()
	{
		int m_byte_beg = 0;
		int m_byte_end = 0;
	}

	virtual ~fbufseg() {}

	const char* c_str(const char* buffer, string & strout)
	{
		int len = m_byte_end - m_byte_beg + 1;
		char * tbuf = (char*)malloc(len + 1);
		memcpy(tbuf, buffer + m_byte_beg, len);
		tbuf[len] = '\0';
		strout = tbuf;
		free(tbuf);
		return strout.c_str();
	}
};

#endif /* _FSTRING_H_ */

