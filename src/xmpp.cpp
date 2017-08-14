/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "util/digcalc.h"
#include "xmpp.h"

static char CHAR_TBL[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890=/";

void __inline__ _strtrim_dquote_(string &src) /* icnluding double quote mark*/
{
	string::size_type ops1, ops2, ops3, ops4, ops5;
	bool bExit = false;
	while(!bExit)
	{
		bExit = true;
		ops1 = src.find_first_not_of(" ");
		if((ops1 != string::npos)&&(ops1 != 0))
		{
			src.erase(0, ops1);
			bExit = false;
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
		
		ops5 = src.find_first_not_of("\"");
		if((ops5 != string::npos)&&(ops5 != 0))
		{
			src.erase(0, ops5);
		}
		else if(ops5 == string::npos)
		{
			src.erase(0, src.length());
		}
	}
	bExit = false;
	while(!bExit)
	{
		bExit = true;
		ops1 = src.find_last_not_of(" ");
		if((ops1 != string::npos)&&(ops1 != src.length() - 1 ))
		{
			src.erase(ops1 + 1, src.length() - ops1 - 1);
			bExit = false;
		}
		
		ops2 = src.find_last_not_of("\t");
		if((ops2 != string::npos)&&(ops2 != src.length() - 1 ))
		{
			src.erase(ops2 + 1, src.length() - ops2 - 1);
			bExit = false;
		}
		ops3 = src.find_last_not_of("\r");
		if((ops3 != string::npos)&&(ops3 != src.length() - 1 ))
		{
			src.erase(ops3 + 1, src.length() - ops3 - 1);
			bExit = false;
		}
			
		ops4 = src.find_last_not_of("\n");
		if((ops4 != string::npos)&&(ops4 != src.length() - 1 ))
		{
			src.erase(ops4 + 1, src.length() - ops4 - 1);
			bExit = false;
		}
		ops5 = src.find_last_not_of("\"");
		if((ops5 != string::npos)&&(ops5 != src.length() - 1 ))
		{
			src.erase(ops5 + 1, src.length() - ops5 - 1);
			bExit = false;
		}
	}
}

bool xmpp_stanza::Parse(const char* text)
{
    m_xml_text += text;

    bool ret = m_xml.LoadString(m_xml_text.c_str(), m_xml_text.length());
    if(ret)
    {
        /* printf("\n\n%s\n", m_xml_text.c_str()); */
        
        TiXmlElement * pStanzaElement = m_xml.RootElement();
        if(pStanzaElement)
        {
            if(pStanzaElement->Attribute("to")
                && strcmp(pStanzaElement->Attribute("to"), "") != 0)
            {
                m_to = pStanzaElement->Attribute("to");
            }
            else if(pStanzaElement->Attribute("from")
                && strcmp(pStanzaElement->Attribute("from"), "") != 0)
            {
                m_from = pStanzaElement->Attribute("from");
            }
            else if(pStanzaElement->Attribute("id")
                && strcmp(pStanzaElement->Attribute("id"), "") != 0)
            {
                m_id = pStanzaElement->Attribute("id");
            }
        }
    }

    return ret;
}

map<string, CXmpp* > CXmpp::m_online_list;
pthread_rwlock_t CXmpp::m_online_list_lock;
BOOL CXmpp::m_online_list_inited = FALSE;

CXmpp::CXmpp(int sockfd, SSL * ssl, SSL_CTX * ssl_ctx, const char* clientip,
    StorageEngine* storage_engine, memcached_st * memcached, BOOL isSSL)
{
    if(!m_online_list_inited)
    {
        pthread_rwlock_init(&m_online_list_lock, NULL);
        m_online_list_inited = TRUE;
    }
    
    m_indent = 0;
    
    m_xmpp_stanza = NULL;

    pthread_mutex_init(&m_send_lock, NULL);

    m_status = STATUS_ORIGINAL;
	  m_sockfd = sockfd;
  	m_clientip = clientip;

    m_bSTARTTLS = FALSE;

    m_lsockfd = NULL;
    m_lssl = NULL;

    m_storageEngine = storage_engine;
    m_memcached = memcached;

    m_ssl = ssl;
    m_ssl_ctx = ssl_ctx;

  	m_isSSL = isSSL;
  	if(m_isSSL && m_ssl)
  	{
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

    m_stream_count = 0;
    m_auth_success = FALSE;

}

CXmpp::~CXmpp()
{
    if(m_xmpp_stanza)
        delete m_xmpp_stanza;
    
    m_xmpp_stanza = NULL;
    
    pthread_rwlock_wrlock(&m_online_list_lock); //acquire write
    m_online_list.erase(m_username);

    if(m_online_list.size() == 0)
    {
        m_online_list_inited = FALSE;
        pthread_rwlock_destroy(&m_online_list_lock);
    }
    else
    {
        pthread_rwlock_unlock(&m_online_list_lock); //release write
    }

    //send the unavailable presence
    if(m_auth_success)
    {
        MailStorage* mailStg;
        StorageEngineInstance stgengine_instance(GetStg(), &mailStg);
        if(mailStg)
        {
            TiXmlPrinter xml_printer;
            xml_printer.SetIndent("");
            vector<string> buddys;
            mailStg->ListBuddys(GetUsername(), buddys);
            stgengine_instance.Release();

            TiXmlDocument xmlPresence;
            xmlPresence.LoadString("<presence type='unavailable' />", strlen("<presence type='unavailable' />"));

            pthread_rwlock_rdlock(&m_online_list_lock); //acquire read

            for(int x = 0; x < buddys.size(); x++)
            {
                map<string, CXmpp*>::iterator it = m_online_list.find(buddys[x]);

                if(it != m_online_list.end())
                {
                    CXmpp* pXmpp = it->second;

                    TiXmlElement * pPresenceElement = xmlPresence.RootElement();

                    char xmpp_buf[1024];
                    if(m_resource == "")
                        sprintf(xmpp_buf, "%s@%s",
                            m_username.c_str(), CMailBase::m_email_domain.c_str());
                    else
                        sprintf(xmpp_buf, "%s@%s/%s",
                            m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());

                    pPresenceElement->SetAttribute("from", xmpp_buf);

                    sprintf(xmpp_buf, "%s@%s/%s",
                        buddys[x].c_str(), CMailBase::m_email_domain.c_str(), pXmpp->GetResource());
                    pPresenceElement->SetAttribute("to", xmpp_buf);

                    pPresenceElement->Accept( &xml_printer );

                    pXmpp->ProtSend2(xml_printer.CStr(), xml_printer.Size());
                }
            }
            pthread_rwlock_unlock(&m_online_list_lock); //acquire write
        }
    }

    //release the resource
    if(m_bSTARTTLS)
	    close_ssl(m_ssl, m_ssl_ctx);
    
    if(m_lsockfd)
        delete m_lsockfd;
    
    if(m_lssl)
        delete m_lssl;

    m_lsockfd = NULL;
    m_lssl = NULL;

    pthread_mutex_destroy(&m_send_lock);

}

int CXmpp::XmppSend(const char* buf, int len)
{
    int ret;
    
    pthread_mutex_lock(&m_send_lock);
    
    if(m_ssl)
        ret = SSLWrite(m_sockfd, m_ssl, buf, len);
    else
        ret = Send(m_sockfd, buf, len);
    
    /* printf("%s", buf); */
    
    pthread_mutex_unlock(&m_send_lock);
    
    return ret;
}

int CXmpp::ProtSend2(const char* buf, int len)
{
    m_send_buf += buf;
    
    if(m_send_buf.length() == 0)
        return 0;
    
    int sent_len = 0;
    if(m_ssl)
        sent_len = SSL_write(m_ssl, m_send_buf.c_str(), m_send_buf.length());
    else
        sent_len = send(m_sockfd, m_send_buf.c_str(), m_send_buf.length(), 0);
    
    if(sent_len > 0)
    {
        if(m_send_buf.length() == sent_len)
            m_send_buf = "";
        else
            m_send_buf = m_send_buf.substr(sent_len, m_send_buf.length() - sent_len);
    }
    return sent_len >= 0 ? 0 : -1;
}

int CXmpp::ProtFlush()
{
    if(m_send_buf.length() == 0)
        return 0;
    
    int sent_len = 0;
    if(m_ssl)
        sent_len = SSL_write(m_ssl, m_send_buf.c_str(), m_send_buf.length());
    else
        sent_len = send(m_sockfd, m_send_buf.c_str(), m_send_buf.length(), 0);
    
    if(sent_len > 0)
    {
        if(m_send_buf.length() == sent_len)
            m_send_buf = "";
        else
            m_send_buf = m_send_buf.substr(sent_len, m_send_buf.length() - sent_len);
    }
    return sent_len >= 0 ? 0 : -1;
}

int CXmpp::ProtRecv(char* buf, int len)
{
    if(m_ssl)
		return m_lssl->xrecv(buf, len);
	else
		return m_lsockfd->xrecv(buf, len);
}

int CXmpp::ProtRecv2(char* buf, int len)
{
    if(m_ssl)
		return m_lssl->xrecv_t(buf, len);
	else
		return m_lsockfd->xrecv_t(buf, len);
}

BOOL CXmpp::Parse(char* text)
{
    BOOL result = TRUE;
   
    char szTag[256] = "";
    char szEnd[3] = "";
    
    sscanf(strstr(text, "<"), "<%[^ >]%*[^>]>", &szTag);
    if(strlen(text) > 2)
    {
        memcpy(szEnd, text + strlen(text) - 2, 2);
        szEnd[2] = '\0';
    }
    
    if(strcasecmp(szTag, "?xml") == 0)
    {
        m_xml_declare = text;
    }
    else if(strcasecmp(szTag, "stream:stream") == 0)
    {
        if(!m_xmpp_stanza)
          m_xmpp_stanza = new xmpp_stanza(m_xml_declare.c_str());
      
        string xml_padding = text;
        xml_padding += "</stream:stream>";
        if(m_xmpp_stanza->Parse(xml_padding.c_str()) && strcmp(m_xmpp_stanza->GetTag(), "stream:stream") == 0)
        {
          if(!StreamTag(m_xmpp_stanza->GetXml()))
              result = FALSE;
        }
        else
        {
          result = FALSE;
        }
        delete m_xmpp_stanza;
        m_xmpp_stanza = NULL;
        
        //reset the indent
        m_indent = 0;
    }
    else if(strcasecmp(szTag, "/stream:stream") == 0)
    {
        m_indent = 0;
        result = FALSE;
    }
    else if(szTag[0] == '/' || szEnd[0] == '/')
    {
      if(szTag[0] == '/')
        m_indent--;
    
      if(!m_xmpp_stanza)
          m_xmpp_stanza = new xmpp_stanza(m_xml_declare.c_str());
      
      if(m_indent == 0 && m_xmpp_stanza->Parse(text))
      {
        const char* szTagName = m_xmpp_stanza->GetTag();
        
        if(szTagName && strcasecmp(szTagName, "message") == 0)
        {
          if(m_encryptxmpp == XMPP_TLS_REQUIRED && !m_bSTARTTLS)
          {
              result = FALSE;
          }
          else
          {
            if(!MessageTag(m_xmpp_stanza->GetXml()))
                result = FALSE;
          }
        }
        else if(szTagName && strcasecmp(szTagName, "iq") == 0)
        {
          if(m_encryptxmpp == XMPP_TLS_REQUIRED && !m_bSTARTTLS)
          {
              result = FALSE;
          }
          else
          {
            if(!IqTag(m_xmpp_stanza->GetXml()))
                result = FALSE;
          }
        }
        else if(szTagName && strcasecmp(szTagName, "presence") == 0)
        {
          if(m_encryptxmpp == XMPP_TLS_REQUIRED && !m_bSTARTTLS)
          {
              result = FALSE;
          }
          else
          {
            if(!PresenceTag(m_xmpp_stanza->GetXml()))
                result = FALSE;
          }
        }
        else if(szTagName && strcasecmp(szTagName, "auth") == 0)
        {
          if(m_encryptxmpp == XMPP_TLS_REQUIRED && !m_bSTARTTLS)
          {
              result = FALSE;
          }
          else
          {
            if(!AuthTag(m_xmpp_stanza->GetXml()))
                result = FALSE;          
          }
        }
        else if(szTagName && strcasecmp(szTagName, "response") == 0)
        {
          if(m_encryptxmpp == XMPP_TLS_REQUIRED && !m_bSTARTTLS)
          {
              result = FALSE;
          }
          else
          {
            if(!ResponseTag(m_xmpp_stanza->GetXml()))
              result = FALSE;
          }
        }
        else if(szTagName && strcasecmp(szTagName, "starttls") == 0)
        {
            if(!StarttlsTag(m_xmpp_stanza->GetXml()))
              result = FALSE;
        }
        else
        {
          if(m_auth_success)
          {
            char xmpp_buf[1024];
            const char* szFrom = m_xmpp_stanza->GetFrom();
            if(!szFrom || szFrom[0] == '\0')
            {
              if(m_resource == "")
                sprintf(xmpp_buf, "%s@%s",
                    m_username.c_str(), CMailBase::m_email_domain.c_str());
              else
                sprintf(xmpp_buf, "%s@%s/%s",
                    m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());
              m_xmpp_stanza->SetFrom(xmpp_buf);
              szFrom = xmpp_buf;
            }

            const char* szTo = m_xmpp_stanza->GetTo();
            string str_username;
            if(szTo && szTo[0] != '\0' && strstr(szTo, "@") != NULL)
            {
                strcut(szTo, NULL, "@", str_username);
                strtrim(str_username);
                
                if(str_username != "")
                {
                    TiXmlPrinter xml_printer;
                    xml_printer.SetIndent("");
                    m_xmpp_stanza->GetXml()->Accept( &xml_printer );

                    pthread_rwlock_rdlock(&m_online_list_lock); //acquire read
                    map<string, CXmpp*>::iterator it = m_online_list.find(str_username);

                    if(it != m_online_list.end())
                    {
                        CXmpp* pXmpp = it->second;
                        pXmpp->ProtSend2(xml_printer.CStr(), xml_printer.Size());
                    }
                    else
                    {
                        MailStorage* mailStg;
                        StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
                        if(!mailStg)
                        {
                            fprintf(stderr, "%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
                            pthread_rwlock_unlock(&m_online_list_lock);
                            return FALSE;
                        }

                        mailStg->InsertMyMessage(szFrom, szTo, xml_printer.CStr());

                        stgengine_instance.Release();
                    }
                    pthread_rwlock_unlock(&m_online_list_lock);
                }
            }
          }
        }
              
        delete m_xmpp_stanza;
        m_xmpp_stanza = NULL;
      }
      else
      {
          if(!m_xmpp_stanza)
            m_xmpp_stanza = new xmpp_stanza(m_xml_declare.c_str());
        
          m_xmpp_stanza->Append(text);
      }
    }
    else
    {
        m_indent++;
        if(!m_xmpp_stanza)
          m_xmpp_stanza = new xmpp_stanza(m_xml_declare.c_str());
        
        m_xmpp_stanza->Append(text);
    }
	return result;
}

BOOL CXmpp::StarttlsTag(TiXmlDocument* xmlDoc)
{
    char xmpp_buf[1024];
    sprintf(xmpp_buf, "<proceed xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>");
    if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
        return FALSE;
    
    int flags = fcntl(m_sockfd, F_GETFL, 0); 
    fcntl(m_sockfd, F_SETFL, flags & (~O_NONBLOCK)); 
    
    if(!create_ssl(m_sockfd, 
        m_ca_crt_root.c_str(),
        m_ca_crt_server.c_str(),
        m_ca_password.c_str(),
        m_ca_key_server.c_str(),
        m_enableclientcacheck,
        &m_ssl, &m_ssl_ctx))
    {
        throw new string(ERR_error_string(ERR_get_error(), NULL));
        return FALSE;
    }
    
    flags = fcntl(m_sockfd, F_GETFL, 0); 
    fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);

    m_lssl = new linessl(m_sockfd, m_ssl);
    
    if(!m_lssl)
        return FALSE;
    
    m_bSTARTTLS = TRUE;
    return TRUE;
}

BOOL CXmpp::StreamTag(TiXmlDocument* xmlDoc)
{
    if(ProtSend2(m_xml_declare.c_str(), m_xml_declare.length()) != 0)
        return FALSE;

    char stream_id[128];
    sprintf(stream_id, "%s_%u_%p_%d", CMailBase::m_localhostname.c_str(), getpid(), this, m_stream_count++);

    MD5_CTX_OBJ context;
    context.MD5Update((unsigned char*)stream_id, strlen(stream_id));
    unsigned char digest[16];
    context.MD5Final (digest);
    sprintf(m_stream_id,
        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7],
        digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15]);

    char xmpp_buf[2048];
    sprintf(xmpp_buf, "<stream:stream from='%s' id='%s' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' version='1.0'>",
      CMailBase::m_email_domain.c_str(), m_stream_id);
    if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
        return FALSE;

    if(m_auth_success == TRUE)
    {
        sprintf(xmpp_buf,
          "<stream:features>"
              "<compression xmlns='http://jabber.org/features/compress'>"
                  "<method>zlib</method>"
              "</compression>"
              "<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'/>"
              "<session xmlns='urn:ietf:params:xml:ns:xmpp-session'/>"
          "</stream:features>");

        if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
            return FALSE;
    }
    else
    {
        if(m_encryptxmpp == XMPP_OLDSSL_BASED)
        {
            sprintf(xmpp_buf,
              "<stream:features>"
                "<mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>"
                    "<mechanism>DIGEST-MD5</mechanism>"
                    "<mechanism>PLAIN</mechanism>"
                "</mechanisms>"
              "</stream:features>");
        }
        else if(m_encryptxmpp == XMPP_TLS_REQUIRED && !m_bSTARTTLS)
        {
            sprintf(xmpp_buf,
              "<stream:features>"
                "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'>"
                    "<required/>"
                "</starttls>"
                "<mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>"
                    "<mechanism>DIGEST-MD5</mechanism>"
                    "<mechanism>PLAIN</mechanism>"
                "</mechanisms>"
              "</stream:features>");
        }
        else
        {
            if(!m_bSTARTTLS)
            {
                sprintf(xmpp_buf,
                  "<stream:features>"
                    "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls' />"
                    "<mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>"
                        "<mechanism>DIGEST-MD5</mechanism>"
                        "<mechanism>PLAIN</mechanism>"
                    "</mechanisms>"
                  "</stream:features>");
            }
            else
            {
                sprintf(xmpp_buf,
                  "<stream:features>"
                    "<mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>"
                        "<mechanism>DIGEST-MD5</mechanism>"
                        "<mechanism>PLAIN</mechanism>"
                    "</mechanisms>"
                  "</stream:features>");
            }
        }
        if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
            return FALSE;
    }

    return TRUE;
}

BOOL CXmpp::PresenceTag(TiXmlDocument* xmlDoc)
{
    char xmpp_buf[2048];

    TiXmlElement * pPresenceElement = xmlDoc->RootElement();

    if(pPresenceElement)
    {
        presence_type p_type = PRESENCE_AVAILABLE;

        string str_p_type = pPresenceElement->Attribute("type") ? pPresenceElement->Attribute("type") : "";

        if(str_p_type == "subscribe")
        {
            p_type = PRESENCE_SBSCRIBE;
        }
        else if(str_p_type == "available")
        {
            p_type = PRESENCE_AVAILABLE;
        }
        else if(str_p_type == "unavailable")
        {
            p_type = PRESENCE_UNAVAILABLE;
        }
        else if(str_p_type == "subscribed")
        {
            p_type = PRESENCE_SBSCRIBED;
        }
        else if(str_p_type == "unsubscribe")
        {
            p_type = PRESENCE_UNSBSCRIBE;
        }
        else if(str_p_type == "unsubscribed")
        {
            p_type = PRESENCE_UNSBSCRIBED;
        }
        else if(str_p_type == "probe")
        {
            p_type = PRESENCE_PROBE;
        }

        string str_to;

        if(pPresenceElement->Attribute("to")
            && strcmp(pPresenceElement->Attribute("to"), "") != 0
            && strcmp(pPresenceElement->Attribute("to"), CMailBase::m_email_domain.c_str()) != 0)
        {
             str_to = pPresenceElement->Attribute("to");
        }

        string strid = "";
        if(strstr(str_to.c_str(), "@") != NULL)
        {
            strcut(str_to.c_str(), NULL, "@", strid);
            strtrim(strid);
        }
        if(strid != "")
        {
            if(p_type== PRESENCE_SBSCRIBED)
            {
                MailStorage* mailStg;
                StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
                if(!mailStg)
                {
                    fprintf(stderr, "%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
                    return FALSE;
                }

                mailStg->InsertBuddy(m_username.c_str(), strid.c_str());

                stgengine_instance.Release();
            }
            else if(p_type== PRESENCE_UNSBSCRIBED)
            {
                MailStorage* mailStg;
                StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
                if(!mailStg)
                {
                    fprintf(stderr, "%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
                    return FALSE;
                }

                mailStg->RemoveBuddy(m_username.c_str(), strid.c_str());

                stgengine_instance.Release();
            }
        }
        TiXmlElement pReponseElement(*pPresenceElement);

        char xmpp_buf[1024];
        if(m_resource == "")
            sprintf(xmpp_buf, "%s@%s",
                m_username.c_str(), CMailBase::m_email_domain.c_str());
        else
            sprintf(xmpp_buf, "%s@%s/%s",
                m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());

        pReponseElement.SetAttribute("from", xmpp_buf);

        TiXmlPrinter xml_printer;
        xml_printer.SetIndent("");
        if(strid != "")
        {
            pReponseElement.Accept( &xml_printer );

            pthread_rwlock_rdlock(&m_online_list_lock); //acquire read
            map<string, CXmpp*>::iterator it = m_online_list.find(strid);

            if(it != m_online_list.end())
            {
                CXmpp* pXmpp = it->second;
                pXmpp->ProtSend2(xml_printer.CStr(), xml_printer.Size());
            }
            else
            {
                MailStorage* mailStg;
                StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
                if(!mailStg)
                {
                    fprintf(stderr, "%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
                    pthread_rwlock_unlock(&m_online_list_lock);
                    return FALSE;
                }

                mailStg->InsertMyMessage(xmpp_buf, str_to.c_str(), xml_printer.CStr());

                stgengine_instance.Release();
            }
            pthread_rwlock_unlock(&m_online_list_lock);
        }
        else
        {
            if(p_type == PRESENCE_AVAILABLE)
            {
                MailStorage* mailStg;
                StorageEngineInstance stgengine_instance(GetStg(), &mailStg);
                if(!mailStg)
                {
                    fprintf(stderr, "%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
                    return FALSE;
                }

                vector<string> buddys;
                mailStg->ListBuddys(GetUsername(), buddys);
                stgengine_instance.Release();
                pthread_rwlock_rdlock(&m_online_list_lock); //acquire read
                for(int x = 0; x < buddys.size(); x++)
                {
                    map<string, CXmpp*>::iterator it = m_online_list.find(buddys[x]);

                    if(it != m_online_list.end())
                    {
                        CXmpp* pXmpp = it->second;

                        sprintf(xmpp_buf, "%s@%s/%s",
                            buddys[x].c_str(), CMailBase::m_email_domain.c_str(), pXmpp->GetResource());
                        pReponseElement.SetAttribute("to", xmpp_buf);
                        pReponseElement.Accept( &xml_printer );

                        pXmpp->ProtSend2(xml_printer.CStr(), xml_printer.Size());
                    }
                }
                pthread_rwlock_unlock(&m_online_list_lock);
            }
        }

    }

    return TRUE;
}

BOOL CXmpp::IqTag(TiXmlDocument* xmlDoc)
{
    TiXmlElement * pIqElement = xmlDoc->RootElement();
    if(pIqElement)
    {
        TiXmlElement pReponseElement(*pIqElement);
        TiXmlPrinter xml_printer;
        xml_printer.SetIndent("");

        if(pIqElement->Attribute("to")
            && strcmp(pIqElement->Attribute("to"), "") != 0
            && strcmp(pIqElement->Attribute("to"), CMailBase::m_email_domain.c_str()) != 0)
        {
            string strid = "";
            if(strstr(pIqElement->Attribute("to"), "@") != NULL)
            {
                strcut(pIqElement->Attribute("to"), NULL, "@", strid);
                strtrim(strid);
            }
            if(strid != "")
            {
                char xmpp_from[1024];
                if(m_resource == "")
                    sprintf(xmpp_from, "%s@%s",
                    m_username.c_str(), CMailBase::m_email_domain.c_str());
                else
                    sprintf(xmpp_from, "%s@%s/%s",
                        m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());

                pReponseElement.SetAttribute("from", xmpp_from);

                pReponseElement.Accept( &xml_printer );

                pthread_rwlock_rdlock(&m_online_list_lock); //acquire read

                map<string, CXmpp*>::iterator it = m_online_list.find(strid);

                if(it != m_online_list.end())
                {
                    CXmpp* pXmpp = it->second;
                    pXmpp->ProtSend2(xml_printer.CStr(), xml_printer.Size());
                }
                pthread_rwlock_unlock(&m_online_list_lock);
            }
            return TRUE;
        }

        iq_type itype = IQ_UNKNOW;
        string iq_id = pIqElement->Attribute("id") ? pIqElement->Attribute("id") : "";
        if(pIqElement->Attribute("type"))
        {
            if(pIqElement->Attribute("type") && strcasecmp(pIqElement->Attribute("type"), "set") == 0)
            {
                itype = IQ_SET;
            }
            else if(pIqElement->Attribute("type") && strcasecmp(pIqElement->Attribute("type"), "get") == 0)
            {
                itype = IQ_GET;
            }
            else if(pIqElement->Attribute("type") && strcasecmp(pIqElement->Attribute("type"), "result") == 0)
            {
                itype = IQ_RESULT;
            }
        }

        if(itype == IQ_RESULT)
            return TRUE;

        BOOL isPresenceProbe = FALSE;
        vector<string> buddys;

        char xmpp_buf[4096];
        sprintf(xmpp_buf,
            "<iq type='result' id='%s' from='%s' to='%s/%s'>",
                iq_id.c_str(), CMailBase::m_email_domain.c_str(), m_username.c_str(), m_stream_id);

        if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
            return FALSE;

        TiXmlNode* pChildNode = pIqElement->FirstChild();
        while(pChildNode)
        {
            if(pChildNode && pChildNode->ToElement())
            {
                if(pChildNode->Value() && strcasecmp(pChildNode->Value(), "bind") == 0)
                {
                    TiXmlNode* pGrandChildNode = pChildNode->ToElement()->FirstChild();
                    if(pGrandChildNode && pGrandChildNode->ToElement())
                    {
                        if(pGrandChildNode->Value() && strcasecmp(pGrandChildNode->Value(), "resource") == 0)
                        {
                            if(itype == IQ_SET && pGrandChildNode->ToElement() && pGrandChildNode->ToElement()->GetText())
                                m_resource = pGrandChildNode->ToElement()->GetText();
                        }

                    }
                    if(m_resource == "")
                        sprintf(xmpp_buf,
                        "<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'>"
                            "<jid>%s@%s</jid>"
                        "</bind>",
                        m_username.c_str(), CMailBase::m_email_domain.c_str());
                    else         
                        sprintf(xmpp_buf,
                            "<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'>"
                                "<jid>%s@%s/%s</jid>"
                            "</bind>",
                            m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());
                            
                    if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
                        return FALSE;
                }
                else if(pChildNode->Value() && strcasecmp(pChildNode->Value(), "query") == 0)
                {

                    string xmlns = pChildNode->ToElement() && pChildNode->ToElement()->Attribute("xmlns") ? pChildNode->ToElement()->Attribute("xmlns") : "";

                    if(strcasecmp(xmlns.c_str(), "jabber:iq:roster") == 0)
                    {
                        char* xmpp_buddys;
                        MailStorage* mailStg;
                        StorageEngineInstance stgengine_instance(GetStg(), &mailStg);
                        if(!mailStg)
                        {
                            fprintf(stderr, "%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
                            return FALSE;
                        }


                        string strMessage;
                        mailStg->ListBuddys(GetUsername(), buddys);
                        stgengine_instance.Release();

                        string str_items;
                        for(int x = 0; x < buddys.size(); x++)
                        {
                            str_items += "<item jid='>";
                            str_items += buddys[x];
                            str_items += "'/>";
                        }

                        xmpp_buddys = (char*)malloc(str_items.length() + 1024);
                        sprintf(xmpp_buddys,
                            "<query xmlns='%s'>%s</query>",
                            iq_id.c_str(), CMailBase::m_email_domain.c_str(), m_username.c_str(), m_stream_id, xmlns.c_str(), str_items.c_str());

                        if(ProtSend2(xmpp_buddys, strlen(xmpp_buddys)) != 0)
                            return FALSE;

                        free(xmpp_buddys);

                        isPresenceProbe = TRUE;

                    }
                    else if(strcasecmp(xmlns.c_str(), "http://jabber.org/protocol/disco#items") == 0)
                    {
                       sprintf(xmpp_buf,
                            "<query xmlns='%s'/>", xmlns.c_str());
                       if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
                           return FALSE;

                    }
                    else if(strcasecmp(xmlns.c_str(), "http://jabber.org/protocol/disco#info") == 0)
                    {
                       sprintf(xmpp_buf,
                            "<query xmlns='%s'>"
                            "</query>", xmlns.c_str());
                        if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
                            return FALSE;
                    }
                    else
                    {
                        sprintf(xmpp_buf,
                            "<query xmlns='%s'/>",xmlns.c_str());
                        if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
                            return FALSE;
                    }

                }
                else if(pChildNode->Value() && strcasecmp(pChildNode->Value(), "session") == 0)
                {

                }
                else if(pChildNode->Value() && strcasecmp(pChildNode->Value(), "ping") == 0)
                {
                   string xmlns = pChildNode->ToElement() && pChildNode->ToElement()->Attribute("xmlns") ? pChildNode->ToElement()->Attribute("xmlns") : "";
                   sprintf(xmpp_buf,
                        "<ping xmlns='%s'/>",xmlns.c_str());
                    if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
                        return FALSE;
                }
                else if(pChildNode->Value() && strcasecmp(pChildNode->Value(), "vCard") == 0 )
                {
                    string xmlns = pChildNode->ToElement() && pChildNode->ToElement()->Attribute("xmlns") ? pChildNode->ToElement()->Attribute("xmlns") : "";
                    sprintf(xmpp_buf,
                            "<vCard xmlns='%s'/>", xmlns.c_str());
                    if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
                        return FALSE;
                }
                else
                {
                    TiXmlPrinter xml_printer;
                    xml_printer.SetIndent("");
                    pChildNode->Accept( &xml_printer );

                    if(ProtSend2(xml_printer.CStr(), xml_printer.Size()) != 0)
                        return FALSE;
                }
            }
            pChildNode = pChildNode->NextSibling();
        }

        if(ProtSend2("</iq>", strlen("</iq>")) != 0)
            return FALSE;

        if(isPresenceProbe)
        {
            //send the probe presence
            TiXmlDocument xmlPresence;
            xmlPresence.LoadString("<presence type='available' />", strlen("<presence type='available' />"));

            pthread_rwlock_rdlock(&m_online_list_lock); //acquire read


            for(int x = 0; x < buddys.size(); x++)
            {
                map<string, CXmpp*>::iterator it = m_online_list.find(buddys[x]);

                if(it != m_online_list.end())
                {
                    TiXmlElement * pPresenceElement = xmlPresence.RootElement();
                    
                    if(m_resource == "")
                        sprintf(xmpp_buf, "%s@%s",
                            m_username.c_str(), CMailBase::m_email_domain.c_str());
                    else
                        sprintf(xmpp_buf, "%s@%s/%s",
                            m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());

                    pPresenceElement->SetAttribute("to", xmpp_buf);

                    CXmpp* pXmpp = it->second;

                    sprintf(xmpp_buf, "%s@%s/%s",
                        buddys[x].c_str(), CMailBase::m_email_domain.c_str(), pXmpp->GetResource());
                    pPresenceElement->SetAttribute("from", xmpp_buf);

                    TiXmlPrinter xml_printer;
                    xml_printer.SetIndent("");
                    pPresenceElement->Accept( &xml_printer );

                    if(ProtSend2(xml_printer.CStr(), xml_printer.Size()) != 0)
                        return FALSE;
                }
            }
            pthread_rwlock_unlock(&m_online_list_lock); //relase lock
        }
    }

    return TRUE;
}

BOOL CXmpp::MessageTag(TiXmlDocument* xmlDoc)
{
    char xmpp_buf[2048];

    TiXmlElement * pMessageElement = xmlDoc->RootElement();

    if(pMessageElement)
    {

        if(m_auth_success)
        {
            TiXmlElement pReponseElement(*pMessageElement);

            char xmpp_from[1024];
            if(m_resource == "")
                sprintf(xmpp_from, "%s@%s",
                    m_username.c_str(), CMailBase::m_email_domain.c_str());
            else
                sprintf(xmpp_from, "%s@%s/%s",
                    m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());

            pReponseElement.SetAttribute("from", xmpp_from);

            string xmpp_to = pReponseElement.Attribute("to") ? pReponseElement.Attribute("to") : "";

            TiXmlPrinter xml_printer;
            xml_printer.SetIndent("");
            pReponseElement.Accept( &xml_printer );

            string strid = "";
            if(strstr(xmpp_to.c_str(), "@") != NULL)
            {
                strcut(xmpp_to.c_str(), NULL, "@", strid);
                strtrim(strid);
            }
            if(strid != "")
            {
                pthread_rwlock_rdlock(&m_online_list_lock); //acquire read

                map<string, CXmpp*>::iterator it = m_online_list.find(strid);

                if(it != m_online_list.end())
                {
                    CXmpp* pXmpp = it->second;
                    pXmpp->ProtSend2(xml_printer.CStr(), xml_printer.Size());
                }
                else
                {
                    MailStorage* mailStg;
                    StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
                    if(!mailStg)
                    {
                        fprintf(stderr, "%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
                        pthread_rwlock_unlock(&m_online_list_lock);
                        return FALSE;
                    }

                    mailStg->InsertMyMessage(xmpp_from, xmpp_to.c_str(), xml_printer.CStr());

                    stgengine_instance.Release();
                }
                pthread_rwlock_unlock(&m_online_list_lock);
            }
        }

    }

    return TRUE;
}

BOOL CXmpp::AuthTag(TiXmlDocument* xmlDoc)
{
    TiXmlElement * pAuthElement = xmlDoc->RootElement();
    if(pAuthElement)
    {
        char xmpp_buf[2048];
        
        const char* szMechanism = pAuthElement->Attribute("mechanism");
        if(szMechanism && strcmp(szMechanism, "PLAIN") == 0)
        {
            string strEnoded = pAuthElement->GetText();
            int outlen = BASE64_DECODE_OUTPUT_MAX_LEN(strEnoded.length());//strEnoded.length()*4+1;
            char* tmp1 = (char*)malloc(outlen);
            memset(tmp1, 0, outlen);

            CBase64::Decode((char*)strEnoded.c_str(), strEnoded.length(), tmp1, &outlen);

            m_username = tmp1 + 1;
            m_password = tmp1 + 1 + strlen(tmp1 + 1) + 1;
            free(tmp1);
            MailStorage* mailStg;
            StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
            if(!mailStg)
            {
                fprintf(stderr, "%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
                return FALSE;
            }
            
            string password;
            if(mailStg->GetPassword((char*)m_username.c_str(), password) == 0 && password == m_password)
            {
                sprintf(xmpp_buf, "<success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'></success>");
                m_auth_success = TRUE;

                pthread_rwlock_rdlock(&m_online_list_lock); //acquire read

                map<string, CXmpp*>::iterator it = m_online_list.find(m_username);

                if(it != m_online_list.end())
                {
                    CXmpp* pXmpp = it->second;
                    pXmpp->ProtSend2("</stream:stream>", _CSL_("</stream:stream>"));
                }

                pthread_rwlock_unlock(&m_online_list_lock);

            }
            else
            {
                 sprintf(xmpp_buf,
                     "<failure xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>"
                        "<not-authorized/>"
                     "</failure>");
                 m_auth_success = FALSE;
            }
            stgengine_instance.Release();

            if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
                return FALSE;

            pthread_rwlock_wrlock(&m_online_list_lock); //acquire write
            m_online_list.insert(make_pair<string, CXmpp*>(m_username, this));
            pthread_rwlock_unlock(&m_online_list_lock); //release write

            //recieve all the offline message
            if(m_auth_success)
            {

                MailStorage* mailStg;
                StorageEngineInstance stgengine_instance(GetStg(), &mailStg);
                if(!mailStg)
                {
                    fprintf(stderr, "%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
                    return FALSE;
                }

                string myAddr = GetUsername();
                myAddr += "@";
                myAddr += CMailBase::m_email_domain;

                vector<int> xids;
                string strMessage;
                mailStg->ListMyMessage(myAddr.c_str(), strMessage, xids);
                stgengine_instance.Release();

                if(xids.size() > 0)
                {
                    if(ProtSend2(strMessage.c_str(), strMessage.length()) >= 0)
                    {
                        for(int v = 0; v < xids.size(); v++)
                        {
                            mailStg->RemoveMyMessage(xids[v]);
                        }
                    }
                }
            }
        }
        else if(szMechanism && strcmp(szMechanism, "DIGEST-MD5") == 0)
        {
            char cmd[1024];
            char nonce[15];
            
            sprintf(nonce, "%c%c%c%08x%c%c%c",
                CHAR_TBL[random()%(sizeof(CHAR_TBL)-1)],
                CHAR_TBL[random()%(sizeof(CHAR_TBL)-1)],
                CHAR_TBL[random()%(sizeof(CHAR_TBL)-1)],
                (unsigned int)time(NULL),
                CHAR_TBL[random()%(sizeof(CHAR_TBL)-1)],
                CHAR_TBL[random()%(sizeof(CHAR_TBL)-1)],
                CHAR_TBL[random()%(sizeof(CHAR_TBL)-1)]);
            
            m_strDigitalMD5Nonce = nonce;
            
            sprintf(cmd, "realm=\"%s\",nonce=\"%s\",qop=\"auth\",charset=utf-8,algorithm=md5-sess", m_email_domain.c_str(), nonce);
            
           
            int outlen  = BASE64_ENCODE_OUTPUT_MAX_LEN(strlen(cmd));//strlen(cmd) *4 + 1;
            char* szEncoded = (char*)malloc(outlen);
            memset(szEncoded, 0, outlen);
            
            CBase64::Encode(cmd, strlen(cmd), szEncoded, &outlen);
            if(ProtSend2("<challenge xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>", strlen("<challenge xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>")) != 0)
            {
                free(szEncoded);
                return FALSE;
            }
            if(ProtSend2(szEncoded, strlen(szEncoded)) != 0)
            {
                free(szEncoded);
                return FALSE;
            }
            if(ProtSend2("</challenge>", strlen("</challenge>")) != 0)
            {
                free(szEncoded);
                return FALSE;
            }

            free(szEncoded);
        }
        else
        {
            sprintf(xmpp_buf,
                     "<failure xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>"
                        "<not-authorized/>"
                     "</failure>");
            m_auth_success = FALSE;
            if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
                return FALSE;
        }
    }

    return TRUE;
}

BOOL CXmpp::ResponseTag(TiXmlDocument* xmlDoc)
{
    TiXmlElement * pResponseElement = xmlDoc->RootElement();
    if(pResponseElement)
    {
        char xmpp_buf[2048];
        const char* szXmlns = pResponseElement->Attribute("xmlns");
        if(szXmlns && strcmp(szXmlns, "urn:ietf:params:xml:ns:xmpp-sasl") == 0 &&
            !m_auth_success && m_strDigitalMD5Nonce != "" && m_strDigitalMD5Response == "")
        {
            string strEnoded = pResponseElement->GetText();
            int outlen = BASE64_DECODE_OUTPUT_MAX_LEN(strEnoded.length());//strEnoded.length()*4+1;
            char* tmp1 = (char*)malloc(outlen);
            memset(tmp1, 0, outlen);

            CBase64::Decode((char*)strEnoded.c_str(), strEnoded.length(), tmp1, &outlen);
            
            const char* authinfo = tmp1;
        
            map<string, string> DigestMap;
            char where = 'K'; /* K is for key, V is for value*/
            DigestMap.clear();
            string key, value;
            int x = 0;
            while(1)
            {
                if(authinfo[x] == ',' || authinfo[x] == '\0')
                {
                     _strtrim_dquote_(key);
                    if(key != "")
                    {
                        _strtrim_dquote_(value);
                        DigestMap[key] = value;
                        key = "";
                        value = "";
                    }
                    where = 'K';
                    
                }
                else if(where == 'K' && authinfo[x] == '=')
                {
                    where = 'V';
                }
                else
                {
                    if( where == 'K' )
                        key += (authinfo[x]);
                    else if( where == 'V' )
                        value += (authinfo[x]);
                }
                
                if(authinfo[x] == '\0')
                    break;
                x++;
            }
            free(tmp1);
            
            if(DigestMap["nonce"] != m_strDigitalMD5Nonce)
            {
                sprintf(xmpp_buf,
                    "<response xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>");
                m_auth_success = FALSE;

                if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
                    return FALSE;
                else
                    return TRUE;
                    
            }
            
            MailStorage* mailStg;
            StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
            if(!mailStg)
            {
                fprintf(stderr, "%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
                return FALSE;
            }
            
            string real_password;
            if(mailStg->GetPassword(DigestMap["username"].c_str(), real_password) == 0)
            {
                char * pszNonce = (char*)DigestMap["nonce"].c_str();
                char * pszCNonce = (char*)DigestMap["cnonce"].c_str();
                char * pszUser = (char*)DigestMap["username"].c_str();
                char * pszRealm = (char*)DigestMap["realm"].c_str();
                char * pszPass = (char*)real_password.c_str();
                char * pszAuthzid = (char*)DigestMap["authzid"].c_str();
                char * szNonceCount = (char*)DigestMap["nc"].c_str();
                char * pszQop = (char*)DigestMap["qop"].c_str();
                char * pszURI = (char*)DigestMap["digest-uri"].c_str();        
                HASHHEX HA1;
                HASHHEX HA2 = "";
                HASHHEX Response;
                HASHHEX ResponseAuth;
                
                DigestCalcHA1("md5-sess", pszUser, pszRealm, pszPass, pszNonce, pszCNonce, pszAuthzid, HA1);
                DigestCalcResponse(HA1, pszNonce, szNonceCount, pszCNonce, pszQop, "AUTHENTICATE", pszURI, HA2, Response);
                if(strncmp(Response, DigestMap["response"].c_str(), 32) == 0)
                {
                    m_strDigitalMD5Response = Response;
                    m_username = DigestMap["username"].c_str();
                    DigestCalcResponse(HA1, pszNonce, szNonceCount, pszCNonce, pszQop,
                        "" /* at server site, no need "AUTHENTICATE" */, pszURI, HA2, ResponseAuth);
                    
                    char rspauth[64];
                    
                    sprintf(rspauth, "rspauth=%s",ResponseAuth);
            
                    int outlen  = BASE64_ENCODE_OUTPUT_MAX_LEN(strlen(rspauth));//strlen(rspauth) *4 + 1;
                    char* szEncoded = (char*)malloc(outlen);
                    memset(szEncoded, 0, outlen);
                    
                    CBase64::Encode(rspauth, strlen(rspauth), szEncoded, &outlen);
                    if(ProtSend2("<challenge xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>", strlen("<challenge xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>")) != 0)
                    {
                        free(szEncoded);
                        return FALSE;
                    }
                    if(ProtSend2(szEncoded, strlen(szEncoded)) != 0)
                    {
                        free(szEncoded);
                        return FALSE;
                    }
                    if(ProtSend2("</challenge>", strlen("</challenge>")) != 0)
                    {
                        free(szEncoded);
                        return FALSE;
                    }

                    free(szEncoded);
                }
                else
                {
                    sprintf(xmpp_buf,
                     "<response xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>");
                    m_auth_success = FALSE;
                    
                    if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
                        return FALSE;
                }
            }            
        }
        else if(szXmlns && strcmp(szXmlns, "urn:ietf:params:xml:ns:xmpp-sasl") == 0 &&
            !m_auth_success && m_strDigitalMD5Nonce != "" && m_strDigitalMD5Response != "")
        {
            sprintf(xmpp_buf, "<success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>");
                     
            if(ProtSend2(xmpp_buf, strlen(xmpp_buf)) != 0)
                return FALSE;
            
            m_auth_success = TRUE;
        }
    }
    
    return TRUE;
}
