/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "Lightweight-Directory-Access-Protocol-V3"
 * 	found in "simple_ldap_asn1.asn"
 */

#ifndef	_SortResult_H_
#define	_SortResult_H_


#include <asn_application.h>

/* Including external dependencies */
#include <ENUMERATED.h>
#include "AttributeDescription.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum SortResult__sortResult {
	SortResult__sortResult_success	= 0,
	SortResult__sortResult_operationsError	= 1,
	SortResult__sortResult_timeLimitExceeded	= 3,
	SortResult__sortResult_strongAuthRequired	= 8,
	SortResult__sortResult_adminLimitExceeded	= 11,
	SortResult__sortResult_noSuchAttribute	= 16,
	SortResult__sortResult_inappropriateMatching	= 18,
	SortResult__sortResult_insufficientAccessRights	= 50,
	SortResult__sortResult_busy	= 51,
	SortResult__sortResult_unwillingToPerform	= 53,
	SortResult__sortResult_other	= 80
} e_SortResult__sortResult;

/* SortResult */
typedef struct SortResult {
	ENUMERATED_t	 sortResult;
	AttributeDescription_t	*attributeType	/* OPTIONAL */;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} SortResult_t;

/* Implementation */
/* extern asn_TYPE_descriptor_t asn_DEF_sortResult_2;	// (Use -fall-defs-global to expose) */
extern asn_TYPE_descriptor_t asn_DEF_SortResult;

#ifdef __cplusplus
}
#endif

#endif	/* _SortResult_H_ */
#include <asn_internal.h>
