#include "mime.h"

////////////////////////////////////////////////////////////////////
//filedFrom
filedaddr::filedaddr()
{
	
}

filedaddr::filedaddr(const char* sAddr)
{
	string strAddr;
	strcut(sAddr, ":", NULL, strAddr);
	strtrim(strAddr);
	m_strfiled = strAddr;
	vector<string> addrList;
	vSplitString(strAddr,addrList,",;");
	for(int x = 0; x < addrList.size(); x++)
	{
		strtrim(addrList[x]);
		strtrim(addrList[x], "\"");
		mailaddr addr;
		string strAddr;
		if((addrList[x].find("<", 0, 1) != string::npos)&&
			(addrList[x].find(">", 0, 1) != string::npos)&&
			(addrList[x].find("@", 0, 1) != string::npos))
		{
			strcut(addrList[x].c_str(), "<", ">", strAddr);
			strtrim(strAddr);	/*trim \r\n \t*/
			strtrim(strAddr, "\"");
			strcut(strAddr.c_str(), NULL, "@", addr.mName);
			strcut(strAddr.c_str(), "@", NULL, addr.dName);

			strcut(addrList[x].c_str(), NULL, "<", addr.pName);
			strtrim(addr.pName, "\" \t");
			if(addr.pName == "")
				addr.pName = addr.mName;
			m_addrs.push_back(addr);
		}
		else
		{
			strAddr = addrList[x].c_str();
			strtrim(strAddr); /*trim \r\n \t*/
			strcut(strAddr.c_str(), NULL, "@", addr.mName);
			strcut(strAddr.c_str(), "@", NULL, addr.dName);
			addr.pName = addr.mName;
			m_addrs.push_back(addr);
		}
	}
}

filedaddr::~filedaddr()
{
	
}

////////////////////////////////////////////////////////////////
//
filedContentType::filedContentType()
{

}

filedContentType::filedContentType(const char* sContentType)
{
	string strContentType;
	strcut(sContentType, ":", NULL, strContentType);
	strtrim(strContentType);
	string type;
	
	fnfy_strcut(strContentType.c_str(), NULL," \t\r\n", "; \t\r\n", type);
	
	strcut(type.c_str(), NULL, "/", m_type1);
	strcut(type.c_str(), "/", NULL, m_type2);
	
	if(strcasecmp(m_type1.c_str(), "multipart") == 0)
	{
		m_boundary = "";
		fnfy_strcut(strContentType.c_str(), "boundary=", " \t\r\n\"","; \"\t\r\n", m_boundary);
	}
	else
	{
		m_charset = "";
		m_filename = "";
		fnfy_strcut(strContentType.c_str(), "charset="," \t\r\n\"",";\"\r\n", m_charset);
		fnfy_strcut(strContentType.c_str(), "name="," \t\r\n\"",";\"\r\n", m_filename);		
	}

	string tmp = m_filename;
	DecodeMIMEString(tmp.c_str(), m_filename);
}

filedContentType::~filedContentType()
{

}
	

////////////////////////////////////////////////////////////////
//mimepart
mimepart::mimepart(const char* buffer, vector<fbufseg>* pvbufseg, unsigned int beg, unsigned int end)
{
	m_buffer = (char*)buffer;
	m_Content_Type = NULL;
	m_Content_Disposition = NULL;
	m_pfbufseg= pvbufseg;
	m_beg = beg;
	m_end = end;
	m_linenumber = 0;
	m_blocksize = 0;
	parse();
}

mimepart::~mimepart()
{
	if(m_Content_Type)
		delete m_Content_Type;
	if(m_Content_Disposition)
		delete m_Content_Disposition;
	for(int x = 0; x < m_vmimepart.size(); x++)
	{
		delete m_vmimepart[x];
	}
}

