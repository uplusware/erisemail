/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _ESCAPE_H_
#define _ESCAPE_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <string>
using namespace std;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    void escape(const unsigned char* src, string & dst);
    void unescape(const unsigned char* src, string & dst);
    void encodeURI(const unsigned char* src, string & dst);
    void decodeURI(const unsigned char* src, string & dst);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ESCAPE_H_ */

