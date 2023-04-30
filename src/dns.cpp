/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#include "dns.h"

unsigned int udpdns::gethostname(const unsigned char* dnsbuf,
                                 unsigned int seek,
                                 string& hostname) {
  unsigned int ret = 0;
  unsigned int i = 0;
  while (1) {
    if (dnsbuf[seek + i] == 0) {
      i++;
      break;
    } else if ((dnsbuf[seek + i] & 0xc0) == 0xc0) {
      unsigned short cptr = dnsbuf[seek + i] & (~0xc0);
      cptr <<= 8;
      i++;
      cptr |= dnsbuf[seek + i];
      gethostname(dnsbuf, cptr, hostname);
      i++;
      break;
    } else {
      char sztmp[64];
      memset(sztmp, 0, 64);
      memcpy(sztmp, &dnsbuf[seek + i + 1], dnsbuf[seek + i]);
      i += dnsbuf[seek + i] + 1;
      if (hostname == "") {
        hostname += sztmp;
      } else {
        hostname += ".";
        hostname += sztmp;
      }
    }
  }
  return i;
}

int udpdns::query(const char* hostname,
                  unsigned short record_type,
                  vector<DNSQry_List>& result_list) {
  // make the dns package
  Dns_Header DnsHdr;
  memset(&DnsHdr, 0, sizeof(DnsHdr));
  DnsHdr.Tag = htons(time(NULL) % 0xFFFF);
  DnsHdr.nQuestion = htons(1);
  DnsHdr.Flag = htons(0x0100);
  Question_Tailer qTailer;
  qTailer.QueryBase = htons(0x0001);
  qTailer.QueryType = htons(record_type);

  unsigned char dnsbufptr[1024];
  unsigned int dnsbuflength = 0;
  memcpy(dnsbufptr, &DnsHdr, sizeof(Dns_Header));

  vector<string> vecDest;
  string strhostname = hostname;
  vSplitString(strhostname, vecDest, ".", TRUE, 0x7FFFFFFFU);
  int vLen = vecDest.size();
  int qseek = 0;
  for (int i = 0; i < vLen; i++) {
    dnsbufptr[sizeof(Dns_Header) + qseek] = vecDest[i].length();
    qseek += 1;
    memcpy(&dnsbufptr[sizeof(Dns_Header) + qseek], vecDest[i].c_str(),
           vecDest[i].length());
    qseek += vecDest[i].length();
  }

  dnsbufptr[sizeof(Dns_Header) + qseek] = 0;
  qseek += 1;
  memcpy(&dnsbufptr[sizeof(Dns_Header) + qseek], &qTailer,
         sizeof(Question_Tailer));
  dnsbuflength = sizeof(Dns_Header) + qseek + sizeof(Question_Tailer);

  // connect dns server;
  int dns_sockfd;
  struct sockaddr client_sockaddr;

  struct addrinfo hints;
  struct addrinfo *server_addr, *rp;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
  hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
  hints.ai_protocol = 0;          /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  int s = getaddrinfo(m_server != "" ? m_server.c_str() : NULL, "53", &hints,
                      &server_addr);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    return FALSE;
  }

  for (rp = server_addr; rp != NULL; rp = rp->ai_next) {
    dns_sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (dns_sockfd == -1)
      continue;

    int flags = fcntl(dns_sockfd, F_GETFL, 0);
    fcntl(dns_sockfd, F_SETFL, flags | O_NONBLOCK);

    int res;
    fd_set mask;
    struct timeval dns_timeout;
    dns_timeout.tv_sec = 5;
    dns_timeout.tv_usec = 0;

    FD_ZERO(&mask);
    FD_SET(dns_sockfd, &mask);

    res = select(dns_sockfd + 1, NULL, &mask, NULL, &dns_timeout);

    if (res != 1) {
      close(dns_sockfd);
      continue;
    }

    int n = sendto(dns_sockfd, (char*)dnsbufptr, dnsbuflength, 0,
                   (struct sockaddr*)rp->ai_addr, rp->ai_addrlen);

    if (n != dnsbuflength || n <= 0) {
      close(dns_sockfd);
      continue;
    }

    // Recv Ack
    memset(dnsbufptr, 0, 1024);
    int nLen = sizeof(struct sockaddr);
    FD_ZERO(&mask);
    FD_SET(dns_sockfd, &mask);

    res = select(dns_sockfd + 1, &mask, NULL, NULL, &dns_timeout);
    if (res != 1) /* error or timeout*/
    {
      close(dns_sockfd);
      continue;
    }
    n = recvfrom(dns_sockfd, (char*)dnsbufptr, 1024, 0,
                 (struct sockaddr*)&client_sockaddr, (socklen_t*)&nLen);
    /*
            for(int f = 0; f < 32; f++)
            {
                printf("  %d", f%10);
            }
            printf("\n");
            for(int f = 0; f < 32; f++)
            {
                printf("___", f%10);
            }
            printf("\n");
            for(int f = 0; f < n; f++)
            {
                if(dnsbufptr[f] >= 32 && dnsbufptr[f] < 127)
                    printf("  %c", dnsbufptr[f]);
                else
                    printf(" %02X", dnsbufptr[f]);
                if(f%32 == 31)
                    printf("\n");
            }
            printf("\n");
    */
    if (n < 12) {
      continue;
    }
    // succed to be here, them jump out the loop
    break;
  }

  if (rp == NULL) { /* No address succeeded */
    fprintf(stderr, "Could not connect to DNS server(%s) when query \"%s\"\n",
            m_server.c_str(), hostname);
    freeaddrinfo(server_addr); /* No longer needed */
    return -1;
  }

  freeaddrinfo(server_addr); /* No longer needed */

  Dns_Header* pAckHeader = (Dns_Header*)dnsbufptr;
  if (pAckHeader->Tag != DnsHdr.Tag)
    return -1;
  /*
      printf("\n\n");
      for(int f = 0; f < 32; f++)
      {
          printf("  %d", f%10);
      }
      printf("\n");
      for(int f = 0; f < 32; f++)
      {
          printf("___", f%10);
      }
      printf("\n");
      for(int f = 0; f < sizeof(Dns_Header); f++)
      {
          if(dnsbufptr[f] >= 32 && dnsbufptr[f] < 127)
              printf("  %c", dnsbufptr[f]);
          else
              printf(" %02X", dnsbufptr[f]);
          if(f%32 == 31)
              printf("\n");
      }
      printf("\n");
  */
  // Jump over the Quesion field
  int c = ntohs(pAckHeader->nQuestion);

  int aseek = sizeof(Dns_Header);
  while (c) {
    string hostname;
    unsigned int hostnamelen = gethostname(dnsbufptr, aseek, hostname);
    c--;
    aseek += hostnamelen;
  }
  /*
      printf("\n\n");
      for(int f = 0; f < 32; f++)
      {
          printf("  %d", f%10);
      }
      printf("\n");
      for(int f = 0; f < 32; f++)
      {
          printf("___", f%10);
      }
      printf("\n");
      for(int f = sizeof(Dns_Header); f < aseek; f++)
      {
          if(dnsbufptr[f] >= 32 && dnsbufptr[f] < 127)
              printf("  %c", dnsbufptr[f]);
          else
              printf(" %02X", dnsbufptr[f]);
          if(f%32 == 31)
              printf("\n");
      }
      printf("\n");
  */

  int j;

  aseek += sizeof(Question_Tailer);

  for (j = 0; j < ntohs(pAckHeader->nResource); j++) {
    c = 1;
    while (c) {
      string hostname;
      unsigned int hostnamelen = gethostname(dnsbufptr, aseek, hostname);
      c--;
      aseek += hostnamelen;
    }

    unsigned short reply_type = dnsbufptr[aseek];
    aseek++;
    reply_type <<= 8;
    reply_type |= dnsbufptr[aseek];
    aseek++;

    unsigned short reply_class = dnsbufptr[aseek];
    aseek++;
    reply_class <<= 8;
    reply_class |= dnsbufptr[aseek];
    aseek++;

    unsigned int replay_ttl = dnsbufptr[aseek];
    aseek++;
    replay_ttl <<= 8;

    replay_ttl |= dnsbufptr[aseek];
    aseek++;
    replay_ttl <<= 8;

    replay_ttl |= dnsbufptr[aseek];
    aseek++;
    replay_ttl <<= 8;

    replay_ttl |= dnsbufptr[aseek];
    aseek++;

    unsigned short reply_resource_len = dnsbufptr[aseek];
    aseek++;
    reply_resource_len <<= 8;
    reply_resource_len |= dnsbufptr[aseek];
    aseek++;

    if (reply_type == DNS_RCD_TYPE_MX) {
      unsigned short reply_resource_preference = dnsbufptr[aseek];
      aseek++;
      reply_resource_preference <<= 8;
      reply_resource_preference |= dnsbufptr[aseek];
      aseek++;

      string hostname;
      unsigned int hostnamelen = gethostname(dnsbufptr, aseek, hostname);

      DNSQry_List list;
      strcpy(list.address, hostname.c_str());
      list.priority = reply_resource_preference;
      list.port = 0;
      list.ttl = replay_ttl;
      result_list.push_back(list);
      aseek += hostnamelen;

    } else if (reply_type == DNS_RCD_TYPE_SRV) {
      unsigned short replay_priority = dnsbufptr[aseek];
      aseek++;
      replay_priority <<= 8;
      replay_priority |= dnsbufptr[aseek];
      aseek++;

      unsigned short replay_weight = dnsbufptr[aseek];
      aseek++;
      replay_weight <<= 8;
      replay_weight |= dnsbufptr[aseek];
      aseek++;

      unsigned short replay_port = dnsbufptr[aseek];
      aseek++;
      replay_port <<= 8;
      replay_port |= dnsbufptr[aseek];
      aseek++;

      string hostname;
      unsigned int hostnamelen = gethostname(dnsbufptr, aseek, hostname);

      DNSQry_List list;
      strcpy(list.address, hostname.c_str());
      list.port = replay_port;
      list.ttl = replay_ttl;
      list.priority = replay_priority;
      result_list.push_back(list);

      aseek += hostnamelen;
    } else {
      aseek += reply_resource_len;
    }
  }
