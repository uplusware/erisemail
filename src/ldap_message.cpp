/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ldap_message.h"

int LDAPMessage::parse(const unsigned char* msg_buf, unsigned int msg_len)
{
    asn1_ber_tag message_id_ber_tag;
    message_id_ber_tag.payload = msg_buf[0];
    
   //printf("encode: %d\n", message_id_ber_tag.tag.encode);
    if(message_id_ber_tag.tag.type != ASN_1_TYPE_UNIVERSAL || message_id_ber_tag.tag.val != ASN_1_INTEGER)
    {
        return -1;
    }
    
    unsigned int len_bytes;
    int data_len = get_ber_val_len(&msg_buf[1], len_bytes);
    
    m_messageID = 0;
    
    for(int x = 0; x < data_len; x++)
    {
        m_messageID <<= 8;
        m_messageID += msg_buf[1 + len_bytes + x];
    }
    
   //printf("%d %d %d\n", data_len, len_bytes, m_messageID);
    
    asn1_ber_tag protocol_op_ber_tag;
    protocol_op_ber_tag.payload = msg_buf[1 + len_bytes + data_len];
   //printf("payload: %x, encode: %d\n", protocol_op_ber_tag.payload, protocol_op_ber_tag.tag.encode);
    
    if(protocol_op_ber_tag.tag.type != ASN_1_TYPE_APPLICATION || protocol_op_ber_tag.tag.encode != ASN_1_ENCODE_CONSTRUCTED)
    {
        return -1;
    }
    
    m_protocolOp = protocol_op_ber_tag.tag.val;
    
    unsigned int len_bytes2;
    int data_len2 = get_ber_val_len(&msg_buf[1 + len_bytes + data_len + 1], len_bytes2);
    
   //printf("%x %x, m_protocolOp: %d\n", data_len2, len_bytes2, m_protocolOp);
     
     
    if(m_protocolOp == LDAP_PROTO_OPS_BIND_REQUEST)
    {
        LDAPbindRequest * bind_req = new LDAPbindRequest(m_messageID);
        
        bind_req->parse(&msg_buf[1 + len_bytes + data_len + 1 + len_bytes2], data_len2);
        
        delete bind_req;
    }
    else if(m_protocolOp == LDAP_PROTO_OPS_SEARCH_REQUEST)
    {
        LDAPsearchRequest * search_req = new LDAPsearchRequest(m_messageID);
        
        delete search_req;
    }
    else
    {
        
    }
    return 0;
}

int LDAPbindRequest::parse(const unsigned char* msg_buf, unsigned int msg_len)
{
    asn1_ber_tag version_ber_tag;
    version_ber_tag.payload = msg_buf[0];
    
    if(version_ber_tag.tag.type != ASN_1_TYPE_UNIVERSAL || version_ber_tag.tag.val != ASN_1_INTEGER)
    {
        return -1;
    }
    
    unsigned int len_bytes;
    int data_len = get_ber_val_len(&msg_buf[1], len_bytes);
    
    m_version = 0;
    
    for(int x = 0; x < data_len; x++)
    {
        m_version <<= 8;
        m_version += msg_buf[1 + len_bytes + x];
    }
    
   //printf("LDAPbindRequest: %d %d %d\n", data_len, len_bytes, m_version);
    
    asn1_ber_tag name_ber_tag;
    name_ber_tag.payload = msg_buf[1 + len_bytes + data_len];
   //printf("payload: %x, encode: %d\n", name_ber_tag.payload, name_ber_tag.tag.encode);
    
    if(name_ber_tag.tag.type != ASN_1_TYPE_UNIVERSAL || name_ber_tag.tag.val != ASN_1_OCTET_STRING)
    {
        return -1;
    }
    
    unsigned int len_bytes2;
    int data_len2 = get_ber_val_len(&msg_buf[1 + len_bytes + data_len + 1], len_bytes2);
    
    char* common_name = (char*)malloc(data_len2 + 1);
    
    memcpy(common_name, &msg_buf[1 + len_bytes + data_len + 1 + len_bytes2], data_len2);
    
    common_name[data_len2] = '\0';
    
   //printf("%s\n", common_name);
    
    
    asn1_ber_tag password_ber_tag;
    password_ber_tag.payload = msg_buf[1 + len_bytes + data_len+ 1 + len_bytes2 + data_len2];
   //printf("payload: %x, encode: %d\n", password_ber_tag.payload, password_ber_tag.tag.encode);
    
    if(password_ber_tag.tag.type != ASN_1_TYPE_CONTEXT_SPECIFIC)
    {
        return -1;
    }
    
    unsigned int len_bytes3;
    int data_len3 = get_ber_val_len(&msg_buf[1 + len_bytes + data_len + 1 + len_bytes2 + data_len2 + 1], len_bytes3);
    
    char* password = (char*)malloc(data_len3 + 1);
    
    memcpy(password, &msg_buf[1 + len_bytes + data_len + 1 + len_bytes2  + data_len2 + 1 + len_bytes3], data_len3);
    
    password[data_len3] = '\0';
    
   //printf("%s\n", password);
    
    free(common_name);
    free(password);
    
    return 0;
}

int LDAPsearchRequest::parse(const unsigned char* msg_buf, unsigned int msg_len)
{
    return 0;
}