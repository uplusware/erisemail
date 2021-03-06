/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "Lightweight-Directory-Access-Protocol-V3"
 * 	found in "simple_ldap_asn1.asn"
 */

#ifndef	_SortKeyList_H_
#define	_SortKeyList_H_


#include <asn_application.h>

/* Including external dependencies */
#include <asn_SEQUENCE_OF.h>
#include "AttributeDescription.h"
#include "MatchingRuleId.h"
#include <BOOLEAN.h>
#include <constr_SEQUENCE.h>
#include <constr_SEQUENCE_OF.h>

#ifdef __cplusplus
extern "C" {
#endif
struct SortKeyList__Member {
		AttributeDescription_t	 attributeType;
		MatchingRuleId_t	*orderingRule	/* OPTIONAL */;
		BOOLEAN_t	*reverseOrder	/* DEFAULT FALSE */;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	};
/* SortKeyList */
typedef struct SortKeyList {
	A_SEQUENCE_OF(struct SortKeyList__Member) list;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} SortKeyList_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_SortKeyList;

#ifdef __cplusplus
}
#endif

#endif	/* _SortKeyList_H_ */
#include <asn_internal.h>
