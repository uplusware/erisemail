/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "Lightweight-Directory-Access-Protocol-V3"
 * 	found in "simple_ldap_asn1.asn"
 */

#ifndef	_BindResponse_H_
#define	_BindResponse_H_


#include <asn_application.h>

/* Including external dependencies */
#include <ENUMERATED.h>
#include "LDAPDN.h"
#include "ErrorMessage.h"
#include "ServerSaslCreds.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum BindResponse__resultCode {
	BindResponse__resultCode_success	= 0,
	BindResponse__resultCode_operationsError	= 1,
	BindResponse__resultCode_protocolError	= 2,
	BindResponse__resultCode_timeLimitExceeded	= 3,
	BindResponse__resultCode_sizeLimitExceeded	= 4,
	BindResponse__resultCode_compareFalse	= 5,
	BindResponse__resultCode_compareTrue	= 6,
	BindResponse__resultCode_authMethodNotSupported	= 7,
	BindResponse__resultCode_strongAuthRequired	= 8,
	BindResponse__resultCode_referral	= 10,
	BindResponse__resultCode_adminLimitExceeded	= 11,
	BindResponse__resultCode_unavailableCriticalExtension	= 12,
	BindResponse__resultCode_confidentialityRequired	= 13,
	BindResponse__resultCode_saslBindInProgress	= 14,
	BindResponse__resultCode_noSuchAttribute	= 16,
	BindResponse__resultCode_undefinedAttributeType	= 17,
	BindResponse__resultCode_inappropriateMatching	= 18,
	BindResponse__resultCode_constraintViolation	= 19,
	BindResponse__resultCode_attributeOrValueExists	= 20,
	BindResponse__resultCode_invalidAttributeSyntax	= 21,
	BindResponse__resultCode_noSuchObject	= 32,
	BindResponse__resultCode_aliasProblem	= 33,
	BindResponse__resultCode_invalidDNSyntax	= 34,
	BindResponse__resultCode_aliasDereferencingProblem	= 36,
	BindResponse__resultCode_inappropriateAuthentication	= 48,
	BindResponse__resultCode_invalidCredentials	= 49,
	BindResponse__resultCode_insufficientAccessRights	= 50,
	BindResponse__resultCode_busy	= 51,
	BindResponse__resultCode_unavailable	= 52,
	BindResponse__resultCode_unwillingToPerform	= 53,
	BindResponse__resultCode_loopDetect	= 54,
	BindResponse__resultCode_namingViolation	= 64,
	BindResponse__resultCode_objectClassViolation	= 65,
	BindResponse__resultCode_notAllowedOnNonLeaf	= 66,
	BindResponse__resultCode_notAllowedOnRDN	= 67,
	BindResponse__resultCode_entryAlreadyExists	= 68,
	BindResponse__resultCode_objectClassModsProhibited	= 69,
	BindResponse__resultCode_affectsMultipleDSAs	= 71,
	BindResponse__resultCode_other	= 80,
	BindResponse__resultCode_canceled	= 118,
	BindResponse__resultCode_noSuchOperation	= 119,
	BindResponse__resultCode_tooLate	= 120,
	BindResponse__resultCode_cannotCancel	= 121
} e_BindResponse__resultCode;

/* Forward declarations */
struct Referral;

/* BindResponse */
typedef struct BindResponse {
	ENUMERATED_t	 resultCode;
	LDAPDN_t	 matchedDN;
	ErrorMessage_t	 errorMessage;
	struct Referral	*referral	/* OPTIONAL */;
	ServerSaslCreds_t	*serverSaslCreds	/* OPTIONAL */;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} BindResponse_t;

/* Implementation */
/* extern asn_TYPE_descriptor_t asn_DEF_resultCode_2;	// (Use -fall-defs-global to expose) */
extern asn_TYPE_descriptor_t asn_DEF_BindResponse;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "Referral.h"

#endif	/* _BindResponse_H_ */
#include <asn_internal.h>