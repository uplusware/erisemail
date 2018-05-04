/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "Lightweight-Directory-Access-Protocol-V3"
 * 	found in "simple_ldap_asn1.asn"
 */

#include "SortResult.h"

static int
sortResult_2_constraint(asn_TYPE_descriptor_t *td, const void *sptr,
			asn_app_constraint_failed_f *ctfailcb, void *app_key) {
	/* Replace with underlying type checker */
	td->check_constraints = asn_DEF_ENUMERATED.check_constraints;
	return td->check_constraints(td, sptr, ctfailcb, app_key);
}

/*
 * This type is implemented using ENUMERATED,
 * so here we adjust the DEF accordingly.
 */
static void
sortResult_2_inherit_TYPE_descriptor(asn_TYPE_descriptor_t *td) {
	td->free_struct    = asn_DEF_ENUMERATED.free_struct;
	td->print_struct   = asn_DEF_ENUMERATED.print_struct;
	td->ber_decoder    = asn_DEF_ENUMERATED.ber_decoder;
	td->der_encoder    = asn_DEF_ENUMERATED.der_encoder;
	td->xer_decoder    = asn_DEF_ENUMERATED.xer_decoder;
	td->xer_encoder    = asn_DEF_ENUMERATED.xer_encoder;
	td->uper_decoder   = asn_DEF_ENUMERATED.uper_decoder;
	td->uper_encoder   = asn_DEF_ENUMERATED.uper_encoder;
	if(!td->per_constraints)
		td->per_constraints = asn_DEF_ENUMERATED.per_constraints;
	td->elements       = asn_DEF_ENUMERATED.elements;
	td->elements_count = asn_DEF_ENUMERATED.elements_count;
     /* td->specifics      = asn_DEF_ENUMERATED.specifics;	// Defined explicitly */
}

static void
sortResult_2_free(asn_TYPE_descriptor_t *td,
		void *struct_ptr, int contents_only) {
	sortResult_2_inherit_TYPE_descriptor(td);
	td->free_struct(td, struct_ptr, contents_only);
}

static int
sortResult_2_print(asn_TYPE_descriptor_t *td, const void *struct_ptr,
		int ilevel, asn_app_consume_bytes_f *cb, void *app_key) {
	sortResult_2_inherit_TYPE_descriptor(td);
	return td->print_struct(td, struct_ptr, ilevel, cb, app_key);
}

static asn_dec_rval_t
sortResult_2_decode_ber(asn_codec_ctx_t *opt_codec_ctx, asn_TYPE_descriptor_t *td,
		void **structure, const void *bufptr, size_t size, int tag_mode) {
	sortResult_2_inherit_TYPE_descriptor(td);
	return td->ber_decoder(opt_codec_ctx, td, structure, bufptr, size, tag_mode);
}

static asn_enc_rval_t
sortResult_2_encode_der(asn_TYPE_descriptor_t *td,
		void *structure, int tag_mode, ber_tlv_tag_t tag,
		asn_app_consume_bytes_f *cb, void *app_key) {
	sortResult_2_inherit_TYPE_descriptor(td);
	return td->der_encoder(td, structure, tag_mode, tag, cb, app_key);
}

static asn_dec_rval_t
sortResult_2_decode_xer(asn_codec_ctx_t *opt_codec_ctx, asn_TYPE_descriptor_t *td,
		void **structure, const char *opt_mname, const void *bufptr, size_t size) {
	sortResult_2_inherit_TYPE_descriptor(td);
	return td->xer_decoder(opt_codec_ctx, td, structure, opt_mname, bufptr, size);
}

static asn_enc_rval_t
sortResult_2_encode_xer(asn_TYPE_descriptor_t *td, void *structure,
		int ilevel, enum xer_encoder_flags_e flags,
		asn_app_consume_bytes_f *cb, void *app_key) {
	sortResult_2_inherit_TYPE_descriptor(td);
	return td->xer_encoder(td, structure, ilevel, flags, cb, app_key);
}

