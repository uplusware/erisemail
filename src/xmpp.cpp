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

CXmpp::CXmpp(int sockfd, SSL * ssl, SSL_CTX * ssl_ctx, const char* clientip,
    StorageEngine* storage_engine, memcached_st * memcached, BOOL isSSL)
{		
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
    
    if(m_bSTARTTLS)
	    close_ssl(m_ssl, m_ssl_ctx);
	if(m_lsockfd)
		delete m_lsockfd;

	if(m_lssl)
		delete m_lssl;
    
    m_lsockfd = NULL;
    m_lssl = NULL;

}

int CXmpp::XmppSend(const char* buf, int len)
{
	//printf("<<<< %s\n", buf);
    
    int ret;
    
    if(m_ssl)
        ret = SSLWrite(m_sockfd, m_ssl, buf, len);
    else
        ret = Send(m_sockfd, buf, len);
    
    return ret;
}

int CXmpp::ProtRecv(char* buf, int len)
{
    int recv_len;
    while(1)
    {
        if(m_ssl)
        {
            recv_len = m_lssl->drecv_timed(buf, len);
        }
        else
        {
            recv_len = m_lsockfd->drecv_timed(buf, len);
        }
        if(recv_len == 0 && m_auth_success)
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
                else
                    break;
            } 
        }
        if(recv_len != 0)
            break;
    }
    
    return recv_len;
}

