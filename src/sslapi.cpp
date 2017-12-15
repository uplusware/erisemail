/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "sslapi.h"
#include "posixname.h"

BOOL create_ssl(int sockfd, const char* ca_crt_root, const char* ca_crt_server, const char* ca_password, const char* ca_key_server,
    BOOL ca_enable_verify_client, SSL** pp_ssl, SSL_CTX** pp_ssl_ctx, unsigned int idle_timeout)
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
        
		fprintf(stderr, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
		goto FAIL_CLEAN_SSL_3;
	}

	if(ca_enable_verify_client)
	{
    	SSL_CTX_set_verify(*pp_ssl_ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    	SSL_CTX_set_verify_depth(*pp_ssl_ctx, 4);
	}
	
	SSL_CTX_load_verify_locations(*pp_ssl_ctx, ca_crt_root, NULL);
	if(SSL_CTX_use_certificate_file(*pp_ssl_ctx, ca_crt_server, SSL_FILETYPE_PEM) <= 0)
	{
		
		fprintf(stderr, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
		goto FAIL_CLEAN_SSL_3;
	}

	SSL_CTX_set_default_passwd_cb_userdata(*pp_ssl_ctx, (char*)ca_password);
	if(SSL_CTX_use_PrivateKey_file(*pp_ssl_ctx, ca_key_server, SSL_FILETYPE_PEM) <= 0)
	{
		
		fprintf(stderr, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
		goto FAIL_CLEAN_SSL_3;

	}
	if(!SSL_CTX_check_private_key(*pp_ssl_ctx))
	{
		
		fprintf(stderr, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
		goto FAIL_CLEAN_SSL_3;
	}
	
	ssl_rc = SSL_CTX_set_cipher_list(*pp_ssl_ctx, "ALL");
    if(ssl_rc == 0)
    {
        
		fprintf(stderr, "SSL_CTX_set_cipher_list: %s", ERR_error_string(ERR_get_error(),NULL));
        goto FAIL_CLEAN_SSL_3;
    }
	SSL_CTX_set_mode(*pp_ssl_ctx, SSL_MODE_AUTO_RETRY);

	*pp_ssl = SSL_new(*pp_ssl_ctx);
	if(!*pp_ssl)
	{
		
		fprintf(stderr, "SSL_new: %s", ERR_error_string(ERR_get_error(),NULL));
		goto FAIL_CLEAN_SSL_2;
	}
	ssl_rc = SSL_set_fd(*pp_ssl, sockfd);
    if(ssl_rc == 0)
    {
        
		fprintf(stderr, "SSL_set_fd: %s", ERR_error_string(ERR_get_error(),NULL));
        goto FAIL_CLEAN_SSL_2;
    }
    ssl_rc = SSL_set_cipher_list(*pp_ssl, "ALL");
    if(ssl_rc == 0)
    {
        
		fprintf(stderr, "SSL_set_cipher_list: %s", ERR_error_string(ERR_get_error(),NULL));
        goto FAIL_CLEAN_SSL_2;
    }
    
    do
    {
        ssl_rc = SSL_accept(*pp_ssl);
        if(ssl_rc < 0)
        {
            int ret = SSL_get_error(*pp_ssl, ssl_rc);
                
            if(ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
            {
                fd_set mask;
                struct timeval timeout;
        
                timeout.tv_sec = idle_timeout;
                timeout.tv_usec = 0;

                FD_ZERO(&mask);
                FD_SET(sockfd, &mask);
                
                int res = select(sockfd + 1, ret == SSL_ERROR_WANT_READ ? &mask : NULL, ret == SSL_ERROR_WANT_WRITE ? &mask : NULL, NULL, &timeout);

                if( res == 1)
                {
                    continue;
                }
                else /* timeout or error */
                {
                    
                    fprintf(stderr, "SSL_accept: %s %s", ERR_error_string(ERR_get_error(),NULL), strerror(errno));
                    goto FAIL_CLEAN_SSL_2;
                }
            }
            else
            {
                
                fprintf(stderr, "SSL_accept: %s", ERR_error_string(ERR_get_error(),NULL));
                goto FAIL_CLEAN_SSL_2;
            }

        }
        else if(ssl_rc == 0)
        {
            
            fprintf(stderr, "SSL_accept: %s", ERR_error_string(ERR_get_error(),NULL));
                
            goto FAIL_CLEAN_SSL_1;
        }
        else if(ssl_rc == 1)
        {
            break;
        }
    } while(1);
        
    b_ssl_accepted = TRUE;
    
    if(ca_enable_verify_client)
    {
        ssl_rc = SSL_get_verify_result(*pp_ssl);
        if(ssl_rc != X509_V_OK)
        {
            
            fprintf(stderr, "SSL_get_verify_result: %s", ERR_error_string(ERR_get_error(),NULL));
            goto FAIL_CLEAN_SSL_1;
        }

		X509* client_cert;
		client_cert = SSL_get_peer_certificate(*pp_ssl);
		if (client_cert != NULL)
		{
			X509_free (client_cert);
		}
		else
		{
			
            fprintf(stderr, "SSL_get_peer_certificate: %s", ERR_error_string(ERR_get_error(),NULL));
			goto FAIL_CLEAN_SSL_1;
		}
	}

    return TRUE;

FAIL_CLEAN_SSL_1:
    if(*pp_ssl && b_ssl_accepted)
    {
		SSL_shutdown(*pp_ssl);
        b_ssl_accepted = FALSE;
    }
FAIL_CLEAN_SSL_2:
    if(*pp_ssl)
    {
		SSL_free(*pp_ssl);
        *pp_ssl = NULL;
    }
FAIL_CLEAN_SSL_3:
    if(*pp_ssl_ctx)
    {
		SSL_CTX_free(*pp_ssl_ctx);
        *pp_ssl_ctx = NULL;
    }
    return FALSE;
}

BOOL connect_ssl(int sockfd,  const char* ca_crt_root, const char* ca_crt_client, const char* ca_password, const char* ca_key_client, SSL** pp_ssl, SSL_CTX** pp_ssl_ctx, unsigned int idle_timeout)
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
        
        fprintf(stderr, "SSL_CTX_new: %s\n", ERR_error_string(ERR_get_error(),NULL));
        goto FAIL_CLEAN_SSL_3;
    }

    if(ca_crt_root && ca_crt_client && ca_password && ca_key_client
        && ca_crt_root[0] != '\0' && ca_crt_client[0] != '\0' && ca_password[0] != '\0' && ca_key_client[0] != '\0')
    {
        SSL_CTX_load_verify_locations(*pp_ssl_ctx, ca_crt_root, NULL);
        if(SSL_CTX_use_certificate_file(*pp_ssl_ctx, ca_crt_client, SSL_FILETYPE_PEM) <= 0)
        {
            
            fprintf(stderr, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
            goto FAIL_CLEAN_SSL_2;
        }

        SSL_CTX_set_default_passwd_cb_userdata(*pp_ssl_ctx, (char*)ca_password);
        if(SSL_CTX_use_PrivateKey_file(*pp_ssl_ctx, ca_key_client, SSL_FILETYPE_PEM) <= 0)
        {
            
            fprintf(stderr, "SSL_CTX_use_PrivateKey_file: %s, %s", ERR_error_string(ERR_get_error(),NULL), ca_key_client);
            goto FAIL_CLEAN_SSL_2;

        }
        
        if(!SSL_CTX_check_private_key(*pp_ssl_ctx))
        {
            
            fprintf(stderr, "SSL_CTX_check_private_key: %s", ERR_error_string(ERR_get_error(),NULL));
            goto FAIL_CLEAN_SSL_2;
        }
    }
    
    *pp_ssl = SSL_new(*pp_ssl_ctx);
    if(!*pp_ssl)
    {
        
        fprintf(stderr, "SSL_new: %s\n", ERR_error_string(ERR_get_error(),NULL));
        goto FAIL_CLEAN_SSL_2;
    }
    
    if(SSL_set_fd(*pp_ssl, sockfd) <= 0)
    {
        
        fprintf(stderr, "SSL_set_fd: %s\n", ERR_error_string(ERR_get_error(),NULL));
        goto FAIL_CLEAN_SSL_1;
    }
    
    do
    {
        int ssl_rc = SSL_connect(*pp_ssl);
        if(ssl_rc < 0)
        {
            int ret = SSL_get_error(*pp_ssl, ssl_rc);
            if(ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
            {
                fd_set mask;
                struct timeval timeout;
        
                timeout.tv_sec = idle_timeout;
                timeout.tv_usec = 0;

                FD_ZERO(&mask);
                FD_SET(sockfd, &mask);
                
                int res = select(sockfd + 1, ret == SSL_ERROR_WANT_READ ? &mask : NULL, ret == SSL_ERROR_WANT_WRITE ? &mask : NULL, NULL, &timeout);

                if( res == 1)
                {
                    continue;
                }
                else /* timeout or error */
                {
                    
                    fprintf(stderr, "SSL_connect: %s %s", ERR_error_string(ERR_get_error(),NULL), strerror(errno));
                    goto FAIL_CLEAN_SSL_2;
                }
            }
            else
            {
                
                fprintf(stderr, "SSL_connect: %s", ERR_error_string(ERR_get_error(),NULL));
                goto FAIL_CLEAN_SSL_2;
            }

        }
        else if(ssl_rc == 0)
        {
            
            fprintf(stderr, "SSL_accept: %s", ERR_error_string(ERR_get_error(),NULL));
                
            goto FAIL_CLEAN_SSL_1;
        }
        else if(ssl_rc == 1)
        {
            break;
        }
    } while(1);
    
    X509* client_cert;
    client_cert = SSL_get_peer_certificate(*pp_ssl);
    if (client_cert != NULL)
    {
        
        X509_free (client_cert);
    }
    else
    {
        fprintf(stderr, "SSL_get_peer_certificate: %s", ERR_error_string(ERR_get_error(),NULL));
        goto FAIL_CLEAN_SSL_1;
    }
        
    return TRUE;

FAIL_CLEAN_SSL_1:
    if(*pp_ssl)
    {
		SSL_free(*pp_ssl);
        *pp_ssl = NULL;
    }
FAIL_CLEAN_SSL_2:
    if(*pp_ssl_ctx)
    {
		SSL_CTX_free(*pp_ssl_ctx);
        *pp_ssl_ctx = NULL;
    }
FAIL_CLEAN_SSL_3:
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