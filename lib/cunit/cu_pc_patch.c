/***********************************************************************
* cu_pc_schema.c
*
*        Testing for the schema API functions
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include "CUnit/Basic.h"
#include "cu_tester.h"

/* GLOBALS ************************************************************/

static PCSCHEMA *schema = NULL;
static PCSCHEMA *simpleschema = NULL;
static PCSCHEMA *lasschema = NULL;
static const char *xmlfile = "data/pdal-schema.xml";
static const char *simplexmlfile = "data/simple-schema.xml";
static const char *lasxmlfile = "data/las-schema.xml";

/* Setup/teardown for this suite */
static int
init_suite(void)
{
	char *xmlstr = file_to_str(xmlfile);
	int rv = pc_schema_from_xml(xmlstr, &schema);
	pcfree(xmlstr);
	if ( rv == PC_FAILURE ) return 1;

	xmlstr = file_to_str(simplexmlfile);
	rv = pc_schema_from_xml(xmlstr, &simpleschema);
	pcfree(xmlstr);
	if ( rv == PC_FAILURE ) return 1;

	xmlstr = file_to_str(lasxmlfile);
	rv = pc_schema_from_xml(xmlstr, &lasschema);
	pcfree(xmlstr);
	if ( rv == PC_FAILURE ) return 1;

	return 0;
}

static int
clean_suite(void)
{
	pc_schema_free(schema);
	pc_schema_free(simpleschema);
	pc_schema_free(lasschema);
	return 0;
}


/* TESTS **************************************************************/

static void
test_endian_flip()
{
	PCPOINT *pt;
	double a1, a2, a3, a4, b1, b2, b3, b4;
	int rv;
	uint8_t *ptr;

	/* All at once */
	pt = pc_point_make(schema);
	a1 = 1.5;
	a2 = 1501500.12;
	a3 = 19112;
	a4 = 200;
	rv = pc_point_set_double_by_name(pt, "X", a1);
	rv = pc_point_set_double_by_name(pt, "Z", a2);
	rv = pc_point_set_double_by_name(pt, "Intensity", a3);
	rv = pc_point_set_double_by_name(pt, "UserData", a4);
	rv = pc_point_get_double_by_name(pt, "X", &b1);
	rv = pc_point_get_double_by_name(pt, "Z", &b2);
	rv = pc_point_get_double_by_name(pt, "Intensity", &b3);
	rv = pc_point_get_double_by_name(pt, "UserData", &b4);
	CU_ASSERT_DOUBLE_EQUAL(a1, b1, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a2, b2, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a3, b3, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a4, b4, 0.0000001);

	ptr = uncompressed_bytes_flip_endian(pt->data, schema, 1);
	pcfree(pt->data);
	pt->data = uncompressed_bytes_flip_endian(ptr, schema, 1);

	rv = pc_point_get_double_by_name(pt, "X", &b1);
	rv = pc_point_get_double_by_name(pt, "Z", &b2);
	rv = pc_point_get_double_by_name(pt, "Intensity", &b3);
	rv = pc_point_get_double_by_name(pt, "UserData", &b4);
	CU_ASSERT_DOUBLE_EQUAL(a1, b1, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a2, b2, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a3, b3, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a4, b4, 0.0000001);
}

static void
test_patch_hex_in()
{
	// 00 endian (big)
	// 00000000 pcid
	// 00000000 compression
	// 00000002 npoints
	// 0000000200000003000000050006 pt1 (XYZi)
	// 0000000200000003000000050008 pt2 (XYZi)
	char *hexbuf = "0000000000000000000000000200000002000000030000000500060000000200000003000000050008";

	double d;
	char *str;
	size_t hexsize = strlen(hexbuf);
	uint8_t *wkb = bytes_from_hexbytes(hexbuf, hexsize);
	PCPATCH *pa = pc_patch_from_wkb(simpleschema, wkb, hexsize/2);
	PCPOINTLIST *pl = pc_pointlist_from_patch(pa);
	pc_point_get_double_by_name(pc_pointlist_get_point(pl, 0), "X", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 0.02, 0.000001);
	pc_point_get_double_by_name(pc_pointlist_get_point(pl, 1), "Intensity", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 8, 0.000001);

	str = pc_patch_to_string(pa);
	CU_ASSERT_STRING_EQUAL(str, "{\"pcid\":0,\"pts\":[[0.02,0.03,0.05,6],[0.02,0.03,0.05,8]]}");
    // printf("\n%s\n",str);

	pc_pointlist_free(pl);
	pc_patch_free(pa);
	pcfree(wkb);
}

