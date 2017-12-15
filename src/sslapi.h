/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _SSL_API_
#define _SSL_API_

#include "util/trace.h"
#include "util/general.h"

BOOL create_ssl(int sockfd, const char* ca_crt_root, const char* ca_crt_server, const char* ca_password, const char* ca_key_server,
    BOOL ca_enable_verify_client, SSL** pp_ssl, SSL_CTX** pp_ssl_ctx, unsigned int idle_timeout);

BOOL connect_ssl(int sockfd, const char* ca_crt_root, const char* ca_crt_client, const char* ca_password, const char* ca_key_client,
    SSL** pp_ssl, SSL_CTX** pp_ssl_ctx, unsigned int idle_timeout);
    
BOOL close_ssl(SSL* p_ssl, SSL_CTX* p_ssl_ctx);

#endif /* _SSL_API_ */