/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "util/general.h"
#include "simple_ldap.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "util/md5.h"
#include <stack>
#include "service.h"
#include "LDAPMessage.h"

/* Dump the buffer out to the specified FILE */
static int write_out(const void *buffer, size_t size, void *key)
{
    if(size > 0)
    {
        CLdap* ldap_session = (CLdap*)key;
        return (ldap_session->LdapSend((const char*)buffer, size) < 0) ? -1 : 0;
    }
    return 0;
}

CLdap::CLdap(int sockfd, SSL * ssl, SSL_CTX * ssl_ctx, const char* clientip,
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
	
	m_buf = NULL;
	m_buf_len = 0;
	m_data_len = 0;
    
    m_decoded_buffer = NULL;
    m_decoded_buffer_len = 0;
    m_decoded_data_len = 0;
}

CLdap::~CLdap()
{    
    if(m_bSTARTTLS)
	    close_ssl(m_ssl, m_ssl_ctx);
	if(m_lsockfd)
		delete m_lsockfd;

	if(m_lssl)
		delete m_lssl;
	
	if(m_buf)
		free(m_buf);
    
    if(m_decoded_buffer)
        free(m_decoded_buffer);
    
    m_decoded_buffer_len = 0;
    m_decoded_data_len = 0;
}

int CLdap::LdapSend(const char* buf, int len)
{
    if(m_ssl)
        return SSLWrite(m_sockfd, m_ssl, buf, len, CMailBase::m_connection_idle_timeout);
    else
        return Send(m_sockfd, buf, len, CMailBase::m_connection_idle_timeout);	
}

int CLdap::LdapDecodedBuffer(const char* buf, int len)
{
    if(!m_decoded_buffer)
    {
        m_decoded_buffer_len = len > 1024 ? len : 1024;
        m_decoded_buffer = (unsigned char*)malloc(m_decoded_buffer_len);
        m_decoded_data_len = 0;
    }
    else
    {
        if((m_decoded_data_len + len) > m_decoded_buffer_len)
        {
            m_decoded_buffer_len = m_decoded_data_len + len;
            
            unsigned char* new_buf = (unsigned char*)malloc(m_decoded_buffer_len);
            memcpy(new_buf, m_decoded_buffer, m_decoded_data_len);
            free(m_decoded_buffer);
            m_decoded_buffer = new_buf;
        }
    }
    
    memcpy(m_decoded_buffer + m_decoded_data_len, buf, len);
    
    m_decoded_data_len += len;
    
    return 0;
}


int CLdap::SendDecodedBuffer()
{
    int s = 0;
    if(m_decoded_buffer && m_decoded_data_len > 0)
    {
        s = LdapSend((const char*)m_decoded_buffer, m_decoded_data_len);
        free(m_decoded_buffer);
        m_decoded_buffer = NULL;
        m_decoded_data_len = 0;
        m_decoded_buffer_len = 0;
    }
    return s;
}

int CLdap::ProtRecv(char* buf, int len)
{
    if(m_ssl)
        return m_lssl->xrecv(buf, 1, CMailBase::m_connection_idle_timeout);
    else
        return m_lsockfd->xrecv(buf, 1, CMailBase::m_connection_idle_timeout);
}