/*
* Write an uncompressed patch out to hex
*/
static void
test_patch_hex_out()
{
	// 00 endian
	// 00000000 pcid
	// 00000000 compression
	// 00000002 npoints
	// 0000000200000003000000050006 pt1 (XYZi)
	// 0000000200000003000000050008 pt2 (XYZi)

    static char *wkt_result = "{\"pcid\":0,\"pts\":[[0.02,0.03,0.05,6],[0.02,0.03,0.05,8]]}";
	static char *hexresult_xdr =
	   "0000000000000000000000000200000002000000030000000500060000000200000003000000050008";
	static char *hexresult_ndr =
	   "0100000000000000000200000002000000030000000500000006000200000003000000050000000800";

	double d0[4] = { 0.02, 0.03, 0.05, 6 };
	double d1[4] = { 0.02, 0.03, 0.05, 8 };

	PCPOINT *pt0 = pc_point_from_double_array(simpleschema, d0, 4);
	PCPOINT *pt1 = pc_point_from_double_array(simpleschema, d1, 4);

	PCPATCH_UNCOMPRESSED *pa;
	uint8_t *wkb;
	size_t wkbsize;
	char *hexwkb;
	char *wkt;

	PCPOINTLIST *pl = pc_pointlist_make(2);
	pc_pointlist_add_point(pl, pt0);
	pc_pointlist_add_point(pl, pt1);

	pa = pc_patch_uncompressed_from_pointlist(pl);
	wkb = pc_patch_uncompressed_to_wkb(pa, &wkbsize);
	// printf("wkbsize %zu\n", wkbsize);
	hexwkb = hexbytes_from_bytes(wkb, wkbsize);

	// printf("hexwkb %s\n", hexwkb);
	// printf("hexresult_ndr %s\n", hexresult_ndr);
	// printf("machine_endian %d\n", machine_endian());
	if ( machine_endian() == PC_NDR )
	{
		CU_ASSERT_STRING_EQUAL(hexwkb, hexresult_ndr);
	}
	else
	{
		CU_ASSERT_STRING_EQUAL(hexwkb, hexresult_xdr);
	}

	wkt = pc_patch_uncompressed_to_string(pa);
    // printf("wkt %s\n", wkt);
	CU_ASSERT_STRING_EQUAL(wkt, wkt_result);

	pc_pointlist_free(pl);
	pc_patch_uncompressed_free(pa);
	pcfree(hexwkb);
	pcfree(wkb);
	pcfree(wkt);
}

/*
* Can we read this example point value?
*/
static void
test_schema_xy()
{
    /*
    "Intensity", "ReturnNumber", "NumberOfReturns", "ScanDirectionFlag", "EdgeOfFlightLine", "Classification", "ScanAngleRank", "UserData", "PointSourceId", "Time", "Red", "Green", "Blue", "PointID", "BlockID", "X", "Y", "Z"
    25, 1, 1, 1, 0, 1, 6, 124, 7327, 246093, 39, 57, 56, 20, 0, -125.0417204, 49.2540081, 128.85
    */
	static char *hexpt = "01010000000AE9C90307A1100522A5000019000101010001067C9F1C4953C474650A0E412700390038001400000000000000876B6601962F750155320000";

    uint8_t *bytes =  bytes_from_hexbytes(hexpt, strlen(hexpt));
    PCPOINT *pt;
    double val;

    pt = pc_point_from_wkb(lasschema, bytes, strlen(hexpt)/2);
    pc_point_get_double_by_name(pt, "x", &val);
    CU_ASSERT_DOUBLE_EQUAL(val, -125.0417204, 0.00001);

    pt = pc_point_from_wkb(lasschema, bytes, strlen(hexpt)/2);
    pc_point_get_double_by_name(pt, "y", &val);
    CU_ASSERT_DOUBLE_EQUAL(val, 49.2540081, 0.00001);

}

