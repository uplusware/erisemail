/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "util/general.h"
#include "xmpp.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "util/md5.h"
#include "service.h"

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
    m_state_machine = S_XMPP_INIT;
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
                    
                    //printf("%s: %s\n", buddys[x].c_str(), xml_printer.CStr());
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
	//printf("[%s]: %s\n", m_username.c_str(), buf);
    
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
		return m_lssl->drecv(buf, len);
	else
		return m_lsockfd->drecv(buf, len);
}

BOOL CXmpp::Parse(char* text)
{
    BOOL result = TRUE;
    //printf(" IN: %s\n", text);
    if(m_xml_declare == "")
    {
        m_xml_declare = text;

    }
    else
    {
        if(strcasecmp(text, "</stream:stream>") == 0)
        {
            result = FALSE;
        }
        else if(strncasecmp(text, "<stream:stream ", _CSL_("<stream:stream ")) == 0)
        {
            
            if(!StreamTag(text))
                return FALSE;
        }
        else if(strncasecmp(text, "<auth ", _CSL_("<auth ")) == 0)
        {
            m_state_machine = S_XMPP_AUTHING;
            m_xml_stanza = text;
        }
        else if(m_state_machine == S_XMPP_AUTHING
            && strlen(text) >= _CSL_("</auth>")
            && strcasecmp(text + strlen(text) - _CSL_("</auth>"), "</auth>") == 0)
        {
            m_xml_stanza += text;
            m_state_machine = S_XMPP_AUTHED;
            
            if(!AuthTag(m_xml_stanza.c_str()))
                return FALSE;
            
        }
        else if(strncasecmp(text, "<iq ", _CSL_("<iq ")) == 0)
        {
            m_state_machine = S_XMPP_IQING;
            m_xml_stanza = text;
        }
        else if(m_state_machine == S_XMPP_IQING
            && strlen(text) >= _CSL_("</iq>")
            && strcasecmp(text + strlen(text) - _CSL_("</iq>"), "</iq>") == 0)
        {
            m_xml_stanza += text;
            m_state_machine = S_XMPP_IQED;
            
            if(!IqTag(m_xml_stanza.c_str()))
                return FALSE;
        }
        else if(strncasecmp(text, "<presence>", _CSL_("<presence>")) == 0 || strncasecmp(text, "<presence ", _CSL_("<presence ")) == 0)
        {
            m_state_machine = S_XMPP_PRESENCEING;
            m_xml_stanza = text;
            
            if(strcasecmp(text + strlen(text) - _CSL_("/>"), "/>") == 0)
            {
                m_state_machine = S_XMPP_PRESENCED;
                
                if(!PresenceTag(m_xml_stanza.c_str()))
                    return FALSE;
            }
        }
        else if(m_state_machine == S_XMPP_PRESENCEING
            && strlen(text) >= _CSL_("</presence>")
            && strcasecmp(text + strlen(text) - _CSL_("</presence>"), "</presence>") == 0)
        {
            
            m_xml_stanza += text;
            m_state_machine = S_XMPP_PRESENCED;
            
            if(!PresenceTag(m_xml_stanza.c_str()))
                return FALSE;
            
        }
        else if(strncasecmp(text, "<message>", _CSL_("<message>")) == 0 || strncasecmp(text, "<message ", _CSL_("<message ")) == 0)
        {
            m_state_machine = S_XMPP_MESSAGEING;
            m_xml_stanza = text;
        }
        else if(m_state_machine == S_XMPP_MESSAGEING
            && strlen(text) >= _CSL_("</message>")
            && strcasecmp(text + strlen(text) - _CSL_("</message>"), "</message>") == 0)
        {
            m_xml_stanza += text;
            m_state_machine = S_XMPP_MESSAGED;
            
            if(!MessageTag(m_xml_stanza.c_str()))
                return FALSE;
        }
        else
        {
            if(m_state_machine == S_XMPP_AUTHING)
            {
                m_xml_stanza += text;
            }
            else if(m_state_machine == S_XMPP_IQING)
            {
                m_xml_stanza += text;
            }
            else if(m_state_machine == S_XMPP_PRESENCEING)
            {
                m_xml_stanza += text;
            }
            else if(m_state_machine == S_XMPP_MESSAGEING)
            {
                m_xml_stanza += text;
            }
        }
    }
	
	return result;
}

