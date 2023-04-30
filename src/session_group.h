/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#ifndef _SESSION_GROUP_H_
#define _SESSION_GROUP_H_

#include <sys/epoll.h>
#include "xmpp.h"

#include <map>
#include <stack>
#include <string>
using namespace std;

#define MAX_EVENTS_NUM 65536 * 2
#define MAX_SOCKFD_NUM 65536 * 2

class Session_Group {
 public:
  Session_Group();
  virtual ~Session_Group();
  BOOL Accept(int sockfd,
              SSL* ssl,
              SSL_CTX* ssl_ctx,
              const char* clientip,
              Service_Type st,
              BOOL is_ssl,
              StorageEngine* storage_engine,
              memory_cache* ch,
              memcached_st* memcached);
  BOOL Connect(const char* hostname,
               unsigned short port,
               Service_Type st,
               StorageEngine* storage_engine,
               memory_cache* ch,
               memcached_st* memcached,
               std::stack<string>& data_stack,
               BOOL isXmppDialBack);
  int Poll();

 private:
  map<int, Session_Info*> m_session_list;
  int m_epoll_fd;
  struct epoll_event* m_events;
};

class Session_Info {
 private:
  Session_Group* m_sess_grp;
  Service_Type m_st;
  int m_epoll_fd;
  int m_sockfd;
  SSL* m_ssl;
  SSL_CTX* m_ssl_ctx;
  string m_clientip;
  StorageEngine* m_storage_engine;
  memcached_st* m_memcached;
  BOOL m_isSSL;
  BOOL m_s2s;
  string m_peer_domainname;
  CMailBase* m_protocol;
  string m_recvbuf;
  stack<string> m_message_stack;
  BOOL m_isXmppDialBack;

 public:
  Session_Info(Session_Group* sess_grp,
               Service_Type st,
               int epoll_fd,
               int sockfd,
               SSL* ssl,
               SSL_CTX* ssl_ctx,
               const char* clientip,
               StorageEngine* storage_engine,
               memcached_st* memcached,
               BOOL isSSL,
               BOOL s2s = FALSE,
               const char* pdn = "",
               BOOL isXmppDialBack = FALSE) {
    m_sess_grp = sess_grp;
    m_st = st;
    m_epoll_fd = epoll_fd;
    m_sockfd = sockfd;
    m_ssl = ssl;
    m_ssl_ctx = ssl_ctx;
    m_clientip = clientip;
    m_storage_engine = storage_engine;
    m_isSSL = isSSL;
    m_protocol = NULL;
    m_s2s = s2s;
    m_peer_domainname = pdn;
    m_isXmppDialBack = isXmppDialBack;
  }

  virtual ~Session_Info() {
    if (m_protocol)
      delete m_protocol;
  }

  CMailBase* CreateProtocol() {
    if (m_st == stXMPP)
      m_protocol =
          new CXmpp(this, m_epoll_fd, m_sockfd, m_ssl, m_ssl_ctx,
                    m_clientip.c_str(), m_storage_engine, m_memcached, m_isSSL,
                    m_s2s, m_peer_domainname.c_str(), m_isXmppDialBack);
    else
      m_protocol = NULL;

    return m_protocol;
  }

  CMailBase* GetProtocol() { return m_protocol; }

  Service_Type GetServiceType() { return m_st; }

  string& RecvBuf() { return m_recvbuf; }

  BOOL Connect(const char* hostname,
               unsigned short port,
               Service_Type st,
               StorageEngine* storage_engine,
               memory_cache* ch,
               memcached_st* memcached,
               std::stack<string>& data_stack,
               BOOL isXmppDialBack) {
    return m_sess_grp->Connect(hostname, port, st, storage_engine, ch,
                               memcached, data_stack, isXmppDialBack);
  }

  void PushData(const char* data) { m_message_stack.push(data); }

  void SetStack(std::stack<string>& data_stack) {
    m_message_stack = data_stack;
  }

  bool PopData(string& data) {
    if (!m_message_stack.empty()) {
      data = m_message_stack.top();
      m_message_stack.pop();
      return true;
    } else
      return false;
  }
};

#endif /* _SESSION_GROUP_H_ */