static PCBYTES initbytes(uint8_t *bytes, size_t size, uint32_t interp)
{
    PCBYTES pcb;
    pcb.bytes = bytes;
    pcb.size = size;
    pcb.interpretation = interp;
    pcb.npoints = pcb.size / pc_interpretation_size(pcb.interpretation);
    pcb.compression = PC_DIM_NONE;
    return pcb;
}

/*
* Run-length encode a byte stream by word.
* Lots of identical words means great
* compression ratios.
*/
static void
test_run_length_encoding()
{
	char *bytes, *bytes_rle, *bytes_de_rle;
	int nr;
	uint32_t bytes_nelems;
	size_t bytes_rle_size;
	size_t size;
	uint8_t interp;
	size_t interp_size;
    PCBYTES pcb, epcb, pcb2;

/*
typedef struct
{
    size_t size;
    uint32_t npoints;
    uint32_t interpretation;
    uint32_t compression;
    uint8_t *bytes;
} PCBYTES;
*/

    bytes = "aaaabbbbccdde";
    pcb = initbytes(bytes, strlen(bytes), PC_UINT8);
	nr = pc_bytes_run_count(&pcb);
	CU_ASSERT_EQUAL(nr, 5);

    bytes = "a";
    pcb = initbytes(bytes, strlen(bytes), PC_UINT8);
	nr = pc_bytes_run_count(&pcb);
	CU_ASSERT_EQUAL(nr, 1);

    bytes = "aa";
    pcb = initbytes(bytes, strlen(bytes), PC_UINT8);
	nr = pc_bytes_run_count(&pcb);
	CU_ASSERT_EQUAL(nr, 1);

    bytes = "ab";
    pcb = initbytes(bytes, strlen(bytes), PC_UINT8);
	nr = pc_bytes_run_count(&pcb);
	CU_ASSERT_EQUAL(nr, 2);

	bytes = "abcdefg";
    pcb = initbytes(bytes, strlen(bytes), PC_UINT8);
	nr = pc_bytes_run_count(&pcb);
	CU_ASSERT_EQUAL(nr, 7);

	bytes = "aabcdefg";
    pcb = initbytes(bytes, strlen(bytes), PC_UINT8);
	nr = pc_bytes_run_count(&pcb);
	CU_ASSERT_EQUAL(nr, 7);

	bytes = "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
    pcb = initbytes(bytes, strlen(bytes), PC_UINT8);
	nr = pc_bytes_run_count(&pcb);
	CU_ASSERT_EQUAL(nr, 1);

    epcb = pc_bytes_run_length_encode(pcb);
    pcb2 = pc_bytes_run_length_decode(epcb);

	CU_ASSERT_EQUAL(memcmp(pcb.bytes, pcb2.bytes, pcb.size), 0);
    CU_ASSERT_EQUAL(pcb.size, pcb2.size);
    CU_ASSERT_EQUAL(pcb.npoints, pcb2.npoints);
	pc_bytes_free(epcb);
	pc_bytes_free(pcb2);

	bytes = "aabcdefg";
    pcb = initbytes(bytes, strlen(bytes), PC_UINT8);
    epcb = pc_bytes_run_length_encode(pcb);
    pcb2 = pc_bytes_run_length_decode(epcb);
	CU_ASSERT_EQUAL(memcmp(pcb.bytes, pcb2.bytes, pcb.size), 0);
    CU_ASSERT_EQUAL(pcb.size, pcb2.size);
    CU_ASSERT_EQUAL(pcb.npoints, pcb2.npoints);
	pc_bytes_free(epcb);
	pc_bytes_free(pcb2);

    bytes = (uint8_t*)((uint32_t[]){ 10, 10, 10, 20, 20, 30, 20, 20 });
    pcb = initbytes(bytes, 8, PC_UINT32);
    epcb = pc_bytes_run_length_encode(pcb);
    pcb2 = pc_bytes_run_length_decode(epcb);
	CU_ASSERT_EQUAL(memcmp(pcb.bytes, pcb2.bytes, pcb.size), 0);
    CU_ASSERT_EQUAL(pcb.size, pcb2.size);
    CU_ASSERT_EQUAL(pcb.npoints, pcb2.npoints);
	pc_bytes_free(epcb);
	pc_bytes_free(pcb2);

    bytes = (uint8_t*)((uint16_t[]){ 10, 10, 10, 20, 20, 30, 20, 20 });
    pcb = initbytes(bytes, 8, PC_UINT16);
    epcb = pc_bytes_run_length_encode(pcb);
    pcb2 = pc_bytes_run_length_decode(epcb);
	CU_ASSERT_EQUAL(memcmp(pcb.bytes, pcb2.bytes, pcb.size), 0);
    CU_ASSERT_EQUAL(pcb.size, pcb2.size);
    CU_ASSERT_EQUAL(pcb.npoints, pcb2.npoints);
	pc_bytes_free(epcb);
	pc_bytes_free(pcb2);

}

