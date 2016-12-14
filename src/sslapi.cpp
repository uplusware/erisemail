/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "sslapi.h"

static char SSLERR_LOGNAME[256] = "/var/log/erisemail/sslerr.log";
static char SSLERR_LCKNAME[256] = "/.ERISEMAIL_SSLERR.LOG";
    
BOOL create_ssl(int sockfd, 
    const char* ca_crt_root,
    const char* ca_crt_server,
    const char* ca_password,
    const char* ca_key_server,
    BOOL enableclientcacheck,
    SSL** pp_ssl, SSL_CTX** pp_ssl_ctx)
{
    int ssl_rc = -1;
    BOOL b_ssl_accepted;
    X509* client_cert;
	SSL_METHOD* meth = NULL;
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
    meth = (SSL_METHOD*)TLS_server_method();
#else
    meth = (SSL_METHOD*)SSLv23_server_method();
#endif /* OPENSSL_VERSION_NUMBER */
	*pp_ssl_ctx = SSL_CTX_new(meth);
	if(!*pp_ssl_ctx)
	{
        CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
		uTrace.Write(Trace_Error, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
		goto clean_ssl3;
	}

	if(enableclientcacheck)
	{
    	SSL_CTX_set_verify(*pp_ssl_ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    	SSL_CTX_set_verify_depth(*pp_ssl_ctx, 4);
	}
	
	SSL_CTX_load_verify_locations(*pp_ssl_ctx, ca_crt_root, NULL);
	if(SSL_CTX_use_certificate_file(*pp_ssl_ctx, ca_crt_server, SSL_FILETYPE_PEM) <= 0)
	{
		CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
		uTrace.Write(Trace_Error, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
		goto clean_ssl3;
	}

	SSL_CTX_set_default_passwd_cb_userdata(*pp_ssl_ctx, (char*)ca_password);
	if(SSL_CTX_use_PrivateKey_file(*pp_ssl_ctx, ca_key_server, SSL_FILETYPE_PEM) <= 0)
	{
		CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
		uTrace.Write(Trace_Error, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
		goto clean_ssl3;

	}
	if(!SSL_CTX_check_private_key(*pp_ssl_ctx))
	{
		CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
		uTrace.Write(Trace_Error, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
		goto clean_ssl3;
	}
	
	ssl_rc = SSL_CTX_set_cipher_list(*pp_ssl_ctx, "ALL");
    if(ssl_rc == 0)
    {
        CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
		uTrace.Write(Trace_Error, "SSL_CTX_set_cipher_list: %s", ERR_error_string(ERR_get_error(),NULL));
        goto clean_ssl3;
    }
	SSL_CTX_set_mode(*pp_ssl_ctx, SSL_MODE_AUTO_RETRY);

	*pp_ssl = SSL_new(*pp_ssl_ctx);
	if(!*pp_ssl)
	{
		CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
		uTrace.Write(Trace_Error, "SSL_new: %s", ERR_error_string(ERR_get_error(),NULL));
		goto clean_ssl2;
	}
	ssl_rc = SSL_set_fd(*pp_ssl, sockfd);
    if(ssl_rc == 0)
    {
        CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
		uTrace.Write(Trace_Error, "SSL_set_fd: %s", ERR_error_string(ERR_get_error(),NULL));
        goto clean_ssl2;
    }
    ssl_rc = SSL_set_cipher_list(*pp_ssl, "ALL");
    if(ssl_rc == 0)
    {
        CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
		uTrace.Write(Trace_Error, "SSL_set_cipher_list: %s", ERR_error_string(ERR_get_error(),NULL));
        goto clean_ssl2;
    }
    ssl_rc = SSL_accept(*pp_ssl);
	if(ssl_rc < 0)
	{
        CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
		uTrace.Write(Trace_Error, "SSL_accept: %s", ERR_error_string(ERR_get_error(),NULL));
		goto clean_ssl2;
	}
    else if(ssl_rc = 0)
	{
		goto clean_ssl1;
	}

    b_ssl_accepted = TRUE;
    
    if(enableclientcacheck)
    {
        ssl_rc = SSL_get_verify_result(*pp_ssl);
        if(ssl_rc != X509_V_OK)
        {
            CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
            uTrace.Write(Trace_Error, "SSL_get_verify_result: %s", ERR_error_string(ERR_get_error(),NULL));
            goto clean_ssl1;
        }
    }
        
	if(enableclientcacheck)
	{
		X509* client_cert;
		client_cert = SSL_get_peer_certificate(*pp_ssl);
		if (client_cert != NULL)
		{
			X509_free (client_cert);
		}
		else
		{
			CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
            uTrace.Write(Trace_Error, "SSL_get_peer_certificate: %s", ERR_error_string(ERR_get_error(),NULL));
			goto clean_ssl1;
		}
	}

    return TRUE;

clean_ssl1:
    if(*pp_ssl && b_ssl_accepted)
    {
		SSL_shutdown(*pp_ssl);
        b_ssl_accepted = FALSE;
    }
clean_ssl2:
    if(*pp_ssl)
    {
		SSL_free(*pp_ssl);
        *pp_ssl = NULL;
    }
clean_ssl3:
    if(*pp_ssl_ctx)
    {
		SSL_CTX_free(*pp_ssl_ctx);
        *pp_ssl_ctx = NULL;
    }
    return FALSE;
}

BOOL connect_ssl(int sockfd, 
    const char* ca_crt_root,
    const char* ca_crt_client,
    const char* ca_password,
    const char* ca_key_client,
    SSL** pp_ssl, SSL_CTX** pp_ssl_ctx)
{
    SSL_METHOD* meth = NULL;
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
    meth = (SSL_METHOD*)TLS_client_method();
#else
    meth = (SSL_METHOD*)SSLv23_client_method();
#endif /* OPENSSL_VERSION_NUMBER */
        
    *pp_ssl_ctx = SSL_CTX_new(meth);

    if(!*pp_ssl_ctx)
    {
        CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
        uTrace.Write(Trace_Error, "SSL_CTX_new: %s\n", ERR_error_string(ERR_get_error(),NULL));
        goto clean_ssl3;
    }

    if(ca_crt_root && ca_crt_client && ca_password && ca_key_client)
    {
        SSL_CTX_load_verify_locations(*pp_ssl_ctx, ca_crt_root, NULL);
        if(SSL_CTX_use_certificate_file(*pp_ssl_ctx, ca_crt_client, SSL_FILETYPE_PEM) <= 0)
        {
            CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
            uTrace.Write(Trace_Error, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
            goto clean_ssl2;
        }

        SSL_CTX_set_default_passwd_cb_userdata(*pp_ssl_ctx, (char*)ca_password);
        if(SSL_CTX_use_PrivateKey_file(*pp_ssl_ctx, ca_key_client, SSL_FILETYPE_PEM) <= 0)
        {
            CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
            uTrace.Write(Trace_Error, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
            goto clean_ssl2;

        }
        
        if(!SSL_CTX_check_private_key(*pp_ssl_ctx))
        {
            CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
            uTrace.Write(Trace_Error, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
            goto clean_ssl2;
        }
    }
    
    *pp_ssl = SSL_new(*pp_ssl_ctx);
    if(!*pp_ssl)
    {
        CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
        uTrace.Write(Trace_Error, "SSL_new: %s\n", ERR_error_string(ERR_get_error(),NULL));
        goto clean_ssl2;
    }
    
    if(SSL_set_fd(*pp_ssl, sockfd) <= 0)
    {
        CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
        uTrace.Write(Trace_Error, "SSL_set_fd: %s\n", ERR_error_string(ERR_get_error(),NULL));
        goto clean_ssl1;
    }
    
    if(SSL_connect(*pp_ssl) <= 0)
    {
        CUplusTrace uTrace(SSLERR_LOGNAME, SSLERR_LCKNAME);
        uTrace.Write(Trace_Error, "SSL_connect: %s\n", ERR_error_string(ERR_get_error(),NULL));
        goto clean_ssl1;
    }
        
    return TRUE;

clean_ssl1:
    if(*pp_ssl)
    {
		SSL_free(*pp_ssl);
        *pp_ssl = NULL;
    }
clean_ssl2:
    if(*pp_ssl_ctx)
    {
		SSL_CTX_free(*pp_ssl_ctx);
        *pp_ssl_ctx = NULL;
    }
clean_ssl3:
    return FALSE;
}

BOOL close_ssl(SSL* p_ssl, SSL_CTX* p_ssl_ctx)
{
	if(p_ssl)
    {
		SSL_shutdown(p_ssl);
        SSL_free(p_ssl);
    }
    
    if(p_ssl_ctx)
    {
		SSL_CTX_free(p_ssl_ctx);
    }
}