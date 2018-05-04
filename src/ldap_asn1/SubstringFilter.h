/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "Lightweight-Directory-Access-Protocol-V3"
 * 	found in "simple_ldap_asn1.asn"
 */

#ifndef	_SubstringFilter_H_
#define	_SubstringFilter_H_


#include <asn_application.h>

/* Including external dependencies */
#include "AttributeDescription.h"
#include <asn_SEQUENCE_OF.h>
#include "LDAPString.h"
#include <constr_CHOICE.h>
#include <constr_SEQUENCE_OF.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum SubstringFilter__substrings__Member_PR {
	SubstringFilter__substrings__Member_PR_NOTHING,	/* No components present */
	SubstringFilter__substrings__Member_PR_initial,
	SubstringFilter__substrings__Member_PR_any,
	SubstringFilter__substrings__Member_PR_final
} SubstringFilter__substrings__Member_PR;

struct SubstringFilter__substrings__Member {
			SubstringFilter__substrings__Member_PR present;
			union SubstringFilter__substrings__Member_u {
				LDAPString_t	 initial;
				LDAPString_t	 any;
				LDAPString_t	 final;
			} choice;
			
			/* Context for parsing across buffer boundaries */
			asn_struct_ctx_t _asn_ctx;
		};
        
/* SubstringFilter */
typedef struct SubstringFilter {
	AttributeDescription_t	 type;
	struct SubstringFilter__substrings {
		A_SEQUENCE_OF(struct SubstringFilter__substrings__Member) list;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} substrings;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} SubstringFilter_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_SubstringFilter;

#ifdef __cplusplus
}
#endif

#endif	/* _SubstringFilter_H_ */
#include <asn_internal.h>