BOOL CXmpp::Parse(char* text)
{
    BOOL result = TRUE;
    //printf(">>>> %s\n", text);
    if(m_xml_declare == "")
    {
        m_xml_declare = text;

    }
    else
    {
        if(strcasecmp(text, "</stream:stream>") == 0)
        {
             m_recv_pthread_exit = 0;
            result = FALSE;
        }
        else if(strncasecmp(text, "<stream:stream ", _CSL_("<stream:stream ")) == 0)
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
                    
                    /*pthread_attr_init(&m_recv_pthread_attr);
                    pthread_attr_setdetachstate (&m_recv_pthread_attr, PTHREAD_CREATE_DETACHED);
            
                    if(pthread_create(&m_recv_pthread, &m_recv_pthread_attr, CXmpp::xmpp_recv_main, this) != 0)
                    {
                        result = FALSE;
                        fprintf(stderr, "Create xmpp recv thread failed\n");
                    }*/
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
            }
            
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
            
            TiXmlDocument xmlIq;
            xmlIq.LoadString(m_xml_stanza.c_str(), m_xml_stanza.length());
            TiXmlElement * pIqElement = xmlIq.RootElement();
            if(pIqElement)
            {
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
                                if(strcasecmp(pGrandChildNode->Value(), "resource") == 0 && itype != IQ_RESULT)
                                {
                                    if(itype == IQ_SET)
                                        m_resource = pGrandChildNode->ToElement()->GetText();
                                    char xmpp_buf[2048];
                                    sprintf(xmpp_buf,
                                    "<iq type='result' id='%s' to='%s/%s'>"
                                        "<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'>"
                                            "<jid>%s@%s/%s</jid>"
                                        "</bind>"
                                    "</iq>", iq_id.c_str(), m_username.c_str(), m_stream_id, m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());
                                    XmppSend(xmpp_buf, strlen(xmpp_buf));
                                }
                                
                            }
                        }
                        else if(strcasecmp(pChildNode->Value(), "session") == 0 && itype != IQ_RESULT)
                        {
                            char xmpp_buf[2048];
                            sprintf(xmpp_buf,
                            "<iq type='result' id='%s' to='%s/%s'/>",
                                iq_id.c_str(), m_username.c_str(), m_stream_id);
                            XmppSend(xmpp_buf, strlen(xmpp_buf));
                        }
                        else if(strcasecmp(pChildNode->Value(), "ping") == 0 && itype != IQ_RESULT)
                        {
                           char xmpp_buf[2048];
                           string xmlns = pChildNode->ToElement() && pChildNode->ToElement()->Attribute("xmlns") ? pChildNode->ToElement()->Attribute("xmlns") : "";
                           sprintf(xmpp_buf,
                                "<iq type='result' id='%s'>"
                                "<ping xmlns='%s'/>"
                                "</iq>",
                                iq_id.c_str(), xmlns.c_str());
                            XmppSend(xmpp_buf, strlen(xmpp_buf));
                        }
                        else if(strcasecmp(pChildNode->Value(), "query") == 0 && itype != IQ_RESULT)
                        {
                            char xmpp_buf[2048];
                            string xmlns = pChildNode->ToElement() && pChildNode->ToElement()->Attribute("xmlns") ? pChildNode->ToElement()->Attribute("xmlns") : "";
                            
                            if(strcasecmp(xmlns.c_str(), "jabber:iq:roster") == 0)
                            {
                               sprintf(xmpp_buf,
                                    "<iq type='result' id='%s' from='%s' to='%s/%s'>"
                                    "<query xmlns='%s'/>"
                                    "</iq>",
                                    iq_id.c_str(), CMailBase::m_email_domain.c_str(), m_username.c_str(), m_stream_id, xmlns.c_str());
                            }
                            else if(strcasecmp(xmlns.c_str(), "http://jabber.org/protocol/disco#items") == 0)
                            {
                               sprintf(xmpp_buf,
                                    "<iq type='result' id='%s' from='%s' to='%s/%s'>"
                                    "<query xmlns='%s'/>"
                                    "</iq>",
                                    iq_id.c_str(), CMailBase::m_email_domain.c_str(), m_username.c_str(), m_stream_id, xmlns.c_str());

                            }
                            else if(strcasecmp(xmlns.c_str(), "http://jabber.org/protocol/disco#info") == 0)
                            {
                               sprintf(xmpp_buf,
                                    "<iq type='result' id='%s' from='%s' to='%s/%s'>"
                                    "<query xmlns='%s'/>"
                                    "</iq>",
                                    iq_id.c_str(), CMailBase::m_email_domain.c_str(), m_username.c_str(), m_stream_id, xmlns.c_str());

                            }
                            else
                            {
                                sprintf(xmpp_buf,
                                    "<iq type='result' id='%s' to='%s/%s'>"
                                    "<query xmlns='%s'/>"
                                    "</iq>",
                                    iq_id.c_str(), m_username.c_str(), m_stream_id, xmlns.c_str());
                            }
                            XmppSend(xmpp_buf, strlen(xmpp_buf));
                        }
                        else if(strcasecmp(pChildNode->Value(), "vCard") == 0  && itype != IQ_RESULT)
                        {
                            string xmlns = pChildNode->ToElement() && pChildNode->ToElement()->Attribute("xmlns") ? pChildNode->ToElement()->Attribute("xmlns") : "";
                            char xmpp_buf[2048];
                            sprintf(xmpp_buf,
                                    "<iq type='result' id='%s' to='%s/%s'>"
                                    "<vCard xmlns='%s'/>"
                                    "</iq>",
                                    iq_id.c_str(), m_username.c_str(), m_stream_id, xmlns.c_str());
                            XmppSend(xmpp_buf, strlen(xmpp_buf));
                        }
                    }
                    pChildNode = pChildNode->NextSibling();
                }
            }
        }
        else if(strncasecmp(text, "<presence>", _CSL_("<presence>")) == 0 || strncasecmp(text, "<presence ", _CSL_("<presence ")) == 0)
        {
            m_state_machine = S_XMPP_PRESENCEING;
            m_xml_stanza = text;
        }
        else if(m_state_machine == S_XMPP_PRESENCEING
            && strlen(text) >= _CSL_("</presence>")
            && strcasecmp(text + strlen(text) - _CSL_("</presence>"), "</presence>") == 0)
        {
            char xmpp_buf[2048];
            
            m_xml_stanza += text;
            m_state_machine = S_XMPP_PRESENCED;
            
            
            
            TiXmlDocument xmlPresence;
            xmlPresence.LoadString(m_xml_stanza.c_str(), m_xml_stanza.length());
            TiXmlElement * pPresenceElement = xmlPresence.RootElement();
            
            if(pPresenceElement)
            {
                string iq_id = pPresenceElement->Attribute("id") ? pPresenceElement->Attribute("id") : "";
                TiXmlNode* pChildNode = pPresenceElement->FirstChild();
                while(pChildNode)
                {
                    if(pChildNode && pChildNode->ToElement())
                    {
                        if(strcasecmp(pChildNode->Value(), "priority") == 0)
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
                
                TiXmlElement pReponseElement(*pPresenceElement);
                
                if(iq_id != "")
                    pReponseElement.SetAttribute("id", iq_id.c_str());
                
                char xmpp_buf[1024];
                sprintf(xmpp_buf, "%s@%s/%s",
                    m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());
                    
                pReponseElement.SetAttribute("from", xmpp_buf);
                
                sprintf(xmpp_buf, "%s@%s/%s",
                    m_username.c_str(), CMailBase::m_email_domain.c_str(), m_resource.c_str());
                
                pReponseElement.SetAttribute("to", xmpp_buf);
                
                TiXmlPrinter xml_printer;
                pReponseElement.Accept( &xml_printer );
            
                XmppSend(xml_printer.CStr(), xml_printer.Size());
                
            }
            
            
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
            char xmpp_buf[2048];
            
            m_xml_stanza += text;
            m_state_machine = S_XMPP_MESSAGED;
            
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
                    
                    MailStorage* mailStg;
                    StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
                    if(!mailStg)
                    {
                        fprintf(stderr, "%s::%s Get Storage Engine Failed!\n", __FILE__, __FUNCTION__);
                        return FALSE;
                    }
                        
                    mailStg->InsertMyMessage(xmpp_from, xmpp_to.c_str(), xml_printer.CStr());
                    
                    stgengine_instance.Release();
                
                    
                }
                
            }
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

