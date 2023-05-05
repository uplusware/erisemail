/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#ifndef _DNS_H_
#define _DNS_H_

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>
#include <algorithm>
#include <string>

using namespace std;

#include "util/general.h"

typedef struct {
  char address[513];
  unsigned short port;
  unsigned int ttl;
  unsigned int priority;
} DNSQry_List;

bool __inline__ sort_algorithm(const DNSQry_List& l1, const DNSQry_List& l2) {
  return (l1.priority > l2.priority);
}

#pragma pack(push, 1)

typedef struct {
  unsigned short Tag;
  unsigned short Flag;
  unsigned short nQuestion;
  unsigned short nResource;
  unsigned short nAuthoritativeResource;
  unsigned short nAdditionalRecord;
} Dns_Header;

typedef struct {
  unsigned short QueryType;
  unsigned short QueryBase;
} Question_Tailer;

typedef struct {
  unsigned short QueryType;
  unsigned short QueryBase;
} Ack_Tailer;

#pragma pack(push, pop)

#define DNS_RCD_TYPE_A 1
#define DNS_RCD_TYPE_AAAA 28
#define DNS_RCD_TYPE_CNAME 5
#define DNS_RCD_TYPE_MX 15
#define DNS_RCD_TYPE_SRV 33
#define DNS_RCD_TYPE_NS 2

class udpdns {
 public:
  udpdns(const char* server) { m_server = server; }
  virtual ~udpdns() {}

  int mxquery(const char* hostname, vector<DNSQry_List>& mxlist) {
    return query(hostname, DNS_RCD_TYPE_MX, mxlist);
  }

  int srvquery(const char* hostname, vector<DNSQry_List>& srvlist) {
    return query(hostname, DNS_RCD_TYPE_SRV, srvlist);
  }

 protected:
  /* member method */
  unsigned int gethostname(const unsigned char* dnsbuf,
                           unsigned int seek,
                           string& hostname);
  int query(const char* hostname,
            unsigned short record_type,
            vector<DNSQry_List>& result_list);

  /* member variable */
  string m_server;
};

#endif /* _DNS_H_ */