/*
* Strip the common bits off a stream and pack the
* remaining bits in behind. Test bit counting and
* round-trip encode/decode paths.
*/
static void
test_sigbits_encoding()
{
    int i;
	uint8_t *bytes, *ebytes;
    uint16_t *bytes16, *ebytes16;
    uint32_t *bytes32, *ebytes32;
    size_t ebytes_size;

	uint32_t count, nelems;
	uint8_t common8;
	uint16_t common16;
	uint32_t common32;
    PCBYTES pcb, epcb, pcb2;

	/*
	01100001 a
	01100010 b
	01100011 c
	01100000 `
	*/
	bytes = "abc";
    pcb = initbytes(bytes, strlen(bytes), PC_UINT8);
    common8 = pc_bytes_sigbits_count_8(&pcb, &count);
	CU_ASSERT_EQUAL(count, 6);
	CU_ASSERT_EQUAL(common8, '`');

	bytes = "abcdef";
    pcb = initbytes(bytes, strlen(bytes), PC_UINT8);
    common8 = pc_bytes_sigbits_count_8(&pcb, &count);
	CU_ASSERT_EQUAL(count, 5);
	CU_ASSERT_EQUAL(common8, '`');

	/*
	0110000101100001 aa
	0110001001100010 bb
	0110001101100011 cc
	0110000000000000 24576
	*/
	bytes = "aabbcc";
    pcb = initbytes(bytes, strlen(bytes), PC_UINT16);
	count = pc_bytes_sigbits_count(&pcb);
	CU_ASSERT_EQUAL(count, 6);

	/*
	"abca" encoded:
	base      a  b  c  a
	01100000 01 10 11 01
	*/
	bytes = "abcaabcaabcbabcc";
    pcb = initbytes(bytes, strlen(bytes), PC_INT8);
    epcb = pc_bytes_sigbits_encode(pcb);
    CU_ASSERT_EQUAL(epcb.bytes[0], 2);   /* unique bit count */
    CU_ASSERT_EQUAL(epcb.bytes[1], 96);  /* common bits */
    CU_ASSERT_EQUAL(epcb.bytes[2], 109); /* packed byte */
    CU_ASSERT_EQUAL(epcb.bytes[3], 109); /* packed byte */
    CU_ASSERT_EQUAL(epcb.bytes[4], 110); /* packed byte */
    CU_ASSERT_EQUAL(epcb.bytes[5], 111); /* packed byte */
    pc_bytes_free(epcb);

	/*
	"abca" encoded:
	base       a   b   c   d   a   b
	01100000 001 010 011 100 001 010
	*/
    bytes = "abcdab";
    pcb = initbytes(bytes, strlen(bytes), PC_INT8);
    epcb = pc_bytes_sigbits_encode(pcb);
    CU_ASSERT_EQUAL(epcb.bytes[0], 3);   /* unique bit count */
    CU_ASSERT_EQUAL(epcb.bytes[1], 96);  /* common bits */
    CU_ASSERT_EQUAL(epcb.bytes[2], 41);  /* packed byte */
    CU_ASSERT_EQUAL(epcb.bytes[3], 194); /* packed byte */

    pcb2 = pc_bytes_sigbits_decode(epcb);
    CU_ASSERT_EQUAL(pcb2.bytes[0], 'a');
    CU_ASSERT_EQUAL(pcb2.bytes[1], 'b');
    CU_ASSERT_EQUAL(pcb2.bytes[2], 'c');
    CU_ASSERT_EQUAL(pcb2.bytes[3], 'd');
    CU_ASSERT_EQUAL(pcb2.bytes[4], 'a');
    CU_ASSERT_EQUAL(pcb2.bytes[5], 'b');
    pc_bytes_free(pcb2);
    pc_bytes_free(epcb);

    /* Test the 16 bit implementation path */
    nelems = 6;
    bytes16 = (uint16_t[]){
        24929, /* 0110000101100001 */
        24930, /* 0110000101100010 */
        24931, /* 0110000101100011 */
        24932, /* 0110000101100100 */
        24933, /* 0110000101100101 */
        24934  /* 0110000101100110 */
        };
    /* encoded 0110000101100 001 010 011 100 101 110 */
    bytes = (uint8_t*)bytes16;
    pcb = initbytes(bytes, nelems*2, PC_INT16);

    /* Test the 16 bit implementation path */
    common16 = pc_bytes_sigbits_count_16(&pcb, &count);
    CU_ASSERT_EQUAL(common16, 24928);
    CU_ASSERT_EQUAL(count, 13);
    epcb = pc_bytes_sigbits_encode(pcb);
    ebytes16 = (uint16_t*)(epcb.bytes);
    // printf("commonbits %d\n", commonbits);
    CU_ASSERT_EQUAL(ebytes16[0], 3);     /* unique bit count */
    CU_ASSERT_EQUAL(ebytes16[1], 24928); /* common bits */
    CU_ASSERT_EQUAL(ebytes16[2], 10699); /* packed uint16 one */

    /* uint8_t* pc_bytes_sigbits_decode(const uint8_t *bytes, uint32_t interpretation, uint32_t nelems) */
    pcb2 = pc_bytes_sigbits_decode(epcb);
    pc_bytes_free(epcb);
    bytes16 = (uint16_t*)(pcb2.bytes);
    CU_ASSERT_EQUAL(bytes16[0], 24929);
    CU_ASSERT_EQUAL(bytes16[1], 24930);
    CU_ASSERT_EQUAL(bytes16[2], 24931);
    CU_ASSERT_EQUAL(bytes16[3], 24932);
    CU_ASSERT_EQUAL(bytes16[4], 24933);
    CU_ASSERT_EQUAL(bytes16[5], 24934);
    pc_bytes_free(pcb2);

    /* Test the 32 bit implementation path */
    nelems = 6;

    bytes32 = (uint32_t[]){
        103241, /* 0000000000000001 1001 0011 0100 1001 */
        103251, /* 0000000000000001 1001 0011 0101 0011 */
        103261, /* 0000000000000001 1001 0011 0101 1101 */
        103271, /* 0000000000000001 1001 0011 0110 0111 */
        103281, /* 0000000000000001 1001 0011 0111 0001 */
        103291  /* 0000000000000001 1001 0011 0111 1011 */
        };
    bytes = (uint8_t*)bytes32;
    pcb = initbytes(bytes, nelems*4, PC_INT32);

    common32 = pc_bytes_sigbits_count_32(&pcb, &count);
    CU_ASSERT_EQUAL(count, 26);     /* unique bit count */
    CU_ASSERT_EQUAL(common32, 103232);

    epcb = pc_bytes_sigbits_encode(pcb);
    ebytes32 = (uint32_t*)(epcb.bytes);
    CU_ASSERT_EQUAL(ebytes32[0], 6);     /* unique bit count */
    CU_ASSERT_EQUAL(ebytes32[1], 103232); /* common bits */
    CU_ASSERT_EQUAL(ebytes32[2], 624388039); /* packed uint32 */

    pcb2 = pc_bytes_sigbits_decode(epcb);
    pc_bytes_free(epcb);
    bytes32 = (uint32_t*)(pcb2.bytes);
    CU_ASSERT_EQUAL(bytes32[0], 103241);
    CU_ASSERT_EQUAL(bytes32[1], 103251);
    CU_ASSERT_EQUAL(bytes32[2], 103261);
    CU_ASSERT_EQUAL(bytes32[3], 103271);
    CU_ASSERT_EQUAL(bytes32[4], 103281);
    CU_ASSERT_EQUAL(bytes32[5], 103291);
    pc_bytes_free(pcb2);

    /* What if all the words are the same? */
    nelems = 6;
    bytes16 = (uint16_t[]){
        24929, /* 0000000000000001 1001 0011 0100 1001 */
        24929, /* 0000000000000001 1001 0011 0101 0011 */
        24929, /* 0000000000000001 1001 0011 0101 1101 */
        24929, /* 0000000000000001 1001 0011 0110 0111 */
        24929, /* 0000000000000001 1001 0011 0111 0001 */
        24929  /* 0000000000000001 1001 0011 0111 1011 */
        };
    bytes = (uint8_t*)bytes16;
    pcb = initbytes(bytes, nelems*2, PC_INT16);
    epcb = pc_bytes_sigbits_encode(pcb);
    pcb2 = pc_bytes_sigbits_decode(epcb);
    pc_bytes_free(epcb);
    pc_bytes_free(pcb2);

}