void mimepart::parse()
{
	int step = 0;
	m_blocksize = 0;
	m_linenumber = 0;
	
	for(int x = m_beg; x <= m_end; x++)
	{
		string strTmp;
		(*m_pfbufseg)[x].c_str(m_buffer, strTmp);
		
		m_blocksize += strTmp.length();
		m_linenumber += getcharnum(strTmp.c_str(), '\n');
		
		if(strncasecmp(strTmp.c_str(), "Content-Type:", strlen("Content-Type:")) == 0)
		{
			m_Content_Type = new filedContentType(strTmp.c_str());
		}
		else if(strncasecmp(strTmp.c_str(), "Content-ID:", strlen("Content-ID:")) == 0)
		{
			fnfy_strcut(strTmp.c_str(), ":", " \t\"<", " >\t\";\r\n", m_Content_ID);
		}
		else if(strncasecmp(strTmp.c_str(), "Content-Transfer-Encoding:", strlen("Content-Transfer-Encoding:")) == 0)
		{
			fnfy_strcut(strTmp.c_str(), ":", " \t\"\r\n", " \t\";\r\n", m_Content_Transfer_Encoding);
		}
		else if(strncasecmp(strTmp.c_str(), "Content-Disposition:", strlen("Content-Disposition:")) == 0)
		{
			m_Content_Disposition = new filedContentDisposition(strTmp.c_str());
		}
		else if(strTmp == "\r\n")
		{
			m_header.m_beg = m_beg;
			m_header.m_end = x;
			m_header.m_blocksize = m_blocksize;
			m_header.m_linenumber = m_linenumber;
			step = x + 1;
			break;
		}
	}

	//continue
	string strBeg, strEnd;
	unsigned int beg = 0xFFFFFFFFUL;
	unsigned int end = 0xFFFFFFFFUL;
	
	if((m_Content_Type)&&(m_Content_Type->m_boundary != ""))
	{
		strBeg = "--";
		strBeg += m_Content_Type->m_boundary;
		strBeg += "\r\n";

		strEnd = "--";
		strEnd += m_Content_Type->m_boundary;
		strEnd += "--\r\n";
	}
	for(int x = step; x <= m_end; x++)
	{
		string strTmp;
		(*m_pfbufseg)[x].c_str(m_buffer, strTmp);
		m_linenumber += getcharnum(strTmp.c_str(), '\n');
		m_blocksize += strTmp.length();
		
		if((m_Content_Type) && (strcasecmp(m_Content_Type->m_type1.c_str(), "multipart") == 0))
		{
			if(strTmp ==  strBeg)
			{
				if(beg == 0xFFFFFFFFUL)
				{
					beg = x + 1;
				}
				else
				{
					end = x - 1;
					if(end >= beg)
					{
						mimepart * pSubPart = new mimepart(m_buffer, m_pfbufseg, beg, end);
						m_vmimepart.push_back(pSubPart);
					}
					beg = x + 1;
				}
			}
			else if(strTmp == strEnd)
			{
				end = x - 1;
				if(end >= beg)
				{
					mimepart * pSubPart = new mimepart(m_buffer, m_pfbufseg, beg, end);
					m_vmimepart.push_back(pSubPart);
				}
			}
		}
	}
	
	m_data.m_beg = m_header.m_end + 1;
	m_data.m_end = m_end;
			
	m_data.m_linenumber = m_linenumber - m_header.m_linenumber;
	m_data.m_blocksize= m_blocksize - m_header.m_blocksize;
}

unsigned int mimepart::GetSubPartNumber()
{
	return m_vmimepart.size();
}

mimepart* mimepart::GetPart(unsigned int x)
{
	return m_vmimepart[x];
}

filedContentType* mimepart::GetContentType()
{
	return m_Content_Type;
}

//////////////////////////////////////////////////////////////////////////////
//class MIME
MIME::MIME(const char* buf, int len)
{
	m_From = NULL;
	m_Sender = NULL;	/* Sender */
	m_ReplyTo = NULL; 	/* Reply-To */
	m_To = NULL;
	m_CC = NULL;
	m_BCC = NULL;
	m_rootpart = NULL;
	
	m_parsedGlobal = FALSE;
	m_parsedHeader = FALSE;
	
	m_buffer = (char*)buf;
	m_buffer_len = len;

	
}

MIME::~MIME()
{
	if(m_From)
		delete m_From;
	if(m_Sender)
		delete m_Sender;
	if(m_ReplyTo)
		delete m_ReplyTo;
	if(m_To)
		delete m_To;
	if(m_BCC)
		delete m_BCC;
	if(m_CC)
		delete m_CC;
	if(m_rootpart)
		delete m_rootpart;
}