static asn_INTEGER_enum_map_t asn_MAP_sortResult_value2enum_2[] = {
	{ 0,	7,	"success" },
	{ 1,	15,	"operationsError" },
	{ 3,	17,	"timeLimitExceeded" },
	{ 8,	18,	"strongAuthRequired" },
	{ 11,	18,	"adminLimitExceeded" },
	{ 16,	15,	"noSuchAttribute" },
	{ 18,	21,	"inappropriateMatching" },
	{ 50,	24,	"insufficientAccessRights" },
	{ 51,	4,	"busy" },
	{ 53,	18,	"unwillingToPerform" },
	{ 80,	5,	"other" }
};
static unsigned int asn_MAP_sortResult_enum2value_2[] = {
	4,	/* adminLimitExceeded(11) */
	8,	/* busy(51) */
	6,	/* inappropriateMatching(18) */
	7,	/* insufficientAccessRights(50) */
	5,	/* noSuchAttribute(16) */
	1,	/* operationsError(1) */
	10,	/* other(80) */
	3,	/* strongAuthRequired(8) */
	0,	/* success(0) */
	2,	/* timeLimitExceeded(3) */
	9	/* unwillingToPerform(53) */
};
static asn_INTEGER_specifics_t asn_SPC_sortResult_specs_2 = {
	asn_MAP_sortResult_value2enum_2,	/* "tag" => N; sorted by tag */
	asn_MAP_sortResult_enum2value_2,	/* N => "tag"; sorted by N */
	11,	/* Number of elements in the maps */
	0,	/* Enumeration is not extensible */
	1,	/* Strict enumeration */
	0,	/* Native long size */
	0
};
static ber_tlv_tag_t asn_DEF_sortResult_tags_2[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (10 << 2))
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_sortResult_2 = {
	"sortResult",
	"sortResult",
	sortResult_2_free,
	sortResult_2_print,
	sortResult_2_constraint,
	sortResult_2_decode_ber,
	sortResult_2_encode_der,
	sortResult_2_decode_xer,
	sortResult_2_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_sortResult_tags_2,
	sizeof(asn_DEF_sortResult_tags_2)
		/sizeof(asn_DEF_sortResult_tags_2[0]), /* 1 */
	asn_DEF_sortResult_tags_2,	/* Same as above */
	sizeof(asn_DEF_sortResult_tags_2)
		/sizeof(asn_DEF_sortResult_tags_2[0]), /* 1 */
	0,	/* No PER visible constraints */
	0, 0,	/* Defined elsewhere */
	&asn_SPC_sortResult_specs_2	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_SortResult_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct SortResult, sortResult),
		(ASN_TAG_CLASS_UNIVERSAL | (10 << 2)),
		0,
		&asn_DEF_sortResult_2,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"sortResult"
		},
	{ ATF_POINTER, 1, offsetof(struct SortResult, attributeType),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_AttributeDescription,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"attributeType"
		},
};
static ber_tlv_tag_t asn_DEF_SortResult_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_SortResult_tag2el_1[] = {
    { (ASN_TAG_CLASS_UNIVERSAL | (10 << 2)), 0, 0, 0 }, /* sortResult at 401 */
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 1, 0, 0 } /* attributeType at 421 */
};
static asn_SEQUENCE_specifics_t asn_SPC_SortResult_specs_1 = {
	sizeof(struct SortResult),
	offsetof(struct SortResult, _asn_ctx),
	asn_MAP_SortResult_tag2el_1,
	2,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_SortResult = {
	"SortResult",
	"SortResult",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_SortResult_tags_1,
	sizeof(asn_DEF_SortResult_tags_1)
		/sizeof(asn_DEF_SortResult_tags_1[0]), /* 1 */
	asn_DEF_SortResult_tags_1,	/* Same as above */
	sizeof(asn_DEF_SortResult_tags_1)
		/sizeof(asn_DEF_SortResult_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_SortResult_1,
	2,	/* Elements count */
	&asn_SPC_SortResult_specs_1	/* Additional specs */
};

