/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef _POSTUDF_H_
#define _POSTUDF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <mysql.h>

#ifdef __cplusplus
extern "C" {
#endif /* _cplusplus */
/* These must be right or mysqld will not find the symbol! */
my_bool post_notify_init(UDF_INIT* initid, UDF_ARGS* args, char* message);
void post_notify_deinit(UDF_INIT* initid);
char* post_notify(UDF_INIT* initid,
                  UDF_ARGS* args,
                  char* result,
                  unsigned long* length,
                  char* is_null,
                  char* error);
#ifdef __cplusplus
}
#endif /* _cplusplus */

#endif /* _POSTUDF_H_ */
