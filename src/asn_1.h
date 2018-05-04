/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _ASN_1_H_
#define _ASN_1_H_

#include <vector>
using namespace std;

#define ASN_1_BER_RESERVED                      0 
#define ASN_1_BOOLEAN                           1 
#define ASN_1_INTEGER                           2 
#define ASN_1_BIT_STRING                        3 
#define ASN_1_OCTET_STRING                      4 
#define ASN_1_NULL                              5                   
#define ASN_1_OBJECT_IDENTIFIER                 6 
#define ASN_1_OBJECT_DESCRIPION                 7 
#define ASN_1_EXTERNAL                          8 
#define ASN_1_INSTANCE_OF                       8 
#define ASN_1_REAL                              9 
#define ASN_1_ENUMERATED                        10 
#define ASN_1_EMBEDDED_PDV                      11 
#define ASN_1_UFT8_STRING                       12 
#define ASN_1_RELATIVE_OID                      13 
#define ASN_1_RESERVED_14                       14 
#define ASN_1_RESERVED_15                       15 
#define ASN_1_SEQUENCE                          16 
#define ASN_1_SEQUENCE_OF                       16 
#define ASN_1_SET                               17 
#define ASN_1_SET_OF                            17 
#define ASN_1_NUMERIC_STRING                    18 
#define ASN_1_PRINTABLE_STRING                  19 
#define ASN_1_TELETEX_STRING                    20 
#define ASN_1_T61_STRING                        20 
#define ASN_1_VIDEOTEX_STRING                   21 
#define ASN_1_IA5_STRING                        22 
#define ASN_1_UTCTIME                           23 
#define ASN_1_GENERALIZED_TIME                  24 
#define ASN_1_GRAPHIC_STRING                    25 
#define ASN_1_VISIBLE_STRING                    26 
#define ASN_1_ISO646_STRING                     26 
#define ASN_1_GENERAL_STRING                    27 
#define ASN_1_UNIVERSAL_STRING                  28 
#define ASN_1_CHARACTER_STRING                  29 
#define ASN_1_BMP_STRING                        30 

#define ASN_1_TYPE_UNIVERSAL                    0
#define ASN_1_TYPE_APPLICATION                  1
#define ASN_1_TYPE_CONTEXT_SPECIFIC             2
#define ASN_1_TYPE_PRIVATE                      3

#define ASN_1_ENCODE_PRIMITIVE                  0
#define ASN_1_ENCODE_CONSTRUCTED                1

union asn1_ber_tag
{
    struct {
        unsigned char val : 5;
        unsigned char encode : 1;
        unsigned char type : 2;
    } tag;
    unsigned char payload;
};

int get_ber_val_len(const unsigned char * ber_len, unsigned int & len_bytes);

class asn1_tlv
{
public:
    asn1_tlv();
    
    asn1_tlv(const unsigned char* buf, unsigned int len, vector<asn1_tlv* >& brothers);
    
    virtual ~asn1_tlv();
private:
    asn1_ber_tag m_tag;
    
    unsigned int m_len;
    
    const unsigned char* m_val;
    
    const unsigned char* m_buf;
    
    vector<asn1_tlv* > m_children;
};


typedef struct
{
        unsigned char* asn1_buf;
        unsigned int buf_len;
} asn1_element;

#endif /* _ASN_1_H_ */

