/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "Lightweight-Directory-Access-Protocol-V3"
 * 	found in "simple_ldap_asn1.asn"
 */

#include "SyncInfoValue.h"

static asn_TYPE_member_t asn_MBR_refreshDelete_3[] = {
	{ ATF_POINTER, 2, offsetof(struct SyncInfoValue__refreshDelete, cookie),
		(ASN_TAG_CLASS_UNIVERSAL | (4 << 2)),
		0,
		&asn_DEF_OCTET_STRING,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"cookie"
		},
	{ ATF_POINTER, 1, offsetof(struct SyncInfoValue__refreshDelete, refreshDone),
		(ASN_TAG_CLASS_UNIVERSAL | (1 << 2)),
		0,
		&asn_DEF_BOOLEAN,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"refreshDone"
		},
};
static ber_tlv_tag_t asn_DEF_refreshDelete_tags_3[] = {
	(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_refreshDelete_tag2el_3[] = {
    { (ASN_TAG_CLASS_UNIVERSAL | (1 << 2)), 1, 0, 0 }, /* refreshDone at 501 */
    { (ASN_TAG_CLASS_UNIVERSAL | (4 << 2)), 0, 0, 0 } /* cookie at 500 */
};
static asn_SEQUENCE_specifics_t asn_SPC_refreshDelete_specs_3 = {
	sizeof(struct SyncInfoValue__refreshDelete),
	offsetof(struct SyncInfoValue__refreshDelete, _asn_ctx),
	asn_MAP_refreshDelete_tag2el_3,
	2,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_refreshDelete_3 = {
	"refreshDelete",
	"refreshDelete",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_refreshDelete_tags_3,
	sizeof(asn_DEF_refreshDelete_tags_3)
		/sizeof(asn_DEF_refreshDelete_tags_3[0]) - 1, /* 1 */
	asn_DEF_refreshDelete_tags_3,	/* Same as above */
	sizeof(asn_DEF_refreshDelete_tags_3)
		/sizeof(asn_DEF_refreshDelete_tags_3[0]), /* 2 */
	0,	/* No PER visible constraints */
	asn_MBR_refreshDelete_3,
	2,	/* Elements count */
	&asn_SPC_refreshDelete_specs_3	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_refreshPresent_6[] = {
	{ ATF_POINTER, 2, offsetof(struct SyncInfoValue__refreshPresent, cookie),
		(ASN_TAG_CLASS_UNIVERSAL | (4 << 2)),
		0,
		&asn_DEF_OCTET_STRING,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"cookie"
		},
	{ ATF_POINTER, 1, offsetof(struct SyncInfoValue__refreshPresent, refreshDone),
		(ASN_TAG_CLASS_UNIVERSAL | (1 << 2)),
		0,
		&asn_DEF_BOOLEAN,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"refreshDone"
		},
};
static ber_tlv_tag_t asn_DEF_refreshPresent_tags_6[] = {
	(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_refreshPresent_tag2el_6[] = {
    { (ASN_TAG_CLASS_UNIVERSAL | (1 << 2)), 1, 0, 0 }, /* refreshDone at 505 */
    { (ASN_TAG_CLASS_UNIVERSAL | (4 << 2)), 0, 0, 0 } /* cookie at 504 */
};
static asn_SEQUENCE_specifics_t asn_SPC_refreshPresent_specs_6 = {
	sizeof(struct SyncInfoValue__refreshPresent),
	offsetof(struct SyncInfoValue__refreshPresent, _asn_ctx),
	asn_MAP_refreshPresent_tag2el_6,
	2,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_refreshPresent_6 = {
	"refreshPresent",
	"refreshPresent",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_refreshPresent_tags_6,
	sizeof(asn_DEF_refreshPresent_tags_6)
		/sizeof(asn_DEF_refreshPresent_tags_6[0]) - 1, /* 1 */
	asn_DEF_refreshPresent_tags_6,	/* Same as above */
	sizeof(asn_DEF_refreshPresent_tags_6)
		/sizeof(asn_DEF_refreshPresent_tags_6[0]), /* 2 */
	0,	/* No PER visible constraints */
	asn_MBR_refreshPresent_6,
	2,	/* Elements count */
	&asn_SPC_refreshPresent_specs_6	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_syncUUIDs_12[] = {
	{ ATF_POINTER, 0, 0,
		(ASN_TAG_CLASS_UNIVERSAL | (4 << 2)),
		0,
		&asn_DEF_SyncUUID,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		""
		},
};
static ber_tlv_tag_t asn_DEF_syncUUIDs_tags_12[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (17 << 2))
};
static asn_SET_OF_specifics_t asn_SPC_syncUUIDs_specs_12 = {
	sizeof(struct SyncInfoValue__syncIdSet__syncUUIDs),
	offsetof(struct SyncInfoValue__syncIdSet__syncUUIDs, _asn_ctx),
	0,	/* XER encoding is XMLDelimitedItemList */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_syncUUIDs_12 = {
	"syncUUIDs",
	"syncUUIDs",
	SET_OF_free,
	SET_OF_print,
	SET_OF_constraint,
	SET_OF_decode_ber,
	SET_OF_encode_der,
	SET_OF_decode_xer,
	SET_OF_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_syncUUIDs_tags_12,
	sizeof(asn_DEF_syncUUIDs_tags_12)
		/sizeof(asn_DEF_syncUUIDs_tags_12[0]), /* 1 */
	asn_DEF_syncUUIDs_tags_12,	/* Same as above */
	sizeof(asn_DEF_syncUUIDs_tags_12)
		/sizeof(asn_DEF_syncUUIDs_tags_12[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_syncUUIDs_12,
	1,	/* Single element */
	&asn_SPC_syncUUIDs_specs_12	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_syncIdSet_9[] = {
	{ ATF_POINTER, 2, offsetof(struct SyncInfoValue__syncIdSet, cookie),
		(ASN_TAG_CLASS_UNIVERSAL | (4 << 2)),
		0,
		&asn_DEF_OCTET_STRING,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"cookie"
		},
	{ ATF_POINTER, 1, offsetof(struct SyncInfoValue__syncIdSet, refreshDeletes),
		(ASN_TAG_CLASS_UNIVERSAL | (1 << 2)),
		0,
		&asn_DEF_BOOLEAN,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"refreshDeletes"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct SyncInfoValue__syncIdSet, syncUUIDs),
		(ASN_TAG_CLASS_UNIVERSAL | (17 << 2)),
		0,
		&asn_DEF_syncUUIDs_12,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"syncUUIDs"
		},
};
static ber_tlv_tag_t asn_DEF_syncIdSet_tags_9[] = {
	(ASN_TAG_CLASS_CONTEXT | (3 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_syncIdSet_tag2el_9[] = {
    { (ASN_TAG_CLASS_UNIVERSAL | (1 << 2)), 1, 0, 0 }, /* refreshDeletes at 509 */
    { (ASN_TAG_CLASS_UNIVERSAL | (4 << 2)), 0, 0, 0 }, /* cookie at 508 */
    { (ASN_TAG_CLASS_UNIVERSAL | (17 << 2)), 2, 0, 0 } /* syncUUIDs at 511 */
};
static asn_SEQUENCE_specifics_t asn_SPC_syncIdSet_specs_9 = {
	sizeof(struct SyncInfoValue__syncIdSet),
	offsetof(struct SyncInfoValue__syncIdSet, _asn_ctx),
	asn_MAP_syncIdSet_tag2el_9,
	3,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_syncIdSet_9 = {
	"syncIdSet",
	"syncIdSet",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_syncIdSet_tags_9,
	sizeof(asn_DEF_syncIdSet_tags_9)
		/sizeof(asn_DEF_syncIdSet_tags_9[0]) - 1, /* 1 */
	asn_DEF_syncIdSet_tags_9,	/* Same as above */
	sizeof(asn_DEF_syncIdSet_tags_9)
		/sizeof(asn_DEF_syncIdSet_tags_9[0]), /* 2 */
	0,	/* No PER visible constraints */
	asn_MBR_syncIdSet_9,
	3,	/* Elements count */
	&asn_SPC_syncIdSet_specs_9	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_SyncInfoValue_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct SyncInfoValue, choice.newcookie),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_OCTET_STRING,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"newcookie"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct SyncInfoValue, choice.refreshDelete),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		0,
		&asn_DEF_refreshDelete_3,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"refreshDelete"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct SyncInfoValue, choice.refreshPresent),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		0,
		&asn_DEF_refreshPresent_6,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"refreshPresent"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct SyncInfoValue, choice.syncIdSet),
		(ASN_TAG_CLASS_CONTEXT | (3 << 2)),
		0,
		&asn_DEF_syncIdSet_9,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"syncIdSet"
		},
};
static asn_TYPE_tag2member_t asn_MAP_SyncInfoValue_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* newcookie at 498 */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* refreshDelete at 500 */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 }, /* refreshPresent at 504 */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 3, 0, 0 } /* syncIdSet at 508 */
};
static asn_CHOICE_specifics_t asn_SPC_SyncInfoValue_specs_1 = {
	sizeof(struct SyncInfoValue),
	offsetof(struct SyncInfoValue, _asn_ctx),
	offsetof(struct SyncInfoValue, present),
	sizeof(((struct SyncInfoValue *)0)->present),
	asn_MAP_SyncInfoValue_tag2el_1,
	4,	/* Count of tags in the map */
	0,
	-1	/* Extensions start */
};
asn_TYPE_descriptor_t asn_DEF_SyncInfoValue = {
	"SyncInfoValue",
	"SyncInfoValue",
	CHOICE_free,
	CHOICE_print,
	CHOICE_constraint,
	CHOICE_decode_ber,
	CHOICE_encode_der,
	CHOICE_decode_xer,
	CHOICE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	CHOICE_outmost_tag,
	0,	/* No effective tags (pointer) */
	0,	/* No effective tags (count) */
	0,	/* No tags (pointer) */
	0,	/* No tags (count) */
	0,	/* No PER visible constraints */
	asn_MBR_SyncInfoValue_1,
	4,	/* Elements count */
	&asn_SPC_SyncInfoValue_specs_1	/* Additional specs */
};

