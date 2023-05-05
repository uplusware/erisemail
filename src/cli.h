/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#ifndef _CLI_H_
#define _CLI_H_
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>
#include "base.h"
#include "util/general.h"

typedef struct {
  char name[32];
  mqd_t qid;
} QueueInfo;

class CLI : public CMailBase {
 public:
  CLI();
  virtual ~CLI();

  void Run();
  virtual BOOL Parse(char* text);
  vector<QueueInfo> m_queueList;

 protected:
};

#endif /* _CLI_H_ */
