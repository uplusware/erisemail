/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#include "asn_1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int get_ber_val_len(const unsigned char* ber_len, unsigned int& len_bytes) {
  if ((*ber_len & 0x80) == 0x80) {
    unsigned long long val_len = 0;
    len_bytes = *ber_len & (~0x80);
    for (int x = 0; x < len_bytes; x++) {
      val_len <<= 8;
      val_len += ber_len[x + 1];
    }
    len_bytes += 1;
    return val_len;
  } else {
    len_bytes = 1;
    return *ber_len;
  }
}

asn1_tlv::asn1_tlv() {
  m_len = 0;
  m_val = 0;
  m_tag.payload = 0;
}

asn1_tlv::asn1_tlv(const unsigned char* buf,
                   unsigned int len,
                   vector<asn1_tlv*>& brothers) {
  unsigned int parsed_len = 0;

  m_buf = buf;

  m_len = 0;
  m_val = 0;

  m_tag.payload = m_buf[0];  // take 1 byte

  parsed_len += 1;

  unsigned int len_bytes = 0;

  if ((m_buf[1] & 0x80) == 0x80) {
    len_bytes = m_buf[1] & (~0x80);
    for (int x = 0; x < len_bytes; x++) {
      m_len <<= 8;
      m_len += m_buf[x + 2];
    }
  } else {
    m_len = m_buf[1];
  }

  printf("TAG: %d LEN: %d, type : %d\n", m_tag.tag.val, m_len, m_tag.tag.type);

  // take (len_bytes + 1) bytes
  parsed_len += (len_bytes + 1);

  m_val = m_buf + parsed_len;

  // take m_len bytes;
  parsed_len += m_len;

  if (m_tag.tag.encode == ASN_1_ENCODE_CONSTRUCTED) {
    asn1_tlv* asn1_one = new asn1_tlv(m_val, m_len, m_children);
    m_children.push_back(asn1_one);
  } else {
    if (m_tag.tag.val == ASN_1_OCTET_STRING) {
      char* buf = (char*)malloc(m_len + 1);
      memcpy(buf, m_val, m_len);
      buf[m_len] = '\0';

      free(buf);
    }
  }

  if (parsed_len < len) {
    asn1_tlv* asn1_one =
        new asn1_tlv(m_buf + parsed_len, len - parsed_len, brothers);
    brothers.push_back(asn1_one);
  }
}

asn1_tlv::~asn1_tlv() {
  for (int x = 0; x < m_children.size(); x++) {
    delete m_children[x];
  }
}
