/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _LDAP_MESSAGE_H_
#define _LDAP_MESSAGE_H_

#include "asn_1.h"

#define LDAP_PROTO_OPS_BIND_REQUEST       0x00
#define LDAP_PROTO_OPS_BIND_RESPONSE      0x01
#define LDAP_PROTO_OPS_SEARCH_REQUEST     0x03
#define LDAP_PROTO_OPS_SEARCH_ENTRY       0x04

class LDAPMessage
{
public:
    LDAPMessage()
    {
        
    }
    
    LDAPMessage(unsigned int msg_id)
    {
        m_messageID = msg_id;
    }
    virtual ~LDAPMessage()
    {
        
    }

    virtual int parse(const unsigned char* msg_buf, unsigned int msg_len);
    
private:
    unsigned int m_messageID;
    unsigned int m_protocolOp;
    
    const unsigned char* m_messageBuf;
    unsigned long long m_messageLen;
};

class LDAPbindRequest : public LDAPMessage
{
public:
    LDAPbindRequest() {};
    
    LDAPbindRequest(unsigned int msg_id) : LDAPMessage(msg_id)
    {
        
    }
     
    virtual ~LDAPbindRequest() {};
    
    virtual int parse(const unsigned char* msg_buf, unsigned int msg_len);
private:
    unsigned int m_version;
};

class LDAPbindResponse : public LDAPMessage
{
public:
    LDAPbindResponse() {};
    virtual ~LDAPbindResponse() {};
private:
    
};

class LDAPsearchRequest : public LDAPMessage
{
public:
    LDAPsearchRequest() {};
    LDAPsearchRequest(unsigned int msg_id) : LDAPMessage(msg_id)
    {
        
    }
    virtual ~LDAPsearchRequest() {};
    
    virtual int parse(const unsigned char* msg_buf, unsigned int msg_len);
private:

};

class LDAPsearchResEntry : public LDAPMessage
{
public:
    LDAPsearchResEntry() {};
    virtual ~LDAPsearchResEntry() {};
private:

};

class LDAPunbindRequest : public LDAPMessage
{
public:
    LDAPunbindRequest() {};
    virtual ~LDAPunbindRequest() {};
private:

};

#endif /* _LDAP_MESSAGE_H_ */