BOOL CXmpp::StreamTag(const char* text)
{
    m_xml_stream = text;
    XmppSend(m_xml_declare.c_str(), m_xml_declare.length());
    
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
    sprintf(xmpp_buf, "<stream:stream from='%s' id='%s' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' version='1.0'>", CMailBase::m_email_domain.c_str(), m_stream_id);
    XmppSend(xmpp_buf, strlen(xmpp_buf));
    
    if(m_state_machine == S_XMPP_AUTHED)
    {
        sprintf(xmpp_buf,
        "<stream:features>"
            "<compression xmlns='http://jabber.org/features/compress'>"
                "<method>zlib</method>"
            "</compression>"
            "<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'/>"
            "<session xmlns='urn:ietf:params:xml:ns:xmpp-session'/>"
        "</stream:features>");
        
        XmppSend(xmpp_buf, strlen(xmpp_buf));
    }
    else
    {
        sprintf(xmpp_buf, "<stream:features>"
            "<mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>"
                "<mechanism>PLAIN</mechanism>"
            "</mechanisms>"
        "</stream:features>");
        XmppSend(xmpp_buf, strlen(xmpp_buf));

    }
    
    return TRUE;
}

BOOL CXmpp::PresenceTag(const char* text)
{
    //printf("%s\n", text);
    char xmpp_buf[2048];
            
    TiXmlDocument xmlPresence;
    xmlPresence.LoadString(m_xml_stanza.c_str(), m_xml_stanza.length());
    
    TiXmlElement * pPresenceElement = xmlPresence.RootElement();
    
    if(pPresenceElement)
    {
        presence_type p_type = PRESENCE_AVAILABLE;
        string iq_id = pPresenceElement->Attribute("id") ? pPresenceElement->Attribute("id") : "";
        
        string str_p_type = pPresenceElement->Attribute("type") ? pPresenceElement->Attribute("type") : "";
        
        if(str_p_type == "subscribe")
        {
            p_type= PRESENCE_SBSCRIBE;
        }
        else if(str_p_type == "available")
        {
            p_type= PRESENCE_AVAILABLE;
        }
        else if(str_p_type == "unavailable")
        {
            p_type= PRESENCE_UNAVAILABLE;
        }
        else if(str_p_type == "subscribed")
        {
            p_type= PRESENCE_SBSCRIBED;
        }
        else if(str_p_type == "unsubscribe")
        {
            p_type= PRESENCE_UNSBSCRIBE;
        }
        else if(str_p_type == "unsubscribed")
        {
            p_type= PRESENCE_UNSBSCRIBED;
        }
        else if(str_p_type == "probe")
        {
            p_type= PRESENCE_PROBE;
        }
        
        string str_to = pPresenceElement->Attribute("to") ? pPresenceElement->Attribute("to") : "";
        
        
        TiXmlNode* pChildNode = pPresenceElement->FirstChild();
        while(pChildNode)
        {
            if(pChildNode && pChildNode->ToElement())
            {
                if(strcasecmp(pChildNode->Value(), "priority") == 0)
                {
                }
                else if(strcasecmp(pChildNode->Value(), "show") == 0)
                {
                }
                else if(strcasecmp(pChildNode->Value(), "status") == 0)
                {
                }
                else if(strcasecmp(pChildNode->Value(), "c") == 0)
                {
                }
                else if(strcasecmp(pChildNode->Value(), "x") == 0)
                {
                     TiXmlNode* pGrandChildNode = pChildNode->ToElement()->FirstChild();
                     if(strcasecmp(pGrandChildNode->Value(), "photo") == 0)
                     {
                     }
                }
            }
            pChildNode = pChildNode->NextSibling();
        }
        
        string strid;
        strcut(str_to.c_str(), NULL, "@", strid);
        
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
        
        TiXmlElement pReponseElement(*pPresenceElement);
        
        if(iq_id != "")
            pReponseElement.SetAttribute("id", iq_id.c_str());
        
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
                        
                        //printf("%s: %s\n", buddys[x].c_str(), xml_printer.CStr());
                    }
                }
                pthread_rwlock_unlock(&m_online_list_lock);
            }
        }
        
    }
    
    return TRUE;
}

