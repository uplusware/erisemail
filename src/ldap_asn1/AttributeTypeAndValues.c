/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "Lightweight-Directory-Access-Protocol-V3"
 * 	found in "simple_ldap_asn1.asn"
 */

#include "AttributeTypeAndValues.h"

static asn_TYPE_member_t asn_MBR_vals_3[] = {
	{ ATF_POINTER, 0, 0,
		(ASN_TAG_CLASS_UNIVERSAL | (4 << 2)),
		0,
		&asn_DEF_AttributeValue,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		""
		},
};
static ber_tlv_tag_t asn_DEF_vals_tags_3[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (17 << 2))
};
static asn_SET_OF_specifics_t asn_SPC_vals_specs_3 = {
	sizeof(struct AttributeTypeAndValues__vals),
	offsetof(struct AttributeTypeAndValues__vals, _asn_ctx),
	0,	/* XER encoding is XMLDelimitedItemList */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_vals_3 = {
	"vals",
	"vals",
	SET_OF_free,
	SET_OF_print,
	SET_OF_constraint,
	SET_OF_decode_ber,
	SET_OF_encode_der,
	SET_OF_decode_xer,
	SET_OF_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_vals_tags_3,
	sizeof(asn_DEF_vals_tags_3)
		/sizeof(asn_DEF_vals_tags_3[0]), /* 1 */
	asn_DEF_vals_tags_3,	/* Same as above */
	sizeof(asn_DEF_vals_tags_3)
		/sizeof(asn_DEF_vals_tags_3[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_vals_3,
	1,	/* Single element */
	&asn_SPC_vals_specs_3	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_AttributeTypeAndValues_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct AttributeTypeAndValues, type),
		(ASN_TAG_CLASS_UNIVERSAL | (4 << 2)),
		0,
		&asn_DEF_AttributeDescription,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"type"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct AttributeTypeAndValues, vals),
		(ASN_TAG_CLASS_UNIVERSAL | (17 << 2)),
		0,
		&asn_DEF_vals_3,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"vals"
		},
};
static ber_tlv_tag_t asn_DEF_AttributeTypeAndValues_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_AttributeTypeAndValues_tag2el_1[] = {
    { (ASN_TAG_CLASS_UNIVERSAL | (4 << 2)), 0, 0, 0 }, /* type at 287 */
    { (ASN_TAG_CLASS_UNIVERSAL | (17 << 2)), 1, 0, 0 } /* vals at 289 */
};
static asn_SEQUENCE_specifics_t asn_SPC_AttributeTypeAndValues_specs_1 = {
	sizeof(struct AttributeTypeAndValues),
	offsetof(struct AttributeTypeAndValues, _asn_ctx),
	asn_MAP_AttributeTypeAndValues_tag2el_1,
	2,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_AttributeTypeAndValues = {
	"AttributeTypeAndValues",
	"AttributeTypeAndValues",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_AttributeTypeAndValues_tags_1,
	sizeof(asn_DEF_AttributeTypeAndValues_tags_1)
		/sizeof(asn_DEF_AttributeTypeAndValues_tags_1[0]), /* 1 */
	asn_DEF_AttributeTypeAndValues_tags_1,	/* Same as above */
	sizeof(asn_DEF_AttributeTypeAndValues_tags_1)
		/sizeof(asn_DEF_AttributeTypeAndValues_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_AttributeTypeAndValues_1,
	2,	/* Elements count */
	&asn_SPC_AttributeTypeAndValues_specs_1	/* Additional specs */
};

