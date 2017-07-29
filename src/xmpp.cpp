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
#include "xmpp.h"

bool xmpp_stanza::Parse(const char* text)
{
    m_xml_text += text;

    bool ret = m_xml.LoadString(m_xml_text.c_str(), m_xml_text.length());
    if(ret)
    {
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
                    sprintf(xmpp_buf, "%s@%s/%s",
                        m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());

                    pPresenceElement->SetAttribute("from", xmpp_buf);

                    sprintf(xmpp_buf, "%s@%s/%s",
                        buddys[x].c_str(), CMailBase::m_email_domain.c_str(), pXmpp->GetResource());
                    pPresenceElement->SetAttribute("to", xmpp_buf);

                    pPresenceElement->Accept( &xml_printer );

                    pXmpp->XmppSend(xml_printer.CStr(), xml_printer.Size());
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

    pthread_mutex_unlock(&m_send_lock);

    return ret;
}

int CXmpp::ProtRecv(char* buf, int len)
{
    if(m_ssl)
		return m_lssl->xrecv(buf, len);
	else
		return m_lsockfd->xrecv(buf, len);
}

BOOL CXmpp::Parse(char* text)
{
    BOOL result = TRUE;

    if(strstr(text, "<?xml ") != 0)
    {
        m_xml_declare = text;
    }
    else if(strstr(text, "<stream:stream>") != NULL
         || strstr(text, "<stream:stream ") != NULL)
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

    }
    else if(strstr(text, "</stream:stream>") != NULL)
    {
        if(!m_xmpp_stanza)
          m_xmpp_stanza = new xmpp_stanza(m_xml_declare.c_str());

        string xml_padding = "<stream:stream>";
        xml_padding += text;
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

        result = FALSE;
    }
    else
    {
      if(!m_xmpp_stanza)
          m_xmpp_stanza = new xmpp_stanza(m_xml_declare.c_str());

      if(m_xmpp_stanza->Parse(text))
      {
        if(strcasecmp(m_xmpp_stanza->GetTag(), "auth") == 0)
        {
          if(!AuthTag(m_xmpp_stanza->GetXml()))
              result = FALSE;
        }
        else if(strcasecmp(m_xmpp_stanza->GetTag(), "iq") == 0)
        {
          if(!IqTag(m_xmpp_stanza->GetXml()))
              result = FALSE;
        }
        else if(strcasecmp(m_xmpp_stanza->GetTag(), "presence") == 0)
        {
          if(!PresenceTag(m_xmpp_stanza->GetXml()))
              result = FALSE;
        }
        else if(strcasecmp(m_xmpp_stanza->GetTag(), "message") == 0)
        {
          if(!MessageTag(m_xmpp_stanza->GetXml()))
              result = FALSE;
        }
        else
        {
          if(m_auth_success)
          {
            char xmpp_buf[1024];
            const char* sz_from = m_xmpp_stanza->GetFrom();
            if(!sz_from || sz_from[0] == '\0')
            {
              sprintf(xmpp_buf, "%s@%s/%s",
                  m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());
              m_xmpp_stanza->SetFrom(xmpp_buf);
              sz_from = xmpp_buf;
            }

            const char* sz_to = m_xmpp_stanza->GetTo();
            string strid = "";
            if(strstr(sz_to, "@") != NULL)
            {
                strcut(sz_to, NULL, "@", strid);
                strtrim(strid);
            }
            if(strid != "")
            {
                TiXmlPrinter xml_printer;
                m_xmpp_stanza->GetXml()->Accept( &xml_printer );

                pthread_rwlock_rdlock(&m_online_list_lock); //acquire read
                map<string, CXmpp*>::iterator it = m_online_list.find(strid);

                if(it != m_online_list.end())
                {
                    CXmpp* pXmpp = it->second;
                    pXmpp->XmppSend(xml_printer.CStr(), xml_printer.Size());
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

                    mailStg->InsertMyMessage(sz_from, sz_to, xml_printer.CStr());

                    stgengine_instance.Release();
                }
                pthread_rwlock_unlock(&m_online_list_lock);
            }
          }
        }
              
        delete m_xmpp_stanza;
        m_xmpp_stanza = NULL;
      }
    }
	return result;
}

