/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _EMAIL_H_
#define _EMAIL_H_

#include "fstring.h"

class email
{
public:
	email(const char* domain, const char* username, const char* alias);
	email(const char* buf, int len);
	
	virtual ~email();
	
	int output(const char* clientip);
	
	void set_from_addrs(const char* arg);
	void set_to_addrs(const char* arg);
	void set_cc_addrs(const char* arg);
	void set_bcc_addrs(const char* arg);
	void set_reply_addrs(const char* arg);
	void set_subject(const char* arg);
	void set_content(const char* arg);
	void set_content_type(const char* arg);
	void add_attach(const char * file);
	void del_attach(const char * file);

	fbuffer m_buffer;
	
private:
	vector<fbufseg> m_vsegbuf;
	
	string m_username;
	string m_alias;
	string m_domain;
	
	string m_message_id;
	string m_date;
	string m_from_addrs;
	string m_to_addrs;
	string m_cc_addrs;
	string m_bcc_addre; /* do not show in the mail content, just for send */
	string m_reply_addrs;
	string m_user_agent;
	string m_mime_version;
	string m_subject;
	string m_content;
	string m_content_type;
	string m_boundary;
	vector<string> m_attaches;
};

#endif /* _EMAIL_H_*/

