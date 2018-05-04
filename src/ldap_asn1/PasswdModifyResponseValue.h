/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "Lightweight-Directory-Access-Protocol-V3"
 * 	found in "simple_ldap_asn1.asn"
 */

#ifndef	_PasswdModifyResponseValue_H_
#define	_PasswdModifyResponseValue_H_


#include <asn_application.h>

/* Including external dependencies */
#include <OCTET_STRING.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PasswdModifyResponseValue */
typedef struct PasswdModifyResponseValue {
	OCTET_STRING_t	*genPasswd	/* OPTIONAL */;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} PasswdModifyResponseValue_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_PasswdModifyResponseValue;

#ifdef __cplusplus
}
#endif

#endif	/* _PasswdModifyResponseValue_H_ */
#include <asn_internal.h>
