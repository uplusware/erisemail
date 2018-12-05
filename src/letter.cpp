/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>			 /* For O_* constants */
#include <sys/stat.h> 	   /* For mode constants */
#include <semaphore.h>
#include <fcntl.h>
#include "util/md5.h"
#include "letter.h"

#define MEMCACHED_EML 0x00000001
#define MEMCACHED_XML 0x00000002

////////////////////////////////////////////////////////////////////////////////////////////////////
// dynamic_mmap
#ifdef _WITH_HDFS_
    dynamic_mmap::dynamic_mmap(hdfsFS hdfs_conn, hdfsFile ifile)
    {
        m_hdfs_conn = hdfs_conn;
        m_ifile = ifile;
        m_map_beg = -1;
        m_map_end = -1;
    }

#else
    dynamic_mmap::dynamic_mmap(ifstream* ifile)
    {
        m_ifile = ifile;
        m_map_beg = -1;
        m_map_end = -1;
    }
#endif /* _WITH_HDFS_ */

dynamic_mmap::~dynamic_mmap()
{
    
}

char& dynamic_mmap::operator[](int i)
{
    if(i >= 0 && i >= m_map_beg && i <= m_map_end)
    {
        return m_map_buf[i - m_map_beg];
    }
    else
    {
#ifdef _WITH_HDFS_
        if(m_hdfs_conn && m_ifile)
        {
            hdfsSeek(m_hdfs_conn, m_ifile, i);
            int read_count = hdfsRead(m_hdfs_conn, m_ifile, m_map_buf, DYNAMIC_MMAP_SIZE);
            if(read_count > 0)
            {
                m_map_beg = i;
                m_map_end = i + read_count - 1;
                
                if(i >= 0 && i >= m_map_beg && i <= m_map_end)
                {
                    return m_map_buf[i - m_map_beg];
                }
                else
                {
                    throw new dynamic_mmap_exception();
                }
            }
            else
            {
                m_map_beg = -1;
                m_map_end = -1;
                
                throw new dynamic_mmap_exception();
            }
        }
#else
        if(m_ifile && m_ifile->is_open())
        {
            m_ifile->seekg(i, ios::beg);
            m_ifile->read(m_map_buf, DYNAMIC_MMAP_SIZE);
            int read_count = m_ifile->gcount();
            
            if(read_count > 0)
            {
                m_map_beg = i;
                m_map_end = i + read_count - 1;
                
                if(i >= 0 && i >= m_map_beg && i <= m_map_end)
                {
                    return m_map_buf[i - m_map_beg];
                }
                else
                {
                    m_map_beg = -1;
                    m_map_end = -1;
                
                    throw new dynamic_mmap_exception();
                }
            }
            else
            {
                m_map_beg = -1;
                m_map_end = -1;
                
                throw new dynamic_mmap_exception();
            }
                
        }
#endif /* _WITH_HDFS_ */
        else
            throw new dynamic_mmap_exception();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// MailLetter
MailLetter::MailLetter(MailStorage* mailStg, const char* private_path, const char*  encoding, memcached_st * memcached, const char* uid, unsigned long long maxsize)
{
    m_mailbody_fragment = "";
    
    m_mailstg = mailStg;
	m_maxsize = maxsize;
	
	m_att = laIn;	
	m_LetterOk = FALSE;
	m_eml_full_path = "";
	m_emlfile = "";
    m_eml_cache_full_path = "";
    
	m_body = NULL;
    
#ifdef _WITH_HDFS_    
    m_hdfs_conn = NULL;
    m_ofile_in_hdfs = NULL;
    m_ifile_in_hdfs = NULL;
    m_ifile_line_in_hdfs = NULL;
    m_ifile_mmap_in_hdfs = NULL;
#else
    m_ofile = NULL;
    m_ifile = NULL;
    m_ifile_line = NULL;
    m_ifile_mmap = NULL;
#endif /* _WITH_HDFS_ */

	m_size = 0;
	m_real_size = 0;
	
	m_uid = uid;
	m_memcached = memcached;
    m_private_path = private_path;
    m_encoding = encoding;
    
    
	m_letterSummary = new LetterSummary(m_encoding.c_str(), m_memcached);
}

MailLetter::MailLetter(MailStorage* mailStg, const char* private_path, const char*  encoding, memcached_st * memcached, const char* emlfile)
{	
    m_mailbody_fragment = "";
    
    m_mailstg = mailStg;
    m_private_path = private_path;
    m_encoding = encoding;
    
    
	m_att = laOut;
	
	m_eml_full_path = "";
	m_emlfile = "";
    m_eml_cache_full_path = "";
    
	m_LetterOk = TRUE;
    
	m_body = NULL;
    
#ifdef _WITH_HDFS_            
    m_hdfs_conn = NULL;
    m_ofile_in_hdfs = NULL;
    m_ifile_in_hdfs = NULL;
    m_ifile_line_in_hdfs = NULL;
    m_ifile_mmap_in_hdfs = NULL;
#else
    m_ofile = NULL;
    m_ifile = NULL;
    m_ifile_line = NULL;
    m_ifile_mmap = NULL;
#endif /* _WITH_HDFS_ */

	m_real_size = 0;
	
	m_letterSummary = NULL;
	
	m_size = 0;

	m_emlfile = emlfile;
	m_memcached = memcached;
#ifdef _WITH_HDFS_
    m_eml_full_path = CMailBase::m_hdfs_path;
    m_eml_full_path +="/eml/";
    m_eml_full_path += m_emlfile;
#else
	m_eml_full_path = m_private_path.c_str();
	m_eml_full_path += "/eml/";
	m_eml_full_path += m_emlfile;
    
    //Load the file from the db
    if(access(m_eml_full_path.c_str(), F_OK) != 0)
    {
        m_mailstg->LoadMailBodyToFile(m_emlfile.c_str(), m_eml_full_path.c_str());
    }
#endif /* _WITH_HDFS_ */
	
    m_eml_cache_full_path = m_private_path.c_str();
	m_eml_cache_full_path += "/cache/";
	m_eml_cache_full_path += m_emlfile;
    m_eml_cache_full_path += POSTFIX_CACHE;
    
#ifdef _WITH_HDFS_
    if(ConnectHDFS())
    {
        hdfsFileInfo* file_info = hdfsGetPathInfo(m_hdfs_conn, m_eml_full_path.c_str());
        if(file_info)
        {
            m_size = file_info->mSize;
            hdfsFreeFileInfo(file_info, 1);
        }
    }
#else
	m_ifile_mmap = new ifstream(m_eml_full_path.c_str(), ios_base::binary);
    m_ifile_mmap->seekg (0, ios::end);
    m_size = m_ifile_mmap->tellg();
    
    m_ifile_mmap->seekg (0, ios::beg);
#endif /* _WITH_HDFS_ */
    
	if(access(m_eml_cache_full_path.c_str(), F_OK) != 0)
	{
		m_letterSummary = new LetterSummary(m_encoding.c_str(), m_memcached);
        
        char read_buf[4096];
        int read_count = 0;
        while((read_count = Read(read_buf, 4095)) >= 0)
        {
            if(read_count > 0)
            {
                read_buf[read_count] = '\0';
                m_letterSummary->Parse(read_buf);
            }
        }
        
		m_letterSummary->setPath(m_eml_cache_full_path.c_str());
		
		delete m_letterSummary;
		m_letterSummary = NULL;
	}
	
	m_letterSummary =  new LetterSummary(m_encoding.c_str(), m_eml_cache_full_path.c_str(), m_memcached);
}

MailLetter::~MailLetter()
{
	Close();
	
	if(m_letterSummary)
		delete m_letterSummary;    
}

dynamic_mmap& MailLetter::Body(int &len)
{
    if(!m_body)
    {
#ifdef _WITH_HDFS_
        if(ConnectHDFS() && m_ifile_mmap_in_hdfs == NULL)
        {
            m_ifile_mmap_in_hdfs = hdfsOpenFile(m_hdfs_conn, m_eml_full_path.c_str(), O_RDONLY, 0, 0, 0);
        }
        if(ConnectHDFS() && m_ifile_mmap_in_hdfs)
        {
            m_body = new dynamic_mmap(m_hdfs_conn, m_ifile_mmap_in_hdfs);
        }
#else
        if(!m_ifile_mmap)
        {
             m_ifile_mmap = new ifstream(m_eml_full_path.c_str(), ios_base::binary);
        }
        
        if(m_ifile_mmap && m_ifile_mmap->is_open())
        {
            m_body = new dynamic_mmap(m_ifile_mmap);
        }
#endif /* _WITH_HDFS_ */  
    }
  
    len = m_size;
    
    return *m_body;
}

int MailLetter::Line(string &strline)
{
	if(m_att == laOut)
	{
        strline = "";
#ifdef _WITH_HDFS_    
        if(ConnectHDFS() && m_ifile_in_hdfs == NULL)
        {
            m_ifile_in_hdfs = hdfsOpenFile(m_hdfs_conn, m_eml_full_path.c_str(), O_RDONLY, 0, 0, 0);
            if(!m_ifile_in_hdfs)
                return -1;
        }
        if(m_hdfs_conn && m_ifile_in_hdfs)
        {
            
            std::size_t new_line;
            
            while((new_line = m_line_text.find('\n')) == std::string::npos)
            {                
                char buf[4096];
                int read_count = hdfsRead(m_hdfs_conn, m_ifile_in_hdfs, (void*)buf, 4096);
                if(read_count > 0)
                {
                    buf[read_count] = '\0';
                    
                    m_line_text += buf;
                }
                else
                    return -1;
            }
            
            strline = m_line_text.substr(0, new_line + 1);
            m_line_text = m_line_text.substr(new_line + 1);

            strtrim(strline);
            
            return 0;
        }
#else
		if(!m_ifile_line)
		{
			 m_ifile_line = new ifstream(m_eml_full_path.c_str(), ios_base::binary);
		}
		
		if(m_ifile_line && m_ifile_line->is_open())
		{
            std::size_t new_line;
            
            while((new_line = m_line_text.find('\n')) == std::string::npos)
            {
                if(m_ifile_line->eof())
                    return -1;
                
                char buf[4096];
                m_ifile_line->read(buf, 4095);
                
                int read_count = m_ifile_line->gcount();
                
                if(read_count > 0)
                {
                    buf[read_count] = '\0';
                    
                    m_line_text += buf;
                }
                else
                    return -1;
            }
            
            strline = m_line_text.substr(0, new_line + 1);
            m_line_text = m_line_text.substr(new_line + 1);

            strtrim(strline);
            
            return 0;
        
		}
		else
        {
			return -1;
        }
#endif /* _WITH_HDFS_ */
		
	}
	else
		return -1;
}

int MailLetter::Read(char* buf, unsigned int len)
{
    if(m_att == laOut)
	{
#ifdef _WITH_HDFS_
        if(ConnectHDFS() && m_ifile_in_hdfs == NULL)
        {
            m_ifile_in_hdfs = hdfsOpenFile(m_hdfs_conn, m_eml_full_path.c_str(), O_RDONLY, 0, 0, 0);
            if(!m_ifile_in_hdfs)
                return -1;
        }
        if(ConnectHDFS() && m_ifile_in_hdfs)
        {
            int read_count = hdfsRead(m_hdfs_conn, m_ifile_in_hdfs, (void*)buf, len);
            return read_count > 0 ? read_count : -1;
        }
        else
            return -1;
#else
		if(!m_ifile)
		{
			 m_ifile = new ifstream(m_eml_full_path.c_str(), ios_base::binary);
		}
		
		if(m_ifile && m_ifile->is_open())
		{
            if(m_ifile->eof())
                return -1;
			
            m_ifile->read(buf, len);
            
			return m_ifile->gcount();
		}
		else
			return -1;
#endif /* _WITH_HDFS_ */
	}
	else
		return -1;
}

int MailLetter::Seek(unsigned int pos)
{
    if(m_att == laOut)
	{
        if(m_size < pos)
            return -1;
        
#ifdef _WITH_HDFS_    
        if(ConnectHDFS() && m_ifile_in_hdfs == NULL)
        {
            m_ifile_in_hdfs = hdfsOpenFile(m_hdfs_conn, m_eml_full_path.c_str(), O_RDONLY, 0, 0, 0);
            if(!m_ifile_in_hdfs)
                return -1;
        }
        if(ConnectHDFS() && m_ifile_in_hdfs)
            return hdfsSeek(m_hdfs_conn, m_ifile_in_hdfs, pos);
        else
            return -1;
#else
		if(!m_ifile)
		{
			 m_ifile = new ifstream(m_eml_full_path.c_str(), ios_base::binary);
		}
		
		if(m_ifile && m_ifile->is_open())
		{            
            m_ifile->seekg(pos, ios::beg);
            return 0;
		}
		else
			return -1;
#endif /* _WITH_HDFS_ */
	}
	else
		return -1;
}

int MailLetter::Write(const char* buf, unsigned int len)
{	
	if(m_att == laOut)
	{
		return -1;
	}
    
    if(m_size + len > m_maxsize)
    {
        return -1;
    }
#ifdef _WITH_HDFS_        
    char emlfile[512];
    sprintf(emlfile, "%s.eml", m_uid.c_str());

    m_emlfile = emlfile;
    
    m_eml_full_path = CMailBase::m_hdfs_path;
    m_eml_full_path +="/eml/";
    m_eml_full_path += m_emlfile;
    
    m_eml_cache_full_path = m_private_path;
    m_eml_cache_full_path += "/cache/";
    m_eml_cache_full_path += m_emlfile;
    m_eml_cache_full_path += POSTFIX_CACHE;
    
    if(ConnectHDFS() && m_ofile_in_hdfs == NULL)
    {
        m_ofile_in_hdfs = hdfsOpenFile(m_hdfs_conn, m_eml_full_path.c_str(), O_WRONLY |O_CREAT, 0, 0, 0);
    }
    
    if(ConnectHDFS() && m_ofile_in_hdfs)
    {
        int write_len = hdfsWrite(m_hdfs_conn, m_ofile_in_hdfs, (void*)buf, len);
        if(write_len < 0)
            return -1;
        
        m_size += len;
        
        //generate the summary
        char* tbuf = new char[len + 1];
        memcpy(tbuf, buf, len);
        tbuf[len] = '\0';
        m_letterSummary->Parse(tbuf);
        delete tbuf;
        return 0;
    }
    else
    {
        return -1;
    }
    
#else
	if(!m_ofile)
	{
		char emlfile[512];
		sprintf(emlfile, "%s.eml", m_uid.c_str());

		m_emlfile = emlfile;
		
		m_eml_full_path = m_private_path;
		m_eml_full_path +="/eml/";
		m_eml_full_path += m_emlfile;
		
        m_eml_cache_full_path = m_private_path;
		m_eml_cache_full_path += "/cache/";
		m_eml_cache_full_path += m_emlfile;
        m_eml_cache_full_path += POSTFIX_CACHE;
        
		m_ofile = new ofstream(m_eml_full_path.c_str(), ios_base::binary|ios::out|ios::trunc);			
		if(m_ofile)
		{		
			if(!m_ofile->is_open())
			{
				return -1;
			}
		}
	}
			
	if(m_ofile)
	{
		if(m_ofile->is_open())
		{
            m_mailbody_fragment += buf;
            if(m_mailbody_fragment.length() >= 512*1024) //64k
            {
                if(m_mailstg->SaveMailBodyToDB(m_emlfile.c_str(), m_mailbody_fragment.c_str()) < 0)
                    return -1;
                m_mailbody_fragment = "";
            }
            
			//if(m_ofile->write(buf, len))
			//{
                m_ofile->write(buf, len);
                
				//generate the summary
				char* tbuf = new char[len + 1];
				memcpy(tbuf, buf, len);
				tbuf[len] = '\0';
				m_letterSummary->Parse(tbuf);
				delete tbuf;
				
				m_size += len;
				return 0;
			//}
			//else
			//{
			//	return -1;
			//}
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
#endif /* _WITH_HDFS_ */
}

void MailLetter::Flush()
{
	if(m_att == laOut)
	{
		return;
	}
#ifdef _WITH_HDFS_   
    if(ConnectHDFS() && m_ofile_in_hdfs)
    {
        hdfsFlush(m_hdfs_conn, m_ofile_in_hdfs);
        m_letterSummary->Flush();
    }
#else
	if(m_ofile)
	{
		if(m_ofile->is_open())
		{
			m_ofile->flush();
			m_letterSummary->Flush();
		}
	}
#endif /* _WITH_HDFS_ */
}

void MailLetter::Close()
{	
	if(m_att == laIn)
	{
#ifdef _WITH_HDFS_ 
        if(m_hdfs_conn && m_ofile_in_hdfs)
        {
            hdfsCloseFile(m_hdfs_conn, m_ofile_in_hdfs);
            m_ofile_in_hdfs = NULL;
            
            if(m_LetterOk == TRUE)
            {
                m_letterSummary->setPath(m_eml_cache_full_path.c_str());
            }
        }
#else
        if(m_mailbody_fragment.length() > 0)
        {
            m_mailstg->SaveMailBodyToDB(m_emlfile.c_str(), m_mailbody_fragment.c_str());
            m_mailbody_fragment = "";
        }
		if(m_ofile)
		{
			if(m_ofile->is_open())
			{
				m_ofile->close();
				if(m_LetterOk == TRUE)
				{
					m_letterSummary->setPath(m_eml_cache_full_path.c_str());
				}
			}
		}
#endif /* _WITH_HDFS_ */
	}
    
#ifdef _WITH_HDFS_ 
    if(m_hdfs_conn)
    {
        if(m_ofile_in_hdfs)
            hdfsCloseFile(m_hdfs_conn, m_ofile_in_hdfs);
        
        if(m_ifile_in_hdfs)
            hdfsCloseFile(m_hdfs_conn, m_ifile_in_hdfs);
        
        if(m_ifile_line_in_hdfs)
            hdfsCloseFile(m_hdfs_conn, m_ifile_line_in_hdfs);
        
        if(m_ifile_mmap_in_hdfs)
            hdfsCloseFile(m_hdfs_conn, m_ifile_mmap_in_hdfs);
    
        m_ofile_in_hdfs = NULL;
        m_ifile_in_hdfs = NULL;
        m_ifile_line_in_hdfs = NULL;
        m_ifile_mmap_in_hdfs = NULL;
        
        hdfsDisconnect(m_hdfs_conn);
            
        m_hdfs_conn = NULL;
    }
#else
	if(m_ofile)
	{
		if(m_ofile->is_open())
			m_ofile->close();
		delete m_ofile;
	}
	m_ofile = NULL;
    
    if(m_ifile)
	{
		if(m_ifile->is_open())
			m_ifile->close();
		delete m_ifile;
	}
	m_ifile = NULL;
    
    if(m_ifile_line)
	{
		if(m_ifile_line->is_open())
			m_ifile_line->close();
		delete m_ifile_line;
	}
	m_ifile_line = NULL;
    
    if(m_ifile_mmap)
	{
		if(m_ifile_mmap->is_open())
			m_ifile_mmap->close();
		delete m_ifile_mmap;
	}
	m_ifile_mmap = NULL;    
#endif /* _WITH_HDFS_ */

    if(m_body)
        delete m_body;
    
    m_body = NULL;
    
	if(m_LetterOk == FALSE && m_att == laIn) 
	{
		if(m_eml_full_path != "")
		{
			unlink(m_eml_full_path.c_str());
		}
		m_eml_full_path = "";
	}
}

void MailLetter::get_attach_summary(MimeSummary* part, unsigned long& count, unsigned long& sumsize)
{
	//for solaris compatibility
	std::queue<MimeSummary*> AttachQueue;
	
	while(part != NULL)
	{
		if(part->GetSubPartNumber() == 0)
		{
			if(part->GetContentType() != NULL)
			{
				if(strcasecmp(part->GetContentType()->m_type1.c_str(), "multipart") != 0 && strcasecmp(part->GetContentType()->m_type2.c_str(), "mixed") != 0)
				{
					if(part->GetContentType()->m_filename != "" || ( part->GetContentDisposition() && part->GetContentDisposition()->m_filename != ""))
					{
						count++;
						sumsize += (part->m_data->m_end - part->m_data->m_beg)*3/4;
					}
				}
			}
		}
		else
		{
			for(int a = 0; a < part->GetSubPartNumber(); a++)
			{
				if(part->GetPart(a) != NULL)
					AttachQueue.push(part->GetPart(a));
			}
		}
		if(AttachQueue.empty())
		{
			part = NULL;
		}
		else
		{
			part = AttachQueue.front();
			AttachQueue.pop();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////
// LetterSummary

LetterSummary::LetterSummary(const char* encoding, memcached_st * memcached) : BlockSummary(0, NULL)
{
	m_memcached = memcached;
	m_xml = NULL;
	m_isheader = TRUE;
	m_strfield = "";
	m_xmlpath = "";
	m_xml = new TiXmlDocument();
    
    m_encoding = encoding;
    
	string strXMLInit = "<?xml version='1.0' encoding='" + m_encoding + "'?><letter></letter>";

	m_xml->Parse(strXMLInit.c_str());
	
	m_mode = sWT;

	m_header = NULL;
	m_data = NULL;
	m_mime = NULL;
	m_pParentElement = m_xml->RootElement();
}

LetterSummary::LetterSummary(const char* encoding, const char* szpath, memcached_st * memcached) 
{
    m_encoding = encoding;
    
	m_memcached = memcached;
	m_isheader = TRUE;
	m_xmlpath = szpath;
	m_strfield = "";
	m_mode = sRD; 

	m_mime = NULL;

	m_xml = NULL;
	
	m_header = NULL;
	m_data = NULL;
	m_xml = new TiXmlDocument();
	
	loadXML();
}

LetterSummary::~LetterSummary()
{	
	if(m_mode == sWT)
	{
		_flush_();
	}

	if(m_header && !m_data)
	{
		m_data = new LetterDataSummary(m_header->m_end + 1, m_pParentElement);
	}
	
	if(m_header)
		delete m_header;

	if(m_data)
		delete m_data;

	if(m_mime)
		delete m_mime;
	
	if(m_mode == sWT)
	{
		if(m_xml)
		{
			if(m_xmlpath != "")
				m_xml->SaveFile(m_xmlpath.c_str());
		}
	}
	
	if(m_xml)
		delete m_xml;
}

void LetterSummary::setPath(const char* szpath)
{
	m_xmlpath = szpath;
}

void LetterSummary::Parse(const char* buf)
{ 
	if(m_mode != sWT && m_mode != sWR)
		return;
	
	m_strfield += buf;
	_parse_();
}

void LetterSummary::_parse_()
{
	while(1)
	{
		char* strfield;
		const char * ptmp = m_strfield.c_str();
		unsigned int x = 0;
		for(x = 0; x < m_strfield.length(); x++)
		{
			if((m_strfield[x] == '\n') && (x < m_strfield.length() - 1) && (x < m_strfield.length() - 1) && (m_strfield[x+1] != '\t') && (m_strfield[x+1] != ' '))
			{
				strfield = new char[x + 2];
				memcpy(strfield, ptmp, x + 1);
				strfield[x + 1] = '\0';
				fieldParse(strfield);
				delete strfield;
				break;
			}

		}
		
		if(x < m_strfield.length() - 1)
		{
			m_strfield = &ptmp[x + 1];
			//_parse_();
		}
		else
		{
			break;
		}
	}
}

void LetterSummary::Flush()
{
	_flush_();
}

void LetterSummary::_flush_()
{
	if(m_mode != sWT && m_mode != sWR)
		return;
	
	if(m_strfield != "")
		fieldParse(m_strfield.c_str());

	m_strfield = "";
}

void LetterSummary::fieldParse(const char* field)
{
	if(m_mode != sWT && m_mode != sWR)
		return;
	
	if(m_isheader == TRUE)
	{
		if(strcmp(field, "\r\n") == 0 || strcmp(field, "\n") == 0)
		{
			m_isheader = FALSE;			
		}
		
		if(!m_header)
			m_header = new LetterHeaderSummary(0, m_pParentElement);
		m_header->fieldParse(field);
	}
	else
	{
		if(!m_data)
			m_data = new LetterDataSummary(m_end, m_pParentElement);

		m_data->fieldParse(field);
	}
	
	
	if(!m_mime)
		m_mime = new MimeSummary(0, m_pParentElement);
	
	if(m_mime)
		m_mime->fieldParse(field);

	m_linenumber += getcharnum(field, '\n');
	m_end += strlen(field);
}

void LetterSummary::loadXML()
{
	if(m_mode != sRD && m_mode != sWR)
	  return;
	
	memcached_return memc_rc;
	size_t memc_value_length;
	uint32_t memc_flags = 0;
	char szMD5dst[36];
    char* xml_text = NULL;
    if(m_memcached)
    {
        ietf::MD5_CTX_OBJ context;
        context.MD5Update ((unsigned char*)m_xmlpath.c_str(), m_xmlpath.length());
        unsigned char digest[16];
        context.MD5Final (digest);
        sprintf(szMD5dst,
            "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
            digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7],
            digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15]);
        szMD5dst[32] = 'x';
        szMD5dst[33] = 'm';
        szMD5dst[34] = 'l';
        szMD5dst[35] = '\0';
        xml_text = memcached_get(m_memcached, szMD5dst, 35, &memc_value_length, &memc_flags, &memc_rc);
        
            
    }
	if (m_memcached && xml_text && memc_rc == MEMCACHED_SUCCESS && memc_value_length > 0 && m_xml->LoadString(xml_text, memc_value_length))
	{
        //printf("get: %s %d\n%s\n", m_xmlpath.c_str(), memc_value_length, xml_text);
		if(xml_text)
		    free(xml_text);
	}
	else
	{
        if(xml_text)
		    free(xml_text);
        
		m_xml->LoadFile(m_xmlpath.c_str());
		if(m_memcached)
        {
            TiXmlPrinter xml_printer;
            m_xml->Accept( &xml_printer );
            
            memc_rc = memcached_set(m_memcached, szMD5dst, 35, xml_printer.CStr(), xml_printer.Size(), (time_t)3600, (uint32_t)memc_flags);
            if(memc_rc != MEMCACHED_SUCCESS)
            {
                fprintf(stderr, "memcached_set: %s %d\n", m_xmlpath.c_str(), xml_printer.Size());
            }
        }
  
	}
	if(m_xml && m_xml->RootElement())
	{
		TiXmlNode * pLetterHeaderNode = m_xml->RootElement()->FirstChild("letterHeader");

		if(pLetterHeaderNode && pLetterHeaderNode->ToElement())
		{
			if(!m_header)
			{
				m_header = new LetterHeaderSummary(pLetterHeaderNode->ToElement());
				if(m_header)
					m_header->loadXML();
			}
		}

		TiXmlNode * pLetterDataNode = m_xml->RootElement()->FirstChild("letterData");
		if(pLetterDataNode && pLetterDataNode->ToElement())
		{
			if(!m_data)
			{
				m_data = new LetterDataSummary(pLetterDataNode->ToElement());
				if(m_data)
					m_data->loadXML();
			}
		}	
		
		TiXmlNode * pRootMIMENode = m_xml->RootElement()->FirstChild("MIME");
		if(pRootMIMENode && pRootMIMENode->ToElement())
		{
			if(!m_mime)
			{
				m_mime = new MimeSummary(pRootMIMENode->ToElement());
				if(m_mime)
					m_mime->loadXML();
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////
// LetterHeaderSummary
LetterHeaderSummary::LetterHeaderSummary(TiXmlElement * pSelfElement) : BlockSummary(pSelfElement)
{
	m_from = NULL;
	m_to = NULL;
	m_sender = NULL;
	m_replyTo = NULL;
	m_cc = NULL;
	m_bcc = NULL;
}

LetterHeaderSummary::LetterHeaderSummary(unsigned int beg, TiXmlElement * pParentElement) : BlockSummary(beg, pParentElement)
{
	m_from = NULL;
	m_to = NULL;
	m_sender = NULL;
	m_replyTo = NULL;
	m_cc = NULL;
	m_bcc = NULL;
	
	if(!m_pNewElement)
		m_pNewElement = new TiXmlElement("letterHeader");
}

LetterHeaderSummary::~LetterHeaderSummary()
{
	if(m_pNewElement)
	{
		m_pNewElement->SetAttribute("beg", (int)m_beg);
		m_pNewElement->SetAttribute("end", (int)m_end);
		m_pNewElement->SetAttribute("linenumber", (int)m_linenumber);
		m_pParentElement->InsertEndChild(*m_pNewElement);
		delete m_pNewElement;
		m_pNewElement = NULL;
	}

	if(m_from) delete m_from;
	if(m_to) delete m_to;
	if(m_sender) delete m_sender;
	if(m_replyTo) delete m_replyTo;
	if(m_cc) delete m_cc;
	if(m_bcc) delete m_bcc;
}

void LetterHeaderSummary::loadXML()
{
	m_pSelfElement->Attribute("beg", (int*)&m_beg);
	m_pSelfElement->Attribute("end", (int*)&m_end);
	m_pSelfElement->Attribute("linenumber", (int*)&m_linenumber);
	
	TiXmlNode * pMessageIDNode = m_pSelfElement->FirstChild("messageID");
	if(pMessageIDNode && pMessageIDNode->ToElement())
	{
		m_messageID = pMessageIDNode->ToElement()->Attribute("value");
	}
	
	TiXmlNode * pDateNode = m_pSelfElement->FirstChild("date");
	if(pDateNode && pDateNode->ToElement())
	{
		m_date = pDateNode->ToElement()->Attribute("string");
		pDateNode->ToElement()->Attribute("year", (int*)&m_tm.tm_year);
		pDateNode->ToElement()->Attribute("mon", (int*)&m_tm.tm_mon);
		pDateNode->ToElement()->Attribute("mday", (int*)&m_tm.tm_mday);
		pDateNode->ToElement()->Attribute("hour", (int*)&m_tm.tm_hour);
		pDateNode->ToElement()->Attribute("min", (int*)&m_tm.tm_min);
		pDateNode->ToElement()->Attribute("sec", (int*)&m_tm.tm_sec);
	}
	
	TiXmlNode * pFromNode = m_pSelfElement->FirstChild("from");

	if(pFromNode && pFromNode->ToElement())
	{
		m_from = new filedaddr();

		m_from->m_strfiled = pFromNode->ToElement()->Attribute("field") == NULL ? "" : pFromNode->ToElement()->Attribute("field");
		
		TiXmlNode * pAddrNode = pFromNode->ToElement()->FirstChild();
		while(pAddrNode)
		{
			if(pAddrNode->ToElement())
			{
				mailaddr maddr;
				maddr.pName = pAddrNode->ToElement()->Attribute("pName") == NULL ? "" : pAddrNode->ToElement()->Attribute("pName");
				maddr.mName = pAddrNode->ToElement()->Attribute("mName") == NULL ? "" : pAddrNode->ToElement()->Attribute("mName");
				maddr.dName = pAddrNode->ToElement()->Attribute("dName") == NULL ? "" : pAddrNode->ToElement()->Attribute("dName");

				m_from->m_addrs.push_back(maddr);
			}
			pAddrNode = pAddrNode->NextSibling();
		}
	}

	TiXmlNode * pToNode = m_pSelfElement->FirstChild("to");
	if(pToNode && pToNode->ToElement())
	{
		m_to = new filedaddr();

		m_to->m_strfiled = pToNode->ToElement()->Attribute("field");
		
		TiXmlNode * pAddrNode = pToNode->ToElement()->FirstChild();
		while(pAddrNode)
		{
			if(pAddrNode->ToElement())
			{
				mailaddr maddr;
				maddr.pName = pAddrNode->ToElement()->Attribute("pName") == NULL ? "" : pAddrNode->ToElement()->Attribute("pName");
				maddr.mName = pAddrNode->ToElement()->Attribute("mName") == NULL ? "" : pAddrNode->ToElement()->Attribute("mName");
				maddr.dName = pAddrNode->ToElement()->Attribute("dName") == NULL ? "" : pAddrNode->ToElement()->Attribute("dName");

				m_to->m_addrs.push_back(maddr);
			}
			pAddrNode = pAddrNode->NextSibling();
		}
	}

	TiXmlNode * pCcNode = m_pSelfElement->FirstChild("cc");
	if(pCcNode && pCcNode->ToElement())
	{
		m_cc = new filedaddr();

		m_cc->m_strfiled = pCcNode->ToElement()->Attribute("field");
		
		TiXmlNode * pAddrNode = pCcNode->ToElement()->FirstChild();
		while(pAddrNode)
		{
			if(pAddrNode->ToElement())
			{
				mailaddr maddr;
				maddr.pName = pAddrNode->ToElement()->Attribute("pName") == NULL ? "" : pAddrNode->ToElement()->Attribute("pName");
				maddr.mName = pAddrNode->ToElement()->Attribute("mName") == NULL ? "" : pAddrNode->ToElement()->Attribute("mName");
				maddr.dName = pAddrNode->ToElement()->Attribute("dName") == NULL ? "" : pAddrNode->ToElement()->Attribute("dName");

				m_cc->m_addrs.push_back(maddr);
			}
			pAddrNode = pAddrNode->NextSibling();
		}
	}

	TiXmlNode * pBccNode = m_pSelfElement->FirstChild("bcc");
	if(pBccNode && pBccNode->ToElement())
	{
		m_bcc = new filedaddr();

		m_bcc->m_strfiled = pBccNode->ToElement()->Attribute("field");
		
		TiXmlNode * pAddrNode = pBccNode->ToElement()->FirstChild();
		while(pAddrNode)
		{
			if(pAddrNode->ToElement())
			{
				mailaddr maddr;
				maddr.pName = pAddrNode->ToElement()->Attribute("pName") == NULL ? "" : pAddrNode->ToElement()->Attribute("pName");
				maddr.mName = pAddrNode->ToElement()->Attribute("mName") == NULL ? "" : pAddrNode->ToElement()->Attribute("mName");
				maddr.dName = pAddrNode->ToElement()->Attribute("dName") == NULL ? "" : pAddrNode->ToElement()->Attribute("dName");

				m_bcc->m_addrs.push_back(maddr);
			}
			pAddrNode = pAddrNode->NextSibling();
		}
	}
	
	TiXmlNode * pSenderNode = m_pSelfElement->FirstChild("sender");
	if(pSenderNode && pSenderNode->ToElement())
	{
		m_sender = new filedaddr();

		m_sender->m_strfiled = pSenderNode->ToElement()->Attribute("field");
		
		TiXmlNode * pAddrNode = pSenderNode->ToElement()->FirstChild();
		while(pAddrNode)
		{
			if(pAddrNode->ToElement())
			{
				mailaddr maddr;
				maddr.pName = pAddrNode->ToElement()->Attribute("pName") == NULL ? "" : pAddrNode->ToElement()->Attribute("pName");
				maddr.mName = pAddrNode->ToElement()->Attribute("mName") == NULL ? "" : pAddrNode->ToElement()->Attribute("mName");
				maddr.dName = pAddrNode->ToElement()->Attribute("dName") == NULL ? "" : pAddrNode->ToElement()->Attribute("dName");

				m_sender->m_addrs.push_back(maddr);
			}
			pAddrNode = pAddrNode->NextSibling();
		}
	}

	TiXmlNode * pReplyToNode = m_pSelfElement->FirstChild("replyTo");
	if(pReplyToNode && pReplyToNode->ToElement())
	{
		m_replyTo= new filedaddr();

		m_replyTo->m_strfiled = pReplyToNode->ToElement()->Attribute("field");
		
		TiXmlNode * pAddrNode = pReplyToNode->ToElement()->FirstChild();
		while(pAddrNode)
		{
			if(pAddrNode->ToElement())
			{
				mailaddr maddr;
				maddr.pName = pAddrNode->ToElement()->Attribute("pName") == NULL ? "" : pAddrNode->ToElement()->Attribute("pName");
				maddr.mName = pAddrNode->ToElement()->Attribute("mName") == NULL ? "" : pAddrNode->ToElement()->Attribute("mName");
				maddr.dName = pAddrNode->ToElement()->Attribute("dName") == NULL ? "" : pAddrNode->ToElement()->Attribute("dName");

				m_replyTo->m_addrs.push_back(maddr);
			}
			pAddrNode = pAddrNode->NextSibling();
		}
	}

	TiXmlNode * pInReplyToNode = m_pSelfElement->FirstChild("inReplyTo");
	if(pInReplyToNode && pInReplyToNode->ToElement())
	{
		m_inReplyTo = pInReplyToNode->ToElement()->Attribute("value") == NULL ? "" : pInReplyToNode->ToElement()->Attribute("value") ;
	}

	TiXmlNode * pSubjectNode = m_pSelfElement->FirstChild("subject");
	if(pSubjectNode && pSubjectNode->ToElement())
	{
		m_subject = pSubjectNode->ToElement()->Attribute("original") == NULL ? "" : pSubjectNode->ToElement()->Attribute("original");
		m_decodedSubject = pSubjectNode->ToElement()->Attribute("decoded") == NULL ? "" : pSubjectNode->ToElement()->Attribute("decoded");
	}
	
}

void LetterHeaderSummary::fieldParse(const char* field)
{
	if(strncasecmp(field, "From:", sizeof("From:") - 1) == 0)
	{
		m_from = new filedaddr(field);
		TiXmlElement* pNewFromElement = new TiXmlElement("from");

		pNewFromElement->SetAttribute("field", m_from->m_strfiled.c_str());
		
		for(int i = 0; i < m_from->m_addrs.size(); i++)
		{
			TiXmlElement* pNewAddrElement = new TiXmlElement("addr");
			pNewAddrElement->SetAttribute("pName",m_from->m_addrs[i].pName.c_str());
			pNewAddrElement->SetAttribute("mName",m_from->m_addrs[i].mName.c_str());
			pNewAddrElement->SetAttribute("dName",m_from->m_addrs[i].dName.c_str());
			
			pNewFromElement->InsertEndChild(*pNewAddrElement);
			delete pNewAddrElement;
		}
		m_pNewElement->InsertEndChild(*pNewFromElement);
		delete pNewFromElement;

	}
	else if(strncasecmp(field, "To:", sizeof("To:") - 1) == 0)
	{
		m_to = new filedaddr(field);

		TiXmlElement* pNewToElement = new TiXmlElement("to");
		pNewToElement->SetAttribute("field", m_to->m_strfiled.c_str());
		for(int i = 0; i < m_to->m_addrs.size(); i++)
		{
			TiXmlElement* pNewAddrElement = new TiXmlElement("addr");
			pNewAddrElement->SetAttribute("pName",m_to->m_addrs[i].pName.c_str());
			pNewAddrElement->SetAttribute("mName",m_to->m_addrs[i].mName.c_str());
			pNewAddrElement->SetAttribute("dName",m_to->m_addrs[i].dName.c_str());

			pNewToElement->InsertEndChild(*pNewAddrElement);
			delete pNewAddrElement;
		}
		m_pNewElement->InsertEndChild(*pNewToElement);
		delete pNewToElement;
		
	}
	else if(strncasecmp(field, "Sender:", sizeof("Sender:") - 1) == 0)
	{
		m_sender= new filedaddr(field);

		TiXmlElement* pNewSenderElement = new TiXmlElement("sender");
		pNewSenderElement->SetAttribute("field", m_sender->m_strfiled.c_str());
		for(int i = 0; i < m_sender->m_addrs.size(); i++)
		{
			TiXmlElement* pNewAddrElement = new TiXmlElement("addr");
			pNewAddrElement->SetAttribute("pName",m_sender->m_addrs[i].pName.c_str());
			pNewAddrElement->SetAttribute("mName",m_sender->m_addrs[i].mName.c_str());
			pNewAddrElement->SetAttribute("dName",m_sender->m_addrs[i].dName.c_str());

			pNewSenderElement->InsertEndChild(*pNewAddrElement);
			delete pNewAddrElement;
		}
		m_pNewElement->InsertEndChild(*pNewSenderElement);
		delete pNewSenderElement;
		
	}
	else if(strncasecmp(field, "Reply-To:", sizeof("Reply-To:") - 1) == 0)
	{
		m_replyTo= new filedaddr(field);
		TiXmlElement* pNewReplyToElement = new TiXmlElement("replyTo");
		pNewReplyToElement->SetAttribute("field", m_replyTo->m_strfiled.c_str());
		for(int i = 0; i < m_replyTo->m_addrs.size(); i++)
		{
			TiXmlElement* pNewAddrElement = new TiXmlElement("addr");
			pNewAddrElement->SetAttribute("pName",m_replyTo->m_addrs[i].pName.c_str());
			pNewAddrElement->SetAttribute("mName",m_replyTo->m_addrs[i].mName.c_str());
			pNewAddrElement->SetAttribute("dName",m_replyTo->m_addrs[i].dName.c_str());

			pNewReplyToElement->InsertEndChild(*pNewAddrElement);
			delete pNewAddrElement;
		}
		m_pNewElement->InsertEndChild(*pNewReplyToElement);
		delete pNewReplyToElement;
		
	}
	else if(strncasecmp(field, "Cc:", sizeof("Cc:") - 1) == 0)
	{
		m_cc = new filedaddr(field);

		TiXmlElement* pNewCcElement = new TiXmlElement("cc");
		pNewCcElement->SetAttribute("field", m_cc->m_strfiled.c_str());
		for(int i = 0; i < m_cc->m_addrs.size(); i++)
		{
			TiXmlElement* pNewAddrElement = new TiXmlElement("addr");
			pNewAddrElement->SetAttribute("pName",m_cc->m_addrs[i].pName.c_str());
			pNewAddrElement->SetAttribute("mName",m_cc->m_addrs[i].mName.c_str());
			pNewAddrElement->SetAttribute("dName",m_cc->m_addrs[i].dName.c_str());

			pNewCcElement->InsertEndChild(*pNewAddrElement);
			delete pNewAddrElement;
		}
		m_pNewElement->InsertEndChild(*pNewCcElement);
		delete pNewCcElement;
		
	}
	else if(strncasecmp(field, "BCC:", sizeof("BCC:") - 1) == 0)
	{
		m_bcc = new filedaddr(field);

		TiXmlElement* pNewBccElement = new TiXmlElement("bcc");
		pNewBccElement->SetAttribute("field", m_bcc->m_strfiled.c_str());
		for(int i = 0; i < m_bcc->m_addrs.size(); i++)
		{
			TiXmlElement* pNewAddrElement = new TiXmlElement("addr");
			pNewAddrElement->SetAttribute("pName",m_bcc->m_addrs[i].pName.c_str());
			pNewAddrElement->SetAttribute("mName",m_bcc->m_addrs[i].mName.c_str());
			pNewAddrElement->SetAttribute("dName",m_bcc->m_addrs[i].dName.c_str());

			pNewBccElement->InsertEndChild(*pNewAddrElement);
			delete pNewAddrElement;
		}
		m_pNewElement->InsertEndChild(*pNewBccElement);
		delete pNewBccElement;
		
	}
	else if(strncasecmp(field, "Subject:", sizeof("Subject:") - 1) == 0)
	{
		fnln_strcut(field, ":", " \t\r\n", " \t\r\n", m_subject);
		DecodeMIMEString(m_subject.c_str(), m_decodedSubject);

		TiXmlElement* pNewSubjectElement = new TiXmlElement("subject");
		
		pNewSubjectElement->SetAttribute("original",m_subject.c_str());
		pNewSubjectElement->SetAttribute("decoded",m_decodedSubject.c_str());
			
		m_pNewElement->InsertEndChild(*pNewSubjectElement);
		delete pNewSubjectElement;
		
	}
	else if(strncasecmp(field, "Date:", sizeof("Date:") - 1) == 0)
	{
		fnln_strcut(field, ":", " \t\r\n", " \t\r\n", m_date);
		//eg. Tue, 04 Nov 2008 16:51:41 +0800
		unsigned year , day, hour, min, sec, zone;
		char szmon[32];
		sscanf(m_date.c_str(), "%*[^,], %d %[^ ] %d %d:%d:%d %d", &day, szmon, &year, &hour, &min, &sec, &zone);
		m_tm.tm_year = year - 1900;
		m_tm.tm_mon = getmonthnumber(szmon);
		m_tm.tm_mday = day;
		m_tm.tm_hour = hour;
		m_tm.tm_min = min;
		m_tm.tm_sec = sec;

		TiXmlElement* pNewDateElement = new TiXmlElement("date");
		
		pNewDateElement->SetAttribute("string",m_date.c_str());

		char strtmp[64];
		sprintf(strtmp, "%u", m_tm.tm_year);
		pNewDateElement->SetAttribute("year", strtmp);

		sprintf(strtmp, "%u", m_tm.tm_mon);
		pNewDateElement->SetAttribute("mon", strtmp);

		sprintf(strtmp, "%u", m_tm.tm_mday);
		pNewDateElement->SetAttribute("mday", strtmp);

		sprintf(strtmp, "%u", m_tm.tm_hour);
		pNewDateElement->SetAttribute("hour", strtmp);

		sprintf(strtmp, "%u", m_tm.tm_min);
		pNewDateElement->SetAttribute("min", strtmp);

		sprintf(strtmp, "%u", m_tm.tm_sec);
		pNewDateElement->SetAttribute("sec", strtmp);
		
		m_pNewElement->InsertEndChild(*pNewDateElement);
		delete pNewDateElement;
	}
	else if(strncasecmp(field, "Message-ID:", sizeof("Message-ID:") - 1) == 0)
	{
		fnln_strcut(field, ":", " \t\r\n", " \t\r\n", m_messageID);
		
		TiXmlElement* pNewMessageIDElement = new TiXmlElement("messageID");
		
		pNewMessageIDElement->SetAttribute("value", m_messageID.c_str());
		
		m_pNewElement->InsertEndChild(*pNewMessageIDElement);
		delete pNewMessageIDElement;
		
	}
	else if(strncasecmp(field, "User-Agent:", sizeof("User-Agent:") - 1) == 0)
	{
		fnln_strcut(field, ":", " \t\r\n", " \t\r\n", m_userAgent);
		TiXmlElement* pNewUserAgentElement = new TiXmlElement("userAgent");
		
		pNewUserAgentElement->SetAttribute("value", m_userAgent.c_str());
		
		m_pNewElement->InsertEndChild(*pNewUserAgentElement);
		delete pNewUserAgentElement;
	}
	else if(strncasecmp(field, "In-Reply-To:", sizeof("In-Reply-To:") - 1) == 0)
	{
		fnln_strcut(field, ":", " \t\r\n", " \t\r\n", m_inReplyTo);
		TiXmlElement* pNewInReplyToElement = new TiXmlElement("inReplyTo");
		
		pNewInReplyToElement->SetAttribute("value", m_inReplyTo.c_str());
		
		m_pNewElement->InsertEndChild(*pNewInReplyToElement);
		delete pNewInReplyToElement;
	}

	m_linenumber += getcharnum(field, '\n');
	m_end += strlen(field);
}

filedaddr* LetterHeaderSummary::GetFrom()
{
	return m_from;
}

filedaddr* LetterHeaderSummary::GetTo()
{
	
	return m_to;
}

filedaddr* LetterHeaderSummary::GetReplyTo()
{
	
	return m_replyTo;
}

filedaddr* LetterHeaderSummary::GetSender()
{
	
	return m_sender;
}


filedaddr* LetterHeaderSummary::GetCc()
{
	return m_cc;
}

filedaddr* LetterHeaderSummary::GetBcc()
{
	return m_bcc;
}

const char* LetterHeaderSummary::GetSubject()
{
	return m_subject.c_str();
}

const char* LetterHeaderSummary::GetDecodedSubject()
{
	return m_decodedSubject.c_str();
}

const char* LetterHeaderSummary::GetMessageID()
{
	return m_messageID.c_str();
}

const char* LetterHeaderSummary::GetInReplyTo()
{
	return m_inReplyTo.c_str();
}

const char* LetterHeaderSummary::GetDate()
{
	return m_date.c_str();
}

struct tm* LetterHeaderSummary::GetTm()
{
	return &m_tm;
}

///////////////////////////////////////////////////////////////////////
// LetterDataSummary
LetterDataSummary::LetterDataSummary(TiXmlElement * pSelfElement) : BlockSummary(pSelfElement)
{

}

LetterDataSummary::LetterDataSummary(unsigned int beg, TiXmlElement * pParentElement) : BlockSummary(beg, pParentElement)
{
	if(!m_pNewElement)
		m_pNewElement = new TiXmlElement("letterData");
}

LetterDataSummary::~LetterDataSummary()
{
	if(m_pNewElement)
	{
		m_pNewElement->SetAttribute("beg", (int)m_beg);
		m_pNewElement->SetAttribute("end", (int)m_end);
		m_pNewElement->SetAttribute("linenumber", (int)m_linenumber);
		m_pParentElement->InsertEndChild(*m_pNewElement);
		delete m_pNewElement;
		m_pNewElement = NULL;
	}
}
void LetterDataSummary::loadXML()
{
	m_pSelfElement->Attribute("beg", (int*)&m_beg);
	m_pSelfElement->Attribute("end", (int*)&m_end);
	m_pSelfElement->Attribute("linenumber", (int*)&m_linenumber);
}

void LetterDataSummary::fieldParse(const char* field)
{
	m_linenumber += getcharnum(field, '\n');
	m_end += strlen(field);
}

////////////////////////////////////////////////////////////////////////
// MimeSummary

MimeSummary::MimeSummary(TiXmlElement* pSelfElement) : BlockSummary(pSelfElement)
{	
	m_isheader = TRUE;
	m_header = NULL;
	m_data = NULL;
}

MimeSummary::MimeSummary(unsigned int beg, TiXmlElement* pParentElement) : BlockSummary(beg, pParentElement)
{
	m_isheader = TRUE;
	
	m_header = NULL;
	m_data = NULL;
	
	if(!m_pNewElement)
		m_pNewElement = new TiXmlElement("MIME");
}

MimeSummary::~MimeSummary()
{
	if(m_header && !m_data)
	{
		m_data = new MimeDataSummary(m_header->m_end + 1, m_pNewElement);
	}
	
	if(m_header)
		delete m_header;

	if(m_data)
		delete m_data;
	
	if(m_pNewElement)
	{
		m_pNewElement->SetAttribute("beg", (int)m_beg);
		m_pNewElement->SetAttribute("end", (int)m_end);
		m_pNewElement->SetAttribute("linenumber", (int)m_linenumber);
		m_pParentElement->InsertEndChild(*m_pNewElement);
		
		delete m_pNewElement;
		m_pNewElement = NULL;
	}
}

void MimeSummary::loadXML()
{
	m_pSelfElement->Attribute("beg", (int*)&m_beg);
	m_pSelfElement->Attribute("end", (int*)&m_end);
	m_pSelfElement->Attribute("linenumber", (int*)&m_linenumber);
	
	TiXmlNode * pMimeHeaderNode = m_pSelfElement->FirstChild("MIMEHeader");
	if(pMimeHeaderNode && pMimeHeaderNode->ToElement())
	{
		if(!m_header)
		{
			m_header = new MimeHeaderSummary(pMimeHeaderNode->ToElement());
			if(m_header)
				m_header->loadXML();
		}
	}
	
	TiXmlNode * pMimeDataNode = m_pSelfElement->FirstChild("MIMEData");
	if(pMimeDataNode && pMimeDataNode->ToElement())
	{
		if(!m_data)
		{
			m_data = new MimeDataSummary(pMimeDataNode->ToElement());
			if(m_data)
				m_data->loadXML();
		}
	}
}

void MimeSummary::fieldParse(const char* field)
{	
	if(m_isheader == TRUE)
	{
		if(strcmp(field, "\r\n") == 0 || strcmp(field, "\n") == 0)
		{
			m_isheader = FALSE;
		}
		if(m_pNewElement)
		{
			if(!m_header)
				m_header = new MimeHeaderSummary(m_beg, m_pNewElement);
			if(m_header)
				m_header->fieldParse(field);
		}
	}
	else
	{
		if(!m_data)
		{
			m_data = new MimeDataSummary(m_end, m_pNewElement);
			if(m_header->m_Content_Type && strcasecmp(m_header->m_Content_Type->m_type1.c_str(), "multipart") == 0)
			{
				m_data->setBoundary(m_header->m_Content_Type->m_boundary.c_str());
			}
		}
		if(m_data)
			m_data->fieldParse(field);
		
	}
	m_linenumber += getcharnum(field, '\n');
	m_end += strlen(field);
}

/////////////////////////////////////////////////////////////////
// MimeHeaderSummary
MimeHeaderSummary::MimeHeaderSummary(TiXmlElement * pSelfElement) : BlockSummary(pSelfElement)
{
	m_Content_Type = NULL;
	m_Content_Disposition = NULL;
	
}

MimeHeaderSummary::MimeHeaderSummary(unsigned int beg, TiXmlElement * pParentElement) : BlockSummary(beg, pParentElement)
{
	m_Content_Type = NULL;
	m_Content_Disposition = NULL;
	
	if(!m_pNewElement)
		m_pNewElement = new TiXmlElement("MIMEHeader");
}

MimeHeaderSummary::~MimeHeaderSummary()
{
	if(m_pNewElement)
	{
		m_pNewElement->SetAttribute("beg", (int)m_beg);
		m_pNewElement->SetAttribute("end", (int)m_end);
		m_pNewElement->SetAttribute("linenumber", (int)m_linenumber);
		
		m_pParentElement->InsertEndChild(*m_pNewElement);
		delete m_pNewElement;
		m_pNewElement = NULL;
	}

	if(m_Content_Type)
		delete m_Content_Type;

	if(m_Content_Disposition)
		delete m_Content_Disposition;
	
}

void MimeHeaderSummary::loadXML()
{
	
	m_pSelfElement->Attribute("beg", (int*)&m_beg);
	m_pSelfElement->Attribute("end", (int*)&m_end);
	m_pSelfElement->Attribute("linenumber", (int*)&m_linenumber);
	
	TiXmlNode * pContentTypeNode = m_pSelfElement->FirstChild("contentType");

	if(pContentTypeNode && pContentTypeNode->ToElement())
	{
		m_Content_Type = new filedContentType();
		if(m_Content_Type)
		{
			m_Content_Type->m_type1= pContentTypeNode->ToElement()->Attribute("type1") == NULL ? "" : pContentTypeNode->ToElement()->Attribute("type1");
			m_Content_Type->m_type2= pContentTypeNode->ToElement()->Attribute("type2") == NULL ? "" : pContentTypeNode->ToElement()->Attribute("type2");
			m_Content_Type->m_charset= pContentTypeNode->ToElement()->Attribute("charset") == NULL ? "" : pContentTypeNode->ToElement()->Attribute("charset");
			m_Content_Type->m_filename= pContentTypeNode->ToElement()->Attribute("filename") == NULL ? "" : pContentTypeNode->ToElement()->Attribute("filename");
			m_Content_Type->m_boundary= pContentTypeNode->ToElement()->Attribute("boundary") == NULL ? "" : pContentTypeNode->ToElement()->Attribute("boundary");			
		}
	}
	
	TiXmlNode * pContentIDNode = m_pSelfElement->FirstChild("contentID");
	if(pContentIDNode && pContentIDNode->ToElement())
	{
		m_Content_ID = pContentIDNode->ToElement()->Attribute("value") == NULL ? "" : pContentIDNode->ToElement()->Attribute("value");		
	}

	TiXmlNode * pContentClassNode = m_pSelfElement->FirstChild("contentClass");
	if(pContentClassNode && pContentClassNode->ToElement())
	{
		m_Content_Class = pContentClassNode->ToElement()->Attribute("value") == NULL ? "" : pContentClassNode->ToElement()->Attribute("value");		
	}
	
	TiXmlNode * pContentTransferEncodingNode = m_pSelfElement->FirstChild("contentTransferEncoding");
	if(pContentTransferEncodingNode && pContentTransferEncodingNode->ToElement())
	{
		m_Content_Transfer_Encoding= pContentTransferEncodingNode->ToElement()->Attribute("value") == NULL ? "" : pContentTransferEncodingNode->ToElement()->Attribute("value");
	}
	
	TiXmlNode * pContentDispositionNode = m_pSelfElement->FirstChild("contentDisposition");

	if(pContentDispositionNode &&  pContentDispositionNode->ToElement())
	{
		m_Content_Disposition = new filedContentDisposition();
		if(m_Content_Disposition)
		{
			m_Content_Disposition->m_disposition= pContentDispositionNode->ToElement()->Attribute("disposition") == NULL ? "" : pContentDispositionNode->ToElement()->Attribute("disposition");
			m_Content_Disposition->m_filename= pContentDispositionNode->ToElement()->Attribute("filename") == NULL ? "" : pContentDispositionNode->ToElement()->Attribute("filename");	
		}
	}
}

void MimeHeaderSummary::fieldParse(const char* field)
{
	if(strncasecmp(field, "Content-Type:", sizeof("Content-Type:") - 1) == 0)
	{
		m_Content_Type = new filedContentType(field);

		TiXmlElement* pNewContentTypeElement = new TiXmlElement("contentType");
		
		pNewContentTypeElement->SetAttribute("type1", m_Content_Type->m_type1.c_str());
		pNewContentTypeElement->SetAttribute("type2", m_Content_Type->m_type2.c_str());
		pNewContentTypeElement->SetAttribute("boundary", m_Content_Type->m_boundary.c_str());
		pNewContentTypeElement->SetAttribute("charset", m_Content_Type->m_charset.c_str());
		pNewContentTypeElement->SetAttribute("filename", m_Content_Type->m_filename.c_str());
		
		m_pNewElement->InsertEndChild(*pNewContentTypeElement);
		delete pNewContentTypeElement;
		
	}
	else if(strncasecmp(field, "Content-ID:", sizeof("Content-ID:") - 1) == 0)
	{
		fnfy_strcut(field, ":", " \t\"<", " >\t\";\r\n", m_Content_ID);

		TiXmlElement* pNewContentIDElement = new TiXmlElement("contentID");
		
		pNewContentIDElement->SetAttribute("value", m_Content_ID.c_str());
		
		m_pNewElement->InsertEndChild(*pNewContentIDElement);
		delete pNewContentIDElement;
		
	}
	else if(strncasecmp(field, "Content-class:", sizeof("Content-class:") - 1) == 0)
	{
		fnfy_strcut(field, ":", " \t\"<", " >\t\";\r\n", m_Content_Class);

		TiXmlElement* pNewContentClassElement = new TiXmlElement("contentClass");
		
		pNewContentClassElement->SetAttribute("value", m_Content_Class.c_str());
		
		m_pNewElement->InsertEndChild(*pNewContentClassElement);
		delete pNewContentClassElement;
		
	}
	else if(strncasecmp(field, "Content-Transfer-Encoding:", sizeof("Content-Transfer-Encoding:") - 1) == 0)
	{
		fnfy_strcut(field, ":", " \t\"\r\n", " \t\";\r\n", m_Content_Transfer_Encoding);

		TiXmlElement* pNewContentTransferEncodingElement = new TiXmlElement("contentTransferEncoding");
		
		pNewContentTransferEncodingElement->SetAttribute("value", m_Content_Transfer_Encoding.c_str());
		
		m_pNewElement->InsertEndChild(*pNewContentTransferEncodingElement);
		delete pNewContentTransferEncodingElement;
		
	}
	else if(strncasecmp(field, "Content-Disposition:", sizeof("Content-Disposition:") - 1) == 0)
	{
		m_Content_Disposition = new filedContentDisposition(field);

		TiXmlElement* pNewContentDispositionElement = new TiXmlElement("contentDisposition");
		
		pNewContentDispositionElement->SetAttribute("disposition", m_Content_Disposition->m_disposition.c_str());
		pNewContentDispositionElement->SetAttribute("filename", m_Content_Disposition->m_filename.c_str());
		
		m_pNewElement->InsertEndChild(*pNewContentDispositionElement);
		delete pNewContentDispositionElement;
	}
	m_linenumber += getcharnum(field, '\n');
	m_end += strlen(field);

}


///////////////////////////////////////////////////////////////////////
// MimeDataSummary
MimeDataSummary::MimeDataSummary(TiXmlElement * pSelfElement) : BlockSummary(pSelfElement)
{
	m_boundary = "";
	m_pNewSummary = NULL;
}

MimeDataSummary::MimeDataSummary(unsigned int beg, TiXmlElement * pParentElement) : BlockSummary(beg, pParentElement)
{
	if(!m_pNewElement)
		m_pNewElement = new TiXmlElement("MIMEData");
	
	m_boundary = "";
	m_pNewSummary = NULL;
}

MimeDataSummary::~MimeDataSummary()
{
	for(int i = 0 ; i < m_vMimeSummary.size(); i++)
	{
		if(m_vMimeSummary[i])
		{
			delete m_vMimeSummary[i];
			m_vMimeSummary[i] = NULL;
		}
	}

	if(m_pNewElement)
	{
		m_pNewElement->SetAttribute("beg", (int)m_beg);
		
		m_pNewElement->SetAttribute("end", (int)m_end);
		m_pNewElement->SetAttribute("linenumber", (int)m_linenumber);
		m_pParentElement->InsertEndChild(*m_pNewElement);
		delete m_pNewElement;
		m_pNewElement = NULL;
	}
	
}

void MimeDataSummary::setBoundary(const char* boundary)
{
	m_boundary = boundary;

	m_boundary_beg = "--";
	m_boundary_beg += m_boundary;
	m_boundary_beg += "\r\n";

	m_boundary_end = "--";
	m_boundary_end += m_boundary;
	m_boundary_end += "--\r\n";
		
}

void MimeDataSummary::loadXML()
{
	m_pSelfElement->Attribute("beg", (int*)&m_beg);
	m_pSelfElement->Attribute("end", (int*)&m_end);
	m_pSelfElement->Attribute("linenumber", (int*)&m_linenumber);

	TiXmlNode * pMimeNode = m_pSelfElement->FirstChild("MIME");
	while(pMimeNode)
	{
		if(pMimeNode->ToElement())
		{
			MimeSummary * mime = new MimeSummary(pMimeNode->ToElement());
			mime->loadXML();
			m_vMimeSummary.push_back(mime);
		}
		pMimeNode = pMimeNode->NextSibling();
	}
}

void MimeDataSummary::fieldParse(const char* field)
{
	string strTmp = field;
	if(m_boundary != "")
	{
		if(strTmp ==  m_boundary_beg)
		{
			if(!m_pNewSummary)
			{
				m_pNewSummary = new MimeSummary(m_end + strlen(field), m_pNewElement);
			}
			else
			{
				m_vMimeSummary.push_back(m_pNewSummary);
				m_pNewSummary = new MimeSummary(m_end + strlen(field), m_pNewElement);
			}
		}
		else if(strTmp == m_boundary_end)
		{
			if(m_pNewSummary)
				m_vMimeSummary.push_back(m_pNewSummary);
            m_pNewSummary = NULL;
		}
		else
		{
			if(m_pNewSummary)
				m_pNewSummary->fieldParse(field);
		}
	}
	
	m_linenumber += getcharnum(field, '\n');
	m_end += strlen(field);
}

