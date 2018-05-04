/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "Lightweight-Directory-Access-Protocol-V3"
 * 	found in "simple_ldap_asn1.asn"
 */

#ifndef	_AuthenticationChoice_H_
#define	_AuthenticationChoice_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Simple.h"
#include "SaslCredentials.h"
#include <OCTET_STRING.h>
#include <constr_CHOICE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum AuthenticationChoice_PR {
	AuthenticationChoice_PR_NOTHING,	/* No components present */
	AuthenticationChoice_PR_simple,
	AuthenticationChoice_PR_sasl,
	AuthenticationChoice_PR_ntlmsspNegotiate,
	AuthenticationChoice_PR_ntlmsspAuth
} AuthenticationChoice_PR;

/* AuthenticationChoice */
typedef struct AuthenticationChoice {
	AuthenticationChoice_PR present;
	union AuthenticationChoice_u {
		Simple_t	 simple;
		SaslCredentials_t	 sasl;
		OCTET_STRING_t	 ntlmsspNegotiate;
		OCTET_STRING_t	 ntlmsspAuth;
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} AuthenticationChoice_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_AuthenticationChoice;

#ifdef __cplusplus
}
#endif

#endif	/* _AuthenticationChoice_H_ */
#include <asn_internal.h>