BOOL CXmpp::IqTag(const char* text)
{
    TiXmlDocument xmlIq;
    xmlIq.LoadString(text, strlen(text));
                    
    TiXmlElement * pIqElement = xmlIq.RootElement();
    if(pIqElement)
    {
        
        TiXmlElement pReponseElement(*pIqElement);
        TiXmlPrinter xml_printer;
        
        if(pIqElement->Attribute("to")
            && strcmp(pIqElement->Attribute("to"), "") != 0 
            && strcmp(pIqElement->Attribute("to"), CMailBase::m_email_domain.c_str()) != 0)
        {
            string strid;
            strcut(pIqElement->Attribute("to"), NULL, "@", strid);

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
                
                //printf("%s\n\n", xml_printer.CStr());
            }
            pthread_rwlock_unlock(&m_online_list_lock);
            
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
        
        char xmpp_buf[2048];
        sprintf(xmpp_buf,
            "<iq type='result' id='%s' from='%s' to='%s/%s'>",
                iq_id.c_str(), CMailBase::m_email_domain.c_str(), m_username.c_str(), m_stream_id);
                        
        XmppSend(xmpp_buf, strlen(xmpp_buf));
        
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
                            XmppSend(xmpp_buf, strlen(xmpp_buf));
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
                        
                        XmppSend(xmpp_buddys, strlen(xmpp_buddys));
                        
                        free(xmpp_buddys);
                        
                        isPresenceProbe = TRUE;
                            
                    }
                    else if(strcasecmp(xmlns.c_str(), "http://jabber.org/protocol/disco#items") == 0)
                    {
                       sprintf(xmpp_buf,
                            "<query xmlns='%s'/>", xmlns.c_str());
                       XmppSend(xmpp_buf, strlen(xmpp_buf));

                    }
                    else if(strcasecmp(xmlns.c_str(), "http://jabber.org/protocol/disco#info") == 0)
                    {
                       sprintf(xmpp_buf,
                            "<query xmlns='%s'/>", xmlns.c_str());
                        XmppSend(xmpp_buf, strlen(xmpp_buf));
                    }
                    else
                    {
                        sprintf(xmpp_buf,
                            "<query xmlns='%s'/>",xmlns.c_str());
                        XmppSend(xmpp_buf, strlen(xmpp_buf));
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
                    XmppSend(xmpp_buf, strlen(xmpp_buf));
                }
                else if(strcasecmp(pChildNode->Value(), "vCard") == 0 )
                {
                    string xmlns = pChildNode->ToElement() && pChildNode->ToElement()->Attribute("xmlns") ? pChildNode->ToElement()->Attribute("xmlns") : "";
                    sprintf(xmpp_buf,
                            "<vCard xmlns='%s'/>", xmlns.c_str());
                    XmppSend(xmpp_buf, strlen(xmpp_buf));
                }
                else
                {
                    TiXmlPrinter xml_printer;
                    pChildNode->Accept( &xml_printer );
                    
                    XmppSend(xml_printer.CStr(), xml_printer.Size());
                }
            }
            pChildNode = pChildNode->NextSibling();
        }
        
        XmppSend("</iq>", strlen("</iq>"));
        
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
                    
                    XmppSend(xml_printer.CStr(), xml_printer.Size());
                }
            }
            pthread_rwlock_unlock(&m_online_list_lock); //relase lock
        }
    }
    
    return TRUE;
}

BOOL CXmpp::MessageTag(const char* text)
{
    char xmpp_buf[2048];
            
    TiXmlDocument xmlPresence;
    xmlPresence.LoadString(m_xml_stanza.c_str(), m_xml_stanza.length());
    
    TiXmlElement * pMessageElement = xmlPresence.RootElement();
    
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
            
            string strid;
            strcut(xmpp_to.c_str(), NULL, "@", strid);
            
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
    
    return TRUE;
}

BOOL CXmpp::AuthTag(const char* text)
{
    TiXmlDocument xmlAuth;
    xmlAuth.LoadString(m_xml_stanza.c_str(), m_xml_stanza.length());
    
    TiXmlElement * pAuthElement = xmlAuth.RootElement();
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
        
        XmppSend(xmpp_buf, strlen(xmpp_buf));
        
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