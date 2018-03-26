/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _LETTER_H_
#define _LETTER_H_

#include <fstream>
#include <queue>
#include <libmemcached/memcached.h>

#include "storage.h"
#include "util/general.h"
#include "base.h"
#include "tinyxml/tinyxml.h"
#include "mime.h"
#include "calendar.h"
#ifdef _WITH_HDFS_        
    #include "hdfs.h"
#endif /* _WITH_HDFS_ */

using namespace std;

#define POSTFIX_CACHE ".v3.cache"

typedef enum
{
	sRD = 0,
	sWT,
	sWR
}SumMode;

class Block
{
public:
	Block()
	{
		m_beg = m_end = 0;
		m_linenumber = 0;
	}
	virtual ~Block()
	{
	
	}
	
	unsigned int m_beg;
	unsigned int m_end;
	unsigned int m_linenumber;
};

#define DYNAMIC_MMAP_SIZE   4096

class dynamic_mmap_exception
{
public:
    dynamic_mmap_exception()
    {
        fprintf(stderr, "out of scope\n");
    }
    virtual ~dynamic_mmap_exception()
    {
        
    }
};

class dynamic_mmap
{
public:
#ifdef _WITH_HDFS_
    dynamic_mmap(hdfsFS hdfs_conn, hdfsFile ifile);
#else
    dynamic_mmap(ifstream* ifile);
#endif /* _WITH_HDFS_ */
    virtual ~dynamic_mmap();
    
    char& operator[](int i);
private:
#ifdef _WITH_HDFS_
    hdfsFS m_hdfs_conn;
    hdfsFile m_ifile;
#else
    ifstream* m_ifile;
#endif /* _WITH_HDFS_ */
    char m_map_buf[DYNAMIC_MMAP_SIZE];
    unsigned int m_map_beg;
    unsigned int m_map_end;
};

void inline dynamic_mmap_copy(char* dest, dynamic_mmap& src, int from, int size)
{
    for(int i = 0; i < size; i++)
    {
        dest[i] = src[from + i];
    }
}

class BlockSummary : public Block
{
public:
	BlockSummary()
	{
		m_pNewElement = NULL;
		m_pParentElement = NULL;
		m_pSelfElement = NULL;
		m_beg = 0;
		m_end = 0;
	}

	BlockSummary(TiXmlElement * pSelfElement)
	{
		m_pNewElement = NULL;
		m_pParentElement = NULL;
		m_pSelfElement = pSelfElement;
		m_beg = 0;
		m_end = 0;
	}
	
	BlockSummary(unsigned int beg, TiXmlElement * pParentElement)
	{
		m_pNewElement = NULL;
		m_pSelfElement = NULL;
		m_pParentElement = pParentElement;
		m_beg = beg;
		m_end = beg;
	}
	
	virtual ~BlockSummary()
	{

	}
	
	virtual void fieldParse(const char* field) = 0;

	virtual void loadXML() = 0;
	
	TiXmlElement * m_pParentElement;
	TiXmlElement * m_pNewElement;
	TiXmlElement * m_pSelfElement;
};

class LetterHeaderSummary : public BlockSummary
{
public:
	LetterHeaderSummary(TiXmlElement * pSelfElement);
	LetterHeaderSummary(unsigned int beg, TiXmlElement * pParentElement);
	virtual ~LetterHeaderSummary();
	
	virtual void fieldParse(const char* field);
	virtual void loadXML();

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
	
private:
	filedaddr* m_from;
	filedaddr* m_to;
	filedaddr* m_sender;
	filedaddr* m_replyTo;
	filedaddr* m_cc;
	filedaddr* m_bcc;
	string m_subject;
	string m_decodedSubject;
	string m_date;
	struct tm m_tm;
	string m_messageID;
	string m_userAgent;
	string m_inReplyTo;
};

class LetterDataSummary : public BlockSummary
{
public:
	LetterDataSummary(TiXmlElement * pSelfElement);
	LetterDataSummary(unsigned int beg, TiXmlElement * pParentElement);
	virtual ~LetterDataSummary();

	virtual void fieldParse(const char* field);
	virtual void loadXML();

};

class MimeHeaderSummary : public BlockSummary
{
public:
	MimeHeaderSummary(TiXmlElement * pSelfElement);
	MimeHeaderSummary(unsigned int beg, TiXmlElement * pParentElement);
	virtual ~MimeHeaderSummary();

	virtual void fieldParse(const char* field);
	virtual void loadXML();

	filedContentType* m_Content_Type;
	string m_Content_ID;
	string m_Content_Transfer_Encoding;
	string m_Content_Class;
	filedContentDisposition* m_Content_Disposition;
};

class MimeSummary;

class MimeDataSummary : public BlockSummary
{
public:
	MimeDataSummary(TiXmlElement * pSelfElement);
	MimeDataSummary(unsigned int beg, TiXmlElement * pParentElement);
	virtual ~MimeDataSummary();

	virtual void fieldParse(const char* field);
	virtual void loadXML();

	
	void setBoundary(const char* boundary);