/*
* Encode and decode a byte stream. Data matches?
*/
static void
test_zlib_encoding()
{
    uint8_t *bytes, *ebytes;
    uint32_t i;
    PCBYTES pcb, epcb, pcb2;
    /*
    uint8_t *
    pc_bytes_zlib_encode(const uint8_t *bytes, uint32_t interpretation,  uint32_t nelems)
    uint8_t *
    pc_bytes_zlib_decode(const uint8_t *bytes, uint32_t interpretation)
    */
    bytes = "abcaabcaabcbabcc";
    pcb = initbytes(bytes, strlen(bytes), PC_INT8);
    epcb = pc_bytes_zlib_encode(pcb);
    pcb2 = pc_bytes_zlib_decode(epcb);
	CU_ASSERT_EQUAL(memcmp(pcb.bytes, pcb2.bytes, pcb.size), 0);
    pc_bytes_free(epcb);
    pc_bytes_free(pcb2);
}

/**
* Pivot a pointlist into a dimlist and back.
* Test for data loss or alteration.
*/
static void
test_patch_dimensional()
{
    PCPOINT *pt;
    int i;
    int npts = 10;
    PCPOINTLIST *pl1, *pl2;
    PCPATCH_DIMENSIONAL *pdl;
    PCDIMSTATS *pds;

    pl1 = pc_pointlist_make(npts);

    for ( i = 0; i < npts; i++ )
    {
        pt = pc_point_make(simpleschema);
        pc_point_set_double_by_name(pt, "x", i*2.0);
        pc_point_set_double_by_name(pt, "y", i*1.9);
        pc_point_set_double_by_name(pt, "Z", i*0.34);
        pc_point_set_double_by_name(pt, "intensity", 10);
        pc_pointlist_add_point(pl1, pt);
    }

    pdl = pc_patch_dimensional_from_pointlist(pl1);
    pl2 = pc_pointlist_from_dimensional(pdl);

    for ( i = 0; i < npts; i++ )
    {
        pt = pc_pointlist_get_point(pl2, i);
        double v1, v2, v3, v4;
        pc_point_get_double_by_name(pt, "x", &v1);
        pc_point_get_double_by_name(pt, "y", &v2);
        pc_point_get_double_by_name(pt, "Z", &v3);
        pc_point_get_double_by_name(pt, "intensity", &v4);
        // printf("%g\n", v4);
        CU_ASSERT_DOUBLE_EQUAL(v1, i*2.0, 0.001);
        CU_ASSERT_DOUBLE_EQUAL(v2, i*1.9, 0.001);
        CU_ASSERT_DOUBLE_EQUAL(v3, i*0.34, 0.001);
        CU_ASSERT_DOUBLE_EQUAL(v4, 10, 0.001);
    }

    pds = pc_dimstats_make(simpleschema);
    pc_dimstats_update(pds, pdl);
    pc_dimstats_update(pds, pdl);


    pc_patch_dimensional_free(pdl);
    pc_pointlist_free(pl1);
    pc_pointlist_free(pl2);
    pc_dimstats_free(pds);
}


