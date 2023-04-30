/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#ifndef _CACHE_H_
#define _CACHE_H_

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <list>
#include <map>
#include <string>

using namespace std;

typedef struct {
  char* pbuf;
  unsigned long flen;
  time_t last_modified; /* Last-Modified */
} filedata;

class memory_cache {
 public:
  memory_cache();
  virtual ~memory_cache();

  map<string, filedata> m_htdoc;
  map<string, string> m_type_table;

  void load(const char* dirpath);
  void unload();
  string m_dirpath;
};

#endif /* _CACHE_H_ */