BOOL CXmpp::StreamTag(TiXmlDocument* xmlDoc)
{
    if(XmppSend(m_xml_declare.c_str(), m_xml_declare.length()) != 0)
        return FALSE;

    char stream_id[128];
    sprintf(stream_id, "%s_%u_%p_%d", CMailBase::m_localhostname.c_str(), getpid(), this, m_stream_count++);

    MD5_CTX context;
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
    if(XmppSend(xmpp_buf, strlen(xmpp_buf)) != 0)
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

        if(XmppSend(xmpp_buf, strlen(xmpp_buf)) != 0)
            return FALSE;
    }
    else
    {
        sprintf(xmpp_buf,
          "<stream:features>"
            "<mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>"
                "<mechanism>PLAIN</mechanism>"
            "</mechanisms>"
          "</stream:features>");
        if(XmppSend(xmpp_buf, strlen(xmpp_buf)) != 0)
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
        sprintf(xmpp_buf, "%s@%s/%s",
            m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());

        pReponseElement.SetAttribute("from", xmpp_buf);

        TiXmlPrinter xml_printer;
        if(strid != "")
        {
            pReponseElement.Accept( &xml_printer );

            pthread_rwlock_rdlock(&m_online_list_lock); //acquire read
            map<string, CXmpp*>::iterator it = m_online_list.find(strid);

            if(it != m_online_list.end())
            {
                CXmpp* pXmpp = it->second;
                pXmpp->XmppSend(xml_printer.CStr(), xml_printer.Size());
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

                        pXmpp->XmppSend(xml_printer.CStr(), xml_printer.Size());
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
                sprintf(xmpp_from, "%s@%s/%s",
                    m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());

                pReponseElement.SetAttribute("from", xmpp_from);

                pReponseElement.Accept( &xml_printer );

                pthread_rwlock_rdlock(&m_online_list_lock); //acquire read

                map<string, CXmpp*>::iterator it = m_online_list.find(strid);

                if(it != m_online_list.end())
                {
                    CXmpp* pXmpp = it->second;
                    pXmpp->XmppSend(xml_printer.CStr(), xml_printer.Size());
                }
                pthread_rwlock_unlock(&m_online_list_lock);
            }
            return TRUE;
        }

        iq_type itype = IQ_UNKNOW;
        string iq_id = pIqElement->Attribute("id") ? pIqElement->Attribute("id") : "";
        if(pIqElement->Attribute("type"))
        {
            if(strcasecmp(pIqElement->Attribute("type"), "set") == 0)
            {
                itype = IQ_SET;
            }
            else if(strcasecmp(pIqElement->Attribute("type"), "get") == 0)
            {
                itype = IQ_GET;
            }
            else if(strcasecmp(pIqElement->Attribute("type"), "result") == 0)
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

        if(XmppSend(xmpp_buf, strlen(xmpp_buf)) != 0)
            return FALSE;

        TiXmlNode* pChildNode = pIqElement->FirstChild();
        while(pChildNode)
        {
            if(pChildNode && pChildNode->ToElement())
            {
                if(strcasecmp(pChildNode->Value(), "bind") == 0)
                {
                    TiXmlNode* pGrandChildNode = pChildNode->ToElement()->FirstChild();
                    if(pGrandChildNode->ToElement())
                    {
                        if(strcasecmp(pGrandChildNode->Value(), "resource") == 0)
                        {
                            if(itype == IQ_SET)
                                m_resource = pGrandChildNode->ToElement()->GetText() ? pGrandChildNode->ToElement()->GetText() : "";

                            sprintf(xmpp_buf,
                                "<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'>"
                                    "<jid>%s@%s/%s</jid>"
                                "</bind>",
                                m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());
                            if(XmppSend(xmpp_buf, strlen(xmpp_buf)) != 0)
                                return FALSE;
                        }

                    }
                }
                else if(strcasecmp(pChildNode->Value(), "query") == 0)
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

                        if(XmppSend(xmpp_buddys, strlen(xmpp_buddys)) != 0)
                            return FALSE;

                        free(xmpp_buddys);

                        isPresenceProbe = TRUE;

                    }
                    else if(strcasecmp(xmlns.c_str(), "http://jabber.org/protocol/disco#items") == 0)
                    {
                       sprintf(xmpp_buf,
                            "<query xmlns='%s'/>", xmlns.c_str());
                       if(XmppSend(xmpp_buf, strlen(xmpp_buf)) != 0)
                           return FALSE;

                    }
                    else if(strcasecmp(xmlns.c_str(), "http://jabber.org/protocol/disco#info") == 0)
                    {
                       sprintf(xmpp_buf,
                            "<query xmlns='%s'>"
                            "</query>", xmlns.c_str());
                        if(XmppSend(xmpp_buf, strlen(xmpp_buf)) != 0)
                            return FALSE;
                    }
                    else
                    {
                        sprintf(xmpp_buf,
                            "<query xmlns='%s'/>",xmlns.c_str());
                        if(XmppSend(xmpp_buf, strlen(xmpp_buf)) != 0)
                            return FALSE;
                    }

                }
                else if(strcasecmp(pChildNode->Value(), "session") == 0)
                {

                }
                else if(strcasecmp(pChildNode->Value(), "ping") == 0)
                {
                   string xmlns = pChildNode->ToElement() && pChildNode->ToElement()->Attribute("xmlns") ? pChildNode->ToElement()->Attribute("xmlns") : "";
                   sprintf(xmpp_buf,
                        "<ping xmlns='%s'/>",xmlns.c_str());
                    if(XmppSend(xmpp_buf, strlen(xmpp_buf)) != 0)
                        return FALSE;
                }
                else if(strcasecmp(pChildNode->Value(), "vCard") == 0 )
                {
                    string xmlns = pChildNode->ToElement() && pChildNode->ToElement()->Attribute("xmlns") ? pChildNode->ToElement()->Attribute("xmlns") : "";
                    sprintf(xmpp_buf,
                            "<vCard xmlns='%s'/>", xmlns.c_str());
                    if(XmppSend(xmpp_buf, strlen(xmpp_buf)) != 0)
                        return FALSE;
                }
                else
                {
                    TiXmlPrinter xml_printer;
                    pChildNode->Accept( &xml_printer );

                    if(XmppSend(xml_printer.CStr(), xml_printer.Size()) != 0)
                        return FALSE;
                }
            }
            pChildNode = pChildNode->NextSibling();
        }

        if(XmppSend("</iq>", strlen("</iq>")) != 0)
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

                    sprintf(xmpp_buf, "%s@%s/%s",
                        m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());

                    pPresenceElement->SetAttribute("to", xmpp_buf);

                    CXmpp* pXmpp = it->second;

                    sprintf(xmpp_buf, "%s@%s/%s",
                        buddys[x].c_str(), CMailBase::m_email_domain.c_str(), pXmpp->GetResource());
                    pPresenceElement->SetAttribute("from", xmpp_buf);

                    TiXmlPrinter xml_printer;
                    pPresenceElement->Accept( &xml_printer );

                    if(XmppSend(xml_printer.CStr(), xml_printer.Size()) != 0)
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
            sprintf(xmpp_from, "%s@%s/%s",
                m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());

            pReponseElement.SetAttribute("from", xmpp_from);

            string xmpp_to = pReponseElement.Attribute("to") ? pReponseElement.Attribute("to") : "";

            TiXmlPrinter xml_printer;
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
                    pXmpp->XmppSend(xml_printer.CStr(), xml_printer.Size());
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
        char xmpp_buf[2048];
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
                pXmpp->XmppSend("</stream:stream>", _CSL_("</stream:stream>"));
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

        if(XmppSend(xmpp_buf, strlen(xmpp_buf)) != 0)
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
                if(XmppSend(strMessage.c_str(), strMessage.length()) >= 0)
                {
                    for(int v = 0; v < xids.size(); v++)
                    {
                        mailStg->RemoveMyMessage(xids[v]);
                    }
                }
            }
        }
    }

    return TRUE;
}