static void
test_patch_dimensional_compression()
{
    PCPOINT *pt;
    int i;
    int npts = 400;
    PCPOINTLIST *pl1, *pl2;
    PCPATCH_DIMENSIONAL *pch1, *pch2;
    PCDIMSTATS *pds = NULL;
    size_t z1, z2;
    char *str;

    pl1 = pc_pointlist_make(npts);

    for ( i = 0; i < npts; i++ )
    {
        pt = pc_point_make(simpleschema);
        pc_point_set_double_by_name(pt, "x", i*2.0);
        pc_point_set_double_by_name(pt, "y", i*1.9);
        pc_point_set_double_by_name(pt, "Z", i*0.34);
        pc_point_set_double_by_name(pt, "intensity", 10);
        pc_pointlist_add_point(pl1, pt);
    }

    pch1 = pc_patch_dimensional_from_pointlist(pl1);
    z1 = pc_patch_dimensional_serialized_size(pch1);
    // printf("z1 %ld\n", z1);

    pds = pc_dimstats_make(simpleschema);
    pc_dimstats_update(pds, pch1);
    pc_dimstats_update(pds, pch1);
    pch2 = pc_patch_dimensional_compress(pch1, pds);
    z2 = pc_patch_dimensional_serialized_size(pch2);
    // printf("z2 %ld\n", z2);

    str = pc_dimstats_to_string(pds);
    CU_ASSERT_STRING_EQUAL(str, "{\"ndims\":4,\"total_points\":1200,\"total_patches\":3,\"dims\":[{\"total_runs\":1200,\"total_commonbits\":45,\"recommended_compression\":2},{\"total_runs\":1200,\"total_commonbits\":45,\"recommended_compression\":2},{\"total_runs\":1200,\"total_commonbits\":54,\"recommended_compression\":2},{\"total_runs\":3,\"total_commonbits\":48,\"recommended_compression\":1}]}");
    // printf("%s\n", str);
    pcfree(str);

    pl2 = pc_pointlist_from_dimensional(pch2);

    for ( i = 0; i < npts; i++ )
    {
        pt = pc_pointlist_get_point(pl2, i);
        double v1, v2, v3, v4;
        pc_point_get_double_by_name(pt, "x", &v1);
        pc_point_get_double_by_name(pt, "y", &v2);
        pc_point_get_double_by_name(pt, "Z", &v3);
        pc_point_get_double_by_name(pt, "intensity", &v4);
        // printf("%g\n", v4);
        CU_ASSERT_DOUBLE_EQUAL(v1, i*2.0, 0.001);
        CU_ASSERT_DOUBLE_EQUAL(v2, i*1.9, 0.001);
        CU_ASSERT_DOUBLE_EQUAL(v3, i*0.34, 0.001);
        CU_ASSERT_DOUBLE_EQUAL(v4, 10, 0.001);
    }

    pc_patch_dimensional_free(pch1);
    pc_patch_dimensional_free(pch2);
//    pc_patch_dimensional_free(pch3);
    pc_pointlist_free(pl1);
    pc_pointlist_free(pl2);
    if ( pds ) pc_dimstats_free(pds);
}