	vector<MimeSummary*> m_vMimeSummary;
	
	MimeSummary * m_pNewSummary;

private:
	string m_boundary;
	string m_boundary_beg;
	string m_boundary_end;
};

class MimeSummary : public BlockSummary
{
public:
	MimeSummary(TiXmlElement* pSelfElement);
	MimeSummary(unsigned int beg, TiXmlElement* pParentElement);
	virtual ~MimeSummary();

	MimeHeaderSummary* m_header;
	MimeDataSummary* m_data;
	
	virtual void fieldParse(const char* field);
	virtual void loadXML();

	filedContentType* GetContentType() {return m_header->m_Content_Type; }
	const char* GetContentID() {  return m_header->m_Content_ID.c_str(); }
	const char* GetContentClass() {  return m_header->m_Content_Class.c_str(); }
	const char* GetContentTransferEncoding() { return m_header->m_Content_Transfer_Encoding.c_str(); }
	filedContentDisposition* GetContentDisposition() { return m_header->m_Content_Disposition; }
	
	unsigned int GetSubPartNumber() { return m_data->m_vMimeSummary.size(); }
	MimeSummary* GetPart(unsigned int x) { return m_data->m_vMimeSummary[x]; }
	
private:
	BOOL m_isheader;
};

class LetterSummary : public BlockSummary
{
public:
	LetterSummary(const char* encoding, memcached_st * memcached);
	LetterSummary(const char* encoding, const char* szpath, memcached_st * memcached);
	virtual ~LetterSummary();

	void Parse(const char* buf);
	virtual void fieldParse(const char* field);
	virtual void loadXML();
	void Flush();
	void setPath(const char* szpath);

	LetterHeaderSummary* m_header;
	LetterDataSummary* m_data;
	
	MimeSummary * m_mime;


private:
	void _parse_();
	void _flush_();
	
	TiXmlDocument * m_xml;
	//char* m_xml_text;
	SumMode m_mode;
	string m_xmlpath;
	
	string m_strfield;

	BOOL m_isheader;
    string m_encoding;
	memcached_st * m_memcached;
};

typedef enum
{
	laOut = 0,
	laIn = 1
}lAtt;

typedef struct
{
	string mail_from;
	string mail_to;
	MailType mail_type;
	string mail_uniqueid;
	int mail_dirid;
	unsigned int mail_status;
	unsigned int mail_time;
	unsigned long long user_maxsize;
	int mail_id;
}Letter_Info;

typedef enum
{
    MMAPED = 0,
    CACHED
} MemoryType;

class MailLetter
{
protected:
	lAtt m_att;
	
	dynamic_mmap * m_body;
    
	unsigned int m_size;
	
	unsigned int m_real_size;
	
    string m_eml_cache_full_path;
	string m_eml_full_path;
	string m_emlfile;
	
	string m_uid;
    
#ifdef _WITH_HDFS_        
    hdfsFS m_hdfs_conn;
    hdfsFile m_ofile_in_hdfs;
    hdfsFile m_ifile_in_hdfs;
    hdfsFile m_ifile_line_in_hdfs;
    hdfsFile m_ifile_mmap_in_hdfs;
#else
  	ofstream* m_ofile;
    ifstream* m_ifile;
    ifstream* m_ifile_line;
    ifstream* m_ifile_mmap;
#endif /* _WITH_HDFS_ */


    
    string m_line_text;

	BOOL m_LetterOk;	
	unsigned long long m_maxsize;

	
		
	LetterSummary * m_letterSummary;

	void get_attach_summary(MimeSummary* part, unsigned long& count, unsigned long& sumsize);
	
    string m_private_path;
    string m_encoding;
    
	memcached_st * m_memcached;
    MailStorage* m_mailstg;
    string m_mailbody_fragment;
	
public:
	MailLetter(MailStorage* mailStg, const char* private_path, const char* encoding, memcached_st * memcached, const char* emlfile);
	MailLetter(MailStorage* mailStg, const char* private_path, const char* encoding, memcached_st * memcached, const char* uid, unsigned long long maxsize);
	
	virtual ~MailLetter();
	
	int 	Write(const char* buf, unsigned int len);
    int 	Read(char* buf, unsigned int len);
    int     Seek(unsigned int pos);
    
	void 	Flush();
	dynamic_mmap& Body(int& len);
	int 	Line(string &strline);
	void 	Close();

	void GetAttachSummary(unsigned long& count, unsigned long& sumsize)
	{
		count = 0;
		sumsize = 0;
		get_attach_summary(m_letterSummary->m_mime, count, sumsize);
	}

	LetterSummary * GetSummary()
	{
		return m_letterSummary;
	}
	
	const char* GetEmlFullPath()
	{
		return m_eml_full_path.c_str();
	}

	const char* GetEmlName()
	{
		return m_emlfile.c_str();
	}
	
	unsigned int GetSize()
	{ 
		return m_size; 
	}
	
	void SetOK()
	{
		m_LetterOk = TRUE;
	}

	BOOL isOK()
	{
		return m_LetterOk;
	}
};


#endif /* _LETTER_H_ */