void MIME::GlobalParse()
{
	if(m_parsedGlobal)
		return;
		
	m_vfbufseg.clear();

	fbufseg seg;
	seg.m_byte_beg = 0;
	seg.m_byte_end = 0;
	for(int x =0; x < m_buffer_len; x++)
	{
		if((m_buffer[x] ==  '\n') && (m_buffer[x+1] !=  ' ') && (m_buffer[x+1] !=  '\t'))
		{
			seg.m_byte_end = x;
			m_vfbufseg.push_back(seg);
			seg.m_byte_beg = x + 1;
		}
	}
	
	for(unsigned int x = 0; x < m_vfbufseg.size(); x++)
	{
		string fstrbuf;
		m_vfbufseg[x].c_str(m_buffer, fstrbuf);
		
		if(strcasecmp(fstrbuf.c_str(), "\r\n") == 0)
		{
			break;
		}
		else if(strncasecmp(fstrbuf.c_str(), "From:", strlen("From:")) == 0)
		{
			m_From = new filedaddr(fstrbuf.c_str());
		}
		else if(strncasecmp(fstrbuf.c_str(), "To:", strlen("To:")) == 0)
		{
			m_To = new filedaddr(fstrbuf.c_str());
		}
		else if(strncasecmp(fstrbuf.c_str(), "Sender:", strlen("Sender:")) == 0)
		{
			m_Sender= new filedaddr(fstrbuf.c_str());
		}
		else if(strncasecmp(fstrbuf.c_str(), "Reply-To:", strlen("Reply-To:")) == 0)
		{
			m_ReplyTo= new filedaddr(fstrbuf.c_str());
		}
		else if(strncasecmp(fstrbuf.c_str(), "Cc:", strlen("Cc:")) == 0)
		{
			m_CC = new filedaddr(fstrbuf.c_str());
		}
		else if(strncasecmp(fstrbuf.c_str(), "BCC:", strlen("BCC:")) == 0)
		{
			m_BCC = new filedaddr(fstrbuf.c_str());
		}
		else if(strncasecmp(fstrbuf.c_str(), "Subject:", strlen("Subject:")) == 0)
		{
			fnln_strcut(fstrbuf.c_str(), ":", " \t\r\n", " \t\r\n", m_Subject);
			DecodeMIMEString(m_Subject.c_str(), m_DecodedSubject);
		}
		else if(strncasecmp(fstrbuf.c_str(), "Date:", strlen("Date:")) == 0)
		{
			fnln_strcut(fstrbuf.c_str(), ":", " \t\r\n", " \t\r\n", m_Date);
			//eg. Tue, 04 Nov 2008 16:51:41 +0800
			unsigned year , day, hour, min, sec, zone;
			char szmon[32];
			sscanf(m_Date.c_str(), "%*[^,], %d %[^ ] %d %d:%d:%d %d", &day, szmon, &year, &hour, &min, &sec, &zone);
			m_tm.tm_year = year - 1900;
			m_tm.tm_mon = getmonthnumber(szmon);
			m_tm.tm_mday = day;
			m_tm.tm_hour = hour;
			m_tm.tm_min = min;
			m_tm.tm_sec = sec;
		}
		else if(strncasecmp(fstrbuf.c_str(), "Message-ID:", strlen("Message-ID:")) == 0)
		{
			fnln_strcut(fstrbuf.c_str(), ":", " \t\r\n", " \t\r\n", m_Message_ID);
		}
		else if(strncasecmp(fstrbuf.c_str(), "User-Agent:", strlen("User-Agent:")) == 0)
		{
			fnln_strcut(fstrbuf.c_str(), ":", " \t\r\n", " \t\r\n", m_User_Agent);
		}
		else if(strncasecmp(fstrbuf.c_str(), "In-Reply-To:", strlen("In-Reply-To:")) == 0)
		{
			fnln_strcut(fstrbuf.c_str(), ":", " \t\r\n", " \t\r\n", m_In_Reply_To);
		}
	}
	
	m_rootpart = new mimepart(m_buffer, &m_vfbufseg, 0, (m_vfbufseg.size() - 1));

	m_parsedHeader = TRUE;
	m_parsedGlobal = TRUE;
}