static void
test_patch_union()
{
    int i;
    int npts = 20;
    PCPOINTLIST *pl1;
    PCPATCH *pu;
    PCPATCH **palist;
    PCDIMSTATS *pds = NULL;
    size_t z1, z2;
    char *str;

    pl1 = pc_pointlist_make(npts);

    for ( i = 0; i < npts; i++ )
    {
        PCPOINT *pt = pc_point_make(simpleschema);
        pc_point_set_double_by_name(pt, "x", i*2.0);
        pc_point_set_double_by_name(pt, "y", i*1.9);
        pc_point_set_double_by_name(pt, "Z", i*0.34);
        pc_point_set_double_by_name(pt, "intensity", 10);
        pc_pointlist_add_point(pl1, pt);
    }

    palist = pcalloc(2*sizeof(PCPATCH*));

    palist[0] = (PCPATCH*)pc_patch_dimensional_from_pointlist(pl1);
    palist[1] = (PCPATCH*)pc_patch_uncompressed_from_pointlist(pl1);

    pu = pc_patch_from_patchlist(palist, 2);
    CU_ASSERT_EQUAL(pu->npoints, 2*npts);

    pc_pointlist_free(pl1);
    pc_patch_free(pu);
    pc_patch_free(palist[0]);
    pc_patch_free(palist[1]);
    pcfree(palist);
}