#if _MORE_DNS_RESOURCE_
  for (j = 0; j < ntohs(pAckHeader->nAuthoritativeResource); j++) {
    c = 1;
    while (c) {
      string hostname;
      unsigned int hostnamelen = gethostname(dnsbufptr, aseek, hostname);
      c--;
      aseek += hostnamelen;
    }

    unsigned short reply_type = dnsbufptr[aseek];
    aseek++;
    reply_type <<= 8;
    reply_type |= dnsbufptr[aseek];
    aseek++;

    unsigned short reply_class = dnsbufptr[aseek];
    aseek++;
    reply_class <<= 8;
    reply_class |= dnsbufptr[aseek];
    aseek++;

    unsigned int replay_ttl = dnsbufptr[aseek];
    aseek++;
    replay_ttl <<= 8;

    replay_ttl |= dnsbufptr[aseek];
    aseek++;
    replay_ttl <<= 8;

    replay_ttl |= dnsbufptr[aseek];
    aseek++;
    replay_ttl <<= 8;

    replay_ttl |= dnsbufptr[aseek];
    aseek++;

    unsigned short reply_resource_len = dnsbufptr[aseek];
    aseek++;
    reply_resource_len <<= 8;
    reply_resource_len |= dnsbufptr[aseek];
    aseek++;

    if (reply_type == DNS_RCD_TYPE_NS) {
      string hostname;
      unsigned int hostnamelen = gethostname(dnsbufptr, aseek, hostname);
    }
    aseek += reply_resource_len;
  }

  for (j = 0; j < ntohs(pAckHeader->nAdditionalRecord); j++) {
    c = 1;
    while (c) {
      string hostname;
      unsigned int hostnamelen = gethostname(dnsbufptr, aseek, hostname);
      c--;
      aseek += hostnamelen;
    }

    unsigned short reply_type = dnsbufptr[aseek];
    aseek++;
    reply_type <<= 8;
    reply_type |= dnsbufptr[aseek];
    aseek++;

    unsigned short reply_class = dnsbufptr[aseek];
    aseek++;
    reply_class <<= 8;
    reply_class |= dnsbufptr[aseek];
    aseek++;

    unsigned int replay_ttl = dnsbufptr[aseek];
    aseek++;
    replay_ttl <<= 8;

    replay_ttl |= dnsbufptr[aseek];
    aseek++;
    replay_ttl <<= 8;

    replay_ttl |= dnsbufptr[aseek];
    aseek++;
    replay_ttl <<= 8;

    replay_ttl |= dnsbufptr[aseek];
    aseek++;

    unsigned short reply_resource_len = dnsbufptr[aseek];
    aseek++;
    reply_resource_len <<= 8;
    reply_resource_len |= dnsbufptr[aseek];
    aseek++;

    if (reply_type == DNS_RCD_TYPE_A) {
      // do nothing
    }

    aseek += reply_resource_len;
  }
#endif /* _MORE_DNS_RESOURCE_ */
  if (result_list.size() > 0)
    sort(result_list.begin(), result_list.end(), sort_algorithm);
  return 0;
}