void MIME::HeaderParse()
{
	if(m_parsedHeader || m_parsedGlobal)
		return;
	
	m_vfbufseg.clear();

	fbufseg seg;
	seg.m_byte_beg = 0;
	seg.m_byte_end = 0;
	for(int x =0; x < m_buffer_len; x++)
	{
		if((m_buffer[x] ==  '\n') && (m_buffer[x+1] !=  ' ') && (m_buffer[x+1] !=  '\t'))
		{
			seg.m_byte_end = x;
			m_vfbufseg.push_back(seg);

			string fstrbuf;
			m_vfbufseg[m_vfbufseg.size() - 1].c_str(m_buffer, fstrbuf);
			if(strcasecmp(fstrbuf.c_str(), "\r\n") == 0)
			{
				break;
			}
			else if(strcasecmp(fstrbuf.c_str(), "\n") == 0)
			{
				break;
			}
			
			seg.m_byte_beg = x + 1;
		}
	}
	
	for(unsigned int x = 0; x < m_vfbufseg.size(); x++)
	{
		
		string fstrbuf;
		m_vfbufseg[x].c_str(m_buffer, fstrbuf);
		
		if(strncasecmp(fstrbuf.c_str(), "From:", strlen("From:")) == 0)
		{
			m_From = new filedaddr(fstrbuf.c_str());
		}
		else if(strncasecmp(fstrbuf.c_str(), "To:", strlen("To:")) == 0)
		{
			m_To = new filedaddr(fstrbuf.c_str());
		}
		else if(strncasecmp(fstrbuf.c_str(), "Sender:", strlen("Sender:")) == 0)
		{
			m_Sender= new filedaddr(fstrbuf.c_str());
		}
		else if(strncasecmp(fstrbuf.c_str(), "Reply-To:", strlen("Reply-To:")) == 0)
		{
			m_ReplyTo= new filedaddr(fstrbuf.c_str());
		}
		else if(strncasecmp(fstrbuf.c_str(), "Cc:", strlen("Cc:")) == 0)
		{
			m_CC = new filedaddr(fstrbuf.c_str());
		}
		else if(strncasecmp(fstrbuf.c_str(), "BCC:", strlen("BCC:")) == 0)
		{
			m_BCC = new filedaddr(fstrbuf.c_str());
		}
		else if(strncasecmp(fstrbuf.c_str(), "Subject:", strlen("Subject:")) == 0)
		{
			fnln_strcut(fstrbuf.c_str(), ":", " \t\r\n", " \t\r\n", m_Subject);
			DecodeMIMEString(m_Subject.c_str(), m_DecodedSubject);
		}
		else if(strncasecmp(fstrbuf.c_str(), "Date:", strlen("Date:")) == 0)
		{
			fnln_strcut(fstrbuf.c_str(), ":", " \t\r\n", " \t\r\n", m_Date);
			//eg. Tue, 04 Nov 2008 16:51:41 +0800
			unsigned year , day, hour, min, sec, zone;
			char szmon[32];
			sscanf(m_Date.c_str(), "%*[^,], %d %[^ ] %d %d:%d:%d %d", &day, szmon, &year, &hour, &min, &sec, &zone);
			m_tm.tm_year = year - 1900;
			m_tm.tm_mon = getmonthnumber(szmon);
			m_tm.tm_mday = day;
			m_tm.tm_hour = hour;
			m_tm.tm_min = min;
			m_tm.tm_sec = sec;
		}
		else if(strncasecmp(fstrbuf.c_str(), "Message-ID:", strlen("Message-ID:")) == 0)
		{
			fnln_strcut(fstrbuf.c_str(), ":", " \t\r\n", " \t\r\n", m_Message_ID);
		}
		else if(strncasecmp(fstrbuf.c_str(), "User-Agent:", strlen("User-Agent:")) == 0)
		{
			fnln_strcut(fstrbuf.c_str(), ":", " \t\r\n", " \t\r\n", m_User_Agent);
		}
		else if(strncasecmp(fstrbuf.c_str(), "In-Reply-To:", strlen("In-Reply-To:")) == 0)
		{
			fnln_strcut(fstrbuf.c_str(), ":", " \t\r\n", " \t\r\n", m_In_Reply_To);
		}
	}

	m_parsedHeader = TRUE;
}

const char* MIME::getfield(unsigned int x, string& fstrbuf)
{
	return m_vfbufseg[x].c_str(m_buffer, fstrbuf);
}

filedaddr* MIME::GetFrom()
{
	return m_From;
}

filedaddr* MIME::GetTo()
{
	
	return m_To;
}

filedaddr* MIME::GetReplyTo()
{
	
	return m_ReplyTo;
}

filedaddr* MIME::GetSender()
{
	
	return m_Sender;
}


filedaddr* MIME::GetCc()
{
	return m_CC;
}

filedaddr* MIME::GetBcc()
{
	return m_BCC;
}

const char* MIME::GetSubject()
{
	return m_Subject.c_str();
}

const char* MIME::GetDecodedSubject()
{
	return m_DecodedSubject.c_str();
}

const char* MIME::GetMessageID()
{
	return m_Message_ID.c_str();
}

const char* MIME::GetInReplyTo()
{
	return m_In_Reply_To.c_str();
}

const char* MIME::GetDate()
{
	return m_Date.c_str();
}

struct tm* MIME::GetTm()
{
	return &m_tm;
}