static void
test_patch_wkb()
{
    int i;
    int npts = 20;
    PCPOINTLIST *pl1;
    PCPATCH_UNCOMPRESSED *pu1, *pu2;
    PCPATCH *pa1, *pa2, *pa3, *pa4;
    PCDIMSTATS *pds = NULL;
    size_t z1, z2;
    uint8_t *wkb1, *wkb2;
    char *str;

    pl1 = pc_pointlist_make(npts);

    for ( i = 0; i < npts; i++ )
    {
        PCPOINT *pt = pc_point_make(simpleschema);
        pc_point_set_double_by_name(pt, "x", i*2.123);
        pc_point_set_double_by_name(pt, "y", i*2.9);
        pc_point_set_double_by_name(pt, "Z", i*0.3099);
        pc_point_set_double_by_name(pt, "intensity", 13);
        pc_pointlist_add_point(pl1, pt);
    }

    pa1 = (PCPATCH*)pc_patch_dimensional_from_pointlist(pl1);
    wkb1 = pc_patch_to_wkb(pa1, &z1);
    str = hexbytes_from_bytes(wkb1, z1);
    // printf("str\n%s\n",str);
    pa2 = pc_patch_from_wkb(simpleschema, wkb1, z1);

    // printf("pa2\n%s\n",pc_patch_to_string(pa2));

    pa3 = pc_patch_compress(pa2, NULL);

    // printf("pa3\n%s\n",pc_patch_to_string(pa3));

    wkb2 = pc_patch_to_wkb(pa3, &z2);
    pa4 = pc_patch_from_wkb(simpleschema, wkb2, z2);

    // printf("pa4\n%s\n",pc_patch_to_string(pa4));

    pu1 = (PCPATCH_UNCOMPRESSED*)pc_patch_uncompressed_from_dimensional((PCPATCH_DIMENSIONAL*)pa1);
    pu2 = (PCPATCH_UNCOMPRESSED*)pc_patch_uncompressed_from_dimensional((PCPATCH_DIMENSIONAL*)pa4);

    // printf("pu1\n%s\n", pc_patch_to_string((PCPATCH*)pu1));
    // printf("pu2\n%s\n", pc_patch_to_string((PCPATCH*)pu2));

    CU_ASSERT_EQUAL(pu1->datasize, pu2->datasize);
    CU_ASSERT_EQUAL(pu1->npoints, pu2->npoints);
    CU_ASSERT(memcmp(pu1->data, pu2->data, pu1->datasize) == 0);


    pc_pointlist_free(pl1);
    pc_patch_free(pa1);
    pc_patch_free(pa2);
    pcfree(wkb1);
}


/* REGISTER ***********************************************************/

CU_TestInfo patch_tests[] = {
	PC_TEST(test_endian_flip),
	PC_TEST(test_patch_hex_in),
	PC_TEST(test_patch_hex_out),
    PC_TEST(test_schema_xy),
	PC_TEST(test_run_length_encoding),
	PC_TEST(test_sigbits_encoding),
	PC_TEST(test_zlib_encoding),
	PC_TEST(test_patch_dimensional),
	PC_TEST(test_patch_dimensional_compression),
	PC_TEST(test_patch_union),
	PC_TEST(test_patch_wkb),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo patch_suite = {"patch", init_suite, clean_suite, patch_tests};