BOOL CLdap::Parse(char* text, int len)
{
	if(!m_buf)
	{
		m_buf = (unsigned char*)malloc(len < 1024 ? 1024 : len);
		if(m_buf)
		{
			m_buf_len = len < 1024 ? 1024 : len;
			memcpy(m_buf, text, len);
			m_data_len = len;
		}
	}
	else
	{
		if(m_buf_len - m_data_len < len)
		{
			unsigned char* new_buf = (unsigned char*)malloc(m_data_len + len);
			memcpy(new_buf, m_buf, m_data_len);
			memcpy(new_buf + m_data_len, text, len);
			free(m_buf);
			m_buf = new_buf;
            m_buf_len = m_data_len + len;
		}
		else
		{
			memcpy(m_buf + m_data_len, text, len);
		}
		m_data_len += len;
	}
	
	if(m_data_len >= 2)
	{
		asn1_ber_tag ber_tag;
		ber_tag.payload = m_buf[0];
        
        if(ber_tag.tag.type != ASN_1_TYPE_UNIVERSAL || ber_tag.tag.val != ASN_1_SEQUENCE)
		{
			return FALSE;
		}
		
		unsigned int asn1_len_buf_len = 1;
		unsigned int asn1_len = 0;
		if((m_buf[1] & 0x80) == 0x80)
		{
			asn1_len_buf_len = (m_buf[1] & (~0x80)) + 1;
		}
		
		if(m_data_len >= (asn1_len_buf_len + 1))
		{
			if(asn1_len_buf_len == 1)
			{
				asn1_len = m_buf[1];
			}
			else
			{
				for(int x = 0; x < asn1_len_buf_len - 1; x++)
				{
					asn1_len <<= 8;
					asn1_len += m_buf[x + 2];
				}
			}
		}
        else
        {
            return TRUE;
        }
        
		if(m_data_len >= asn1_len + asn1_len_buf_len + 1 /*tag len*/)
		{            
            LDAPMessage_t * ldap_msg = NULL;
            asn_dec_rval_t rval;
            rval = ber_decode(0, &asn_DEF_LDAPMessage, (void**)&ldap_msg, m_buf, m_data_len);

            if(rval.code != RC_OK || !ldap_msg)
            {
                return FALSE;
            }
            
            if(ProtocolOp_PR_bindRequest == ldap_msg->protocolOp.present)
            {
                if(ldap_msg->protocolOp.choice.bindRequest.authentication.present != AuthenticationChoice_PR_simple)
                {
                    return FALSE;
                }
                char* name = (char*)malloc(ldap_msg->protocolOp.choice.bindRequest.name.size + 1);
                memcpy(name, ldap_msg->protocolOp.choice.bindRequest.name.buf, ldap_msg->protocolOp.choice.bindRequest.name.size);
                name[ldap_msg->protocolOp.choice.bindRequest.name.size] = '\0';
                
                char* password = (char*)malloc(ldap_msg->protocolOp.choice.bindRequest.authentication.choice.simple.size + 1);
                memcpy(password, ldap_msg->protocolOp.choice.bindRequest.authentication.choice.simple.buf, ldap_msg->protocolOp.choice.bindRequest.authentication.choice.simple.size);
                password[ldap_msg->protocolOp.choice.bindRequest.authentication.choice.simple.size] = '\0';
                
                string str_dn = name;
                string auth_name;
                string auth_pwd = password;
                
                free(password);
                free(name);
                
                
                strcut(str_dn.c_str(), "cn=", ",",auth_name);
                strtrim(auth_name);
                MailStorage* mailStg;
                StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
                
                if(mailStg && mailStg->CheckLogin(auth_name.c_str(), auth_pwd.c_str()) == 0)
                {
                    LDAPMessage_t* bind_response = (LDAPMessage_t*)calloc(1, sizeof(LDAPMessage_t));
                    
                    bind_response->messageID = ldap_msg->messageID;
                    bind_response->protocolOp.present = ProtocolOp_PR_bindResponse;
                    
                    asn_long2INTEGER(&bind_response->protocolOp.choice.bindResponse.resultCode, BindResponse__resultCode_success);
                    
                    OCTET_STRING_fromString(&bind_response->protocolOp.choice.bindResponse.matchedDN, "");
                    OCTET_STRING_fromString(&bind_response->protocolOp.choice.bindResponse.errorMessage, "");
                    
                    //xer_fprint(stderr, &asn_DEF_LDAPMessage, bind_response);
                    
                    asn_enc_rval_t erv;
                    erv = der_encode(&asn_DEF_LDAPMessage, bind_response, write_out, this);
                    if(erv.encoded < 0)
                    {
                        return FALSE;
                    }
                    //SendDecodedBuffer();
                    free(bind_response);
                }
                else
                {
                    return FALSE;
                }
                
            }
            else if(ProtocolOp_PR_unbindRequest == ldap_msg->protocolOp.present)
            {
                return FALSE;
            }
            else if(ProtocolOp_PR_searchRequest == ldap_msg->protocolOp.present)
            {
                SearchRequest_t* search_request = &ldap_msg->protocolOp.choice.searchRequest;
                char* baseObject = (char*)malloc(search_request->baseObject.size + 1);
                memcpy(baseObject, search_request->baseObject.buf, search_request->baseObject.size);
                baseObject[search_request->baseObject.size] = '\0';
                
                std::stack<Filter_t* > filter_stack;
                string str_filter = "";
                Filter_t* current_filter = &search_request->filter;
                while(current_filter->present == Filter_PR_and || current_filter->present == Filter_PR_or || current_filter->present == Filter_PR_substrings || filter_stack.size() > 0)
                {
                    if(current_filter->present == Filter_PR_and)
                    {
                        if(current_filter->choice.and_f.list.count > 0)
                        {
                            current_filter = current_filter->choice.and_f.list.array[0];
                            for(int t = 1; t < current_filter->choice.and_f.list.count; t++)
                            {
                                filter_stack.push(current_filter->choice.and_f.list.array[t]);
                            }
                        }
                        else
                            break;
                    }
                    else if(current_filter->present == Filter_PR_or)
                    {
                        if(current_filter->choice.and_f.list.count > 0)
                        {
                            current_filter = current_filter->choice.or_f.list.array[0];
                            for(int t = 1; t < current_filter->choice.and_f.list.count; t++)
                            {
                                filter_stack.push(current_filter->choice.and_f.list.array[t]);
                            }
                        }
                        else
                            break;
                    }
                    else if(current_filter->present == Filter_PR_substrings)
                    {
                        if(current_filter->choice.substrings.substrings.list.count > 0 && current_filter->choice.substrings.substrings.list.array[0]->present == SubstringFilter__substrings__Member_PR_any)
                        {
                            char* sz_filter = (char*)malloc(current_filter->choice.substrings.substrings.list.array[0]->choice.any.size + 1);
                            memcpy(sz_filter, current_filter->choice.substrings.substrings.list.array[0]->choice.any.buf, current_filter->choice.substrings.substrings.list.array[0]->choice.any.size);
                            sz_filter[current_filter->choice.substrings.substrings.list.array[0]->choice.any.size] = '\0';
                            
                            str_filter = sz_filter;
                            free(sz_filter);
                            if(str_filter != "")
                                break;
                        }
                       
                    }
                    else
                    {
                        current_filter = filter_stack.top();
                        filter_stack.pop();
                    }
                }
                
                MailStorage* mailStg;
                StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
                
                vector <User_Info> listtbl;
                if(mailStg && mailStg->FilterID(listtbl, str_filter.c_str(), "uname", TRUE) == 0)
                {
                    for(int x = 0; x< listtbl.size(); x++)
                    {
                        LDAPMessage_t* search_entry_response = (LDAPMessage_t*)calloc(1, sizeof(LDAPMessage_t));
                        
                        search_entry_response->messageID = ldap_msg->messageID;
                        search_entry_response->protocolOp.present = ProtocolOp_PR_searchResEntry;
                        
                        string object_name = "cn=";
                        object_name += listtbl[x].username;
                        object_name += ",";
                        object_name += baseObject;
                        
                        OCTET_STRING_fromString(&search_entry_response->protocolOp.choice.searchResEntry.objectName, object_name.c_str());
                        
                        struct PartialAttributeList__Member* new_attr_objectClass = (struct PartialAttributeList__Member*)calloc(1, sizeof(struct PartialAttributeList__Member));
                        
                        OCTET_STRING_fromString(&new_attr_objectClass->type, "objectClass");
                        
                        AttributeValue_t new_value_objectClass_top;
                        memset(&new_value_objectClass_top, 0, sizeof(AttributeValue_t));
                        OCTET_STRING_fromString(&new_value_objectClass_top, "top");
                        ASN_SET_ADD(&new_attr_objectClass->vals.list, &new_value_objectClass_top);
                        
                        AttributeValue_t new_value_objectClass_person;
                        memset(&new_value_objectClass_person, 0, sizeof(AttributeValue_t));
                        OCTET_STRING_fromString(&new_value_objectClass_person, "person");
                        ASN_SET_ADD(&new_attr_objectClass->vals.list, &new_value_objectClass_person);
                        
                        
                        AttributeValue_t new_value_objectClass_organizationalPerson;
                        memset(&new_value_objectClass_organizationalPerson, 0, sizeof(AttributeValue_t));
                        OCTET_STRING_fromString(&new_value_objectClass_organizationalPerson, "organizationalPerson");
                        ASN_SET_ADD(&new_attr_objectClass->vals.list, &new_value_objectClass_organizationalPerson);

                        AttributeValue_t new_value_objectClass_inetOrgPerson;
                        memset(&new_value_objectClass_inetOrgPerson, 0, sizeof(AttributeValue_t));
                        OCTET_STRING_fromString(&new_value_objectClass_inetOrgPerson, "inetOrgPerson");
                        ASN_SET_ADD(&new_attr_objectClass->vals.list, &new_value_objectClass_inetOrgPerson);
                        
                        
                        ASN_SEQUENCE_ADD(&search_entry_response->protocolOp.choice.searchResEntry.attributes.list, new_attr_objectClass);
                        
                        
                        struct PartialAttributeList__Member* new_attr_cn = (struct PartialAttributeList__Member*)calloc(1, sizeof(struct PartialAttributeList__Member));
                        
                        OCTET_STRING_fromString(&new_attr_cn->type, "cn");
                        
                        AttributeValue_t new_value_cn;
                        memset(&new_value_cn, 0, sizeof(AttributeValue_t));
                        OCTET_STRING_fromString(&new_value_cn, listtbl[x].username);
                        
                        ASN_SET_ADD(&new_attr_cn->vals.list, &new_value_cn);
                        
                        ASN_SEQUENCE_ADD(&search_entry_response->protocolOp.choice.searchResEntry.attributes.list, new_attr_cn);
                        
                        struct PartialAttributeList__Member* new_attr_sn = (struct PartialAttributeList__Member*)calloc(1, sizeof(struct PartialAttributeList__Member));
                        
                        OCTET_STRING_fromString(&new_attr_sn->type, "sn");
                        
                        AttributeValue_t new_value_sn;
                        memset(&new_value_sn, 0, sizeof(AttributeValue_t));
                        OCTET_STRING_fromString(&new_value_sn, "");
                        
                        ASN_SET_ADD(&new_attr_sn->vals.list, &new_value_sn);
                        
                        ASN_SEQUENCE_ADD(&search_entry_response->protocolOp.choice.searchResEntry.attributes.list, new_attr_sn);

                        struct PartialAttributeList__Member* new_attr_givenName = (struct PartialAttributeList__Member*)calloc(1, sizeof(struct PartialAttributeList__Member));
                        
                        OCTET_STRING_fromString(&new_attr_givenName->type, "givenName");
                        
                        AttributeValue_t new_value_givenName;
                        memset(&new_value_givenName, 0, sizeof(AttributeValue_t));
                        OCTET_STRING_fromString(&new_value_givenName, "");
                        
                        ASN_SET_ADD(&new_attr_givenName->vals.list, &new_value_givenName);
                        
                        ASN_SEQUENCE_ADD(&search_entry_response->protocolOp.choice.searchResEntry.attributes.list, new_attr_givenName);
   
                        struct PartialAttributeList__Member* new_attr_displayName = (struct PartialAttributeList__Member*)calloc(1, sizeof(struct PartialAttributeList__Member));
                        
                        OCTET_STRING_fromString(&new_attr_displayName->type, "displayName");
                        
                        AttributeValue_t new_value_displayName;
                        memset(&new_value_displayName, 0, sizeof(AttributeValue_t));
                        OCTET_STRING_fromString(&new_value_displayName, listtbl[x].alias);
                        
                        ASN_SET_ADD(&new_attr_displayName->vals.list, &new_value_displayName);
                        
                        ASN_SEQUENCE_ADD(&search_entry_response->protocolOp.choice.searchResEntry.attributes.list, new_attr_displayName);

                        struct PartialAttributeList__Member* new_attr_mail = (struct PartialAttributeList__Member*)calloc(1, sizeof(struct PartialAttributeList__Member));

                        OCTET_STRING_fromString(&new_attr_mail->type, "mail");
                        
                        string mail_addr = listtbl[x].username;
                        mail_addr += "@";
                        mail_addr += CMailBase::m_email_domain;
                        AttributeValue_t new_value_mail;
                        memset(&new_value_mail, 0, sizeof(AttributeValue_t));
                        OCTET_STRING_fromString(&new_value_mail, mail_addr.c_str());
                        
                        ASN_SET_ADD(&new_attr_mail->vals.list, &new_value_mail);
                        
                        ASN_SEQUENCE_ADD(&search_entry_response->protocolOp.choice.searchResEntry.attributes.list, new_attr_mail);
                        
                        struct PartialAttributeList__Member* new_attr_uid = (struct PartialAttributeList__Member*)calloc(1, sizeof(struct PartialAttributeList__Member));

                        OCTET_STRING_fromString(&new_attr_uid->type, "uid");
                        
                        string userid = listtbl[x].username;
                        AttributeValue_t new_value_uid;
                        memset(&new_value_uid, 0, sizeof(AttributeValue_t));
                        OCTET_STRING_fromString(&new_value_uid, userid.c_str());
                        
                        ASN_SET_ADD(&new_attr_uid->vals.list, &new_value_uid);
                        
                        ASN_SEQUENCE_ADD(&search_entry_response->protocolOp.choice.searchResEntry.attributes.list, new_attr_uid);
                        
                        
                        //xer_fprint(stderr, &asn_DEF_LDAPMessage, search_entry_response);
                        
                        asn_enc_rval_t erv;
                        erv = der_encode(&asn_DEF_LDAPMessage, search_entry_response, write_out, this);
                        if(erv.encoded < 0)
                        {
                            return FALSE;
                        }
                        //SendDecodedBuffer();
                        
                        free(new_attr_objectClass);
                        free(new_attr_cn);
                        free(new_attr_sn);
                        free(new_attr_givenName);
                        free(new_attr_displayName);
                        free(new_attr_mail);
                        free(new_attr_uid);
                        free(search_entry_response);
                    }
                }
                
                free(baseObject);
                
                LDAPMessage_t* search_done_response = (LDAPMessage_t*)calloc(1, sizeof(LDAPMessage_t));
                
                search_done_response->messageID = ldap_msg->messageID;
                search_done_response->protocolOp.present = ProtocolOp_PR_searchResDone;
                
                asn_long2INTEGER(&search_done_response->protocolOp.choice.searchResDone.resultCode, LDAPResult__resultCode_success);
                
                search_done_response->protocolOp.choice.searchResDone.matchedDN.size = 0;
                search_done_response->protocolOp.choice.searchResDone.matchedDN.buf = NULL;
                
                search_done_response->protocolOp.choice.searchResDone.errorMessage.size = 0;
                search_done_response->protocolOp.choice.searchResDone.errorMessage.buf = NULL;
                
                search_done_response->protocolOp.choice.searchResDone.referral = NULL;
                
                asn_enc_rval_t erv = der_encode(&asn_DEF_LDAPMessage, search_done_response, write_out, this);
                if(erv.encoded < 0)
                {
                    return FALSE;
                }
                
                //SendDecodedBuffer();
                //xer_fprint(stderr, &asn_DEF_LDAPMessage, search_done_response);
                
                free(search_done_response);
                
            }
            
            if(ldap_msg)
            {
                asn_DEF_LDAPMessage.free_struct(&asn_DEF_LDAPMessage, ldap_msg, 0);
                ldap_msg = 0;
            }
             
			free(m_buf);
			m_buf = NULL;
            m_data_len = 0;
            m_buf_len = 0;
		}
	}

    return TRUE;
}