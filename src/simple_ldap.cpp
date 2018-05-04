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
#include "service.h"
#include "LDAPMessage.h"

int get_ber_val_len(const unsigned char * ber_len)
{
    if(*ber_len & 0x80 == 0x80)
    {
        unsigned long long val_len = 0;
        int ber_len_bytes = *ber_len & (~0x80);
        for(int x = 0; x < ber_len_bytes; x++)
        {
            val_len <<= 8;
            val_len += ber_len[x + 1];
        }
        
        return val_len;
    }
    else
    {
        return *ber_len;
    }
}

/* Dump the buffer out to the specified FILE */
static int write_out(const void *buffer, size_t size, void *key)
{
    if(size > 0)
    {
        CLdap* ldap_session = (CLdap*)key;
        return ldap_session->LdapSend((const char*)buffer, size) < 0 ? -1 : 0;
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
}

int CLdap::LdapSend(const char* buf, int len)
{
	/* printf("%s", buf); */
    if(m_ssl)
        return SSLWrite(m_sockfd, m_ssl, buf, len, CMailBase::m_connection_idle_timeout);
    else
        return Send(m_sockfd, buf, len, CMailBase::m_connection_idle_timeout);	
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
		if((m_buf[0] & 0x80) == 0x80)
		{
			asn1_len_buf_len = (m_buf[0] & (~0x80)) + 1;
		}
		
		if(m_data_len >= asn1_len_buf_len + 1)
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
        
		if(m_data_len >= asn1_len + asn1_len_buf_len + 1 /*tag len*/)
		{
            /* printf("%d\n", m_data_len);
            
           
            for(int c = 0; c < m_data_len; c++)
            {
                if(c % 15 == 0)
                    printf("\n");
                printf("%02x", m_buf[c]);
            }
            printf("\n"); */
            
            LDAPMessage_t * ldap_msg = NULL;
            asn_dec_rval_t rval;
            rval = ber_decode(0, &asn_DEF_LDAPMessage, (void**)&ldap_msg, m_buf, m_data_len);

            if(rval.code != RC_OK || !ldap_msg)
            {
                return FALSE;
            }
            
            //printf("%d %d\n", ldap_msg->messageID, ldap_msg->protocolOp.present);
            if(ProtocolOp_PR_bindRequest == ldap_msg->protocolOp.present)
            {
                if(ldap_msg->protocolOp.choice.bindRequest.authentication.present != AuthenticationChoice_PR_simple)
                {
                    return FALSE;
                }
                char* name = (char*)malloc(ldap_msg->protocolOp.choice.bindRequest.name.size + 1);
                memcpy(name, ldap_msg->protocolOp.choice.bindRequest.name.buf, ldap_msg->protocolOp.choice.bindRequest.name.size);
                name[ldap_msg->protocolOp.choice.bindRequest.name.size] = '\0';
                //printf("%s\n", name);
                
                char* password = (char*)malloc(ldap_msg->protocolOp.choice.bindRequest.authentication.choice.simple.size + 1);
                memcpy(password, ldap_msg->protocolOp.choice.bindRequest.authentication.choice.simple.buf, ldap_msg->protocolOp.choice.bindRequest.authentication.choice.simple.size);
                password[ldap_msg->protocolOp.choice.bindRequest.authentication.choice.simple.size] = '\0';
                //printf("%s\n", password);
                
                string auth_name = name;
                string auth_pwd = password;
                free(password);
                free(name);
                
                
                strcut(auth_name.c_str(), "cn=", ",",auth_name);
                strtrim(auth_name);
                MailStorage* mailStg;
                StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
                
                //printf("[%s] [%s]\n", auth_name.c_str(), auth_pwd.c_str());
                
                if(mailStg && mailStg->CheckLogin(auth_name.c_str(), auth_pwd.c_str()) == 0)
                {
                    LDAPMessage_t* bind_response = (LDAPMessage_t*)calloc(1, sizeof(LDAPMessage_t));
                    
                    bind_response->messageID = ldap_msg->messageID;
                    bind_response->protocolOp.present = ProtocolOp_PR_bindResponse;
                    
                    asn_long2INTEGER(&bind_response->protocolOp.choice.bindResponse.resultCode, BindResponse__resultCode_success);
                    bind_response->protocolOp.choice.bindResponse.matchedDN.size = 0;
                    bind_response->protocolOp.choice.bindResponse.matchedDN.buf = NULL;
                    bind_response->protocolOp.choice.bindResponse.errorMessage.size = 0;
                    bind_response->protocolOp.choice.bindResponse.errorMessage.buf = NULL;
                    
                    bind_response->protocolOp.choice.bindResponse.referral = NULL;
                    bind_response->protocolOp.choice.bindResponse.serverSaslCreds = NULL;
                    
                    bind_response->controls = NULL;
                    
                    asn_enc_rval_t erv;
                    erv = der_encode(&asn_DEF_LDAPMessage, bind_response, write_out, this);
                    if(erv.encoded < 0)
                    {
                        return FALSE;
                    }
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
                char* baseObject = (char*)malloc(ldap_msg->protocolOp.choice.searchRequest.baseObject.size + 1);
                memcpy(baseObject, ldap_msg->protocolOp.choice.searchRequest.baseObject.buf, ldap_msg->protocolOp.choice.searchRequest.baseObject.size);
                baseObject[ldap_msg->protocolOp.choice.searchRequest.baseObject.size] = '\0';
                //printf("baseObject: %s\n", baseObject);
                MailStorage* mailStg;
                StorageEngineInstance stgengine_instance(m_storageEngine, &mailStg);
                vector <User_Info> listtbl;
                if(mailStg && mailStg->ListID(listtbl, "uname", TRUE) == 0)
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
                        
                        struct PartialAttributeList__Member* new_attr1 = (struct PartialAttributeList__Member*)calloc(1, sizeof(struct PartialAttributeList__Member));
                        
                        
                        OCTET_STRING_fromString(&new_attr1->type, "alias");
                        
                        AttributeValue_t new_value1;
                        memset(&new_value1, 0, sizeof(AttributeValue_t));
                        OCTET_STRING_fromString(&new_value1, listtbl[x].alias);
                        
                        ASN_SET_ADD(&new_attr1->vals.list, &new_value1);
                        
                        ASN_SEQUENCE_ADD(&search_entry_response->protocolOp.choice.searchResEntry.attributes.list, new_attr1);
                        
                        
                        struct PartialAttributeList__Member* new_attr2 = (struct PartialAttributeList__Member*)calloc(1, sizeof(struct PartialAttributeList__Member));
                        
                        
                        OCTET_STRING_fromString(&new_attr2->type, "mail");
                        
                        string mail_addr = listtbl[x].username;
                        mail_addr += "@";
                        mail_addr += CMailBase::m_email_domain;
                        AttributeValue_t new_value2;
                        memset(&new_value2, 0, sizeof(AttributeValue_t));
                        OCTET_STRING_fromString(&new_value2, mail_addr.c_str());
                        
                        ASN_SET_ADD(&new_attr2->vals.list, &new_value2);
                        
                        ASN_SEQUENCE_ADD(&search_entry_response->protocolOp.choice.searchResEntry.attributes.list, new_attr2);
                        
                        //xer_fprint(stderr, &asn_DEF_LDAPMessage, search_entry_response);
                        
                        
                        
                        asn_enc_rval_t erv;
                        erv = der_encode(&asn_DEF_LDAPMessage, search_entry_response, write_out, this);
                        if(erv.encoded < 0)
                        {
                            return FALSE;
                        }
                        free(new_attr1);
                        free(new_attr2);
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