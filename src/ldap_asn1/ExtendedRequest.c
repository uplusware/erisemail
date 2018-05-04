/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "Lightweight-Directory-Access-Protocol-V3"
 * 	found in "simple_ldap_asn1.asn"
 */

#include "ExtendedRequest.h"

static asn_TYPE_member_t asn_MBR_ExtendedRequest_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct ExtendedRequest, requestName),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_LDAPOID,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"requestName"
		},
	{ ATF_POINTER, 1, offsetof(struct ExtendedRequest, requestValue),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_OCTET_STRING,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"requestValue"
		},
};
static ber_tlv_tag_t asn_DEF_ExtendedRequest_tags_1[] = {
	(ASN_TAG_CLASS_APPLICATION | (23 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_ExtendedRequest_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* requestName at 327 */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 } /* requestValue at 328 */
};
static asn_SEQUENCE_specifics_t asn_SPC_ExtendedRequest_specs_1 = {
	sizeof(struct ExtendedRequest),
	offsetof(struct ExtendedRequest, _asn_ctx),
	asn_MAP_ExtendedRequest_tag2el_1,
	2,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_ExtendedRequest = {
	"ExtendedRequest",
	"ExtendedRequest",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_ExtendedRequest_tags_1,
	sizeof(asn_DEF_ExtendedRequest_tags_1)
		/sizeof(asn_DEF_ExtendedRequest_tags_1[0]) - 1, /* 1 */
	asn_DEF_ExtendedRequest_tags_1,	/* Same as above */
	sizeof(asn_DEF_ExtendedRequest_tags_1)
		/sizeof(asn_DEF_ExtendedRequest_tags_1[0]), /* 2 */
	0,	/* No PER visible constraints */
	asn_MBR_ExtendedRequest_1,
	2,	/* Elements count */
	&asn_SPC_ExtendedRequest_specs_1	/* Additional specs */
};

