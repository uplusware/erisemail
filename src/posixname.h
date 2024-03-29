/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef __POSIXNAME__
#define __POSIXNAME__

#define ERISEMAIL_POSIX_PREFIX "/.ERISEMAIL_"

#define ERISEMAIL_POSIX_QUEUE_SUFFIX ".que"
#define ERISEMAIL_POSIX_SEMAPHORE_SUFFIX ".sem"

#define ERISEMAIL_POST_NOTIFY \
  ERISEMAIL_POSIX_PREFIX "POST_NOTIFY" ERISEMAIL_POSIX_SEMAPHORE_SUFFIX
#define ERISEMAIL_GLOBAL_REJECT_LIST \
  ERISEMAIL_POSIX_PREFIX "GLOBAL_REJECT_LIST" ERISEMAIL_POSIX_SEMAPHORE_SUFFIX
#define ERISEMAIL_GLOBAL_PERMIT_LIST \
  ERISEMAIL_POSIX_PREFIX "GLOBAL_PERMIT_LIST" ERISEMAIL_POSIX_SEMAPHORE_SUFFIX
#define ERISEMAIL_DOMAIN_PERMIT_LIST \
  ERISEMAIL_POSIX_PREFIX "DOMAIN_PERMIT_LIST" ERISEMAIL_POSIX_SEMAPHORE_SUFFIX
#define ERISEMAIL_WEBADMIN_PERMIT_LIST      \
  ERISEMAIL_POSIX_PREFIX "WEBADMIN_PERMIT_" \
                         "LIST" ERISEMAIL_POSIX_SEMAPHORE_SUFFIX

#define ERISEMAIL_SERVICE_LOGNAME "/var/log/erisemail/service.log"
#define ERISEMAIL_SERVICE_LCKNAME \
  ERISEMAIL_POSIX_PREFIX "SERVICE_LOG" ERISEMAIL_POSIX_SEMAPHORE_SUFFIX

#define ERISEMAIL_SSLERR_LOGNAME "/var/log/erisemail/sslerr.log"
#define ERISEMAIL_SSLERR_LCKNAME \
  ERISEMAIL_POSIX_PREFIX "SSLERR_LOG" ERISEMAIL_POSIX_SEMAPHORE_SUFFIX

#define ERISEMAIL_MYSQLERR_LOGNAME "/var/log/erisemail/mysqlerr.log"
#define ERISEMAIL_MYSQLERR_LCKNAME \
  ERISEMAIL_POSIX_PREFIX "MYSQLERR_LOG" ERISEMAIL_POSIX_SEMAPHORE_SUFFIX

#endif /* __POSIXNAME__ */