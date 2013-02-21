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
	// 00 endian
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
	PCPOINTLIST *pl = pc_patch_to_points(pa);
	pc_point_get_double_by_name(pl->points[0], "X", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 0.02, 0.000001);
	pc_point_get_double_by_name(pl->points[1], "Intensity", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 8, 0.000001);

	str = pc_patch_to_string(pa);
	CU_ASSERT_STRING_EQUAL(str, "[ 0 : (0.02, 0.03, 0.05, 6), (0.02, 0.03, 0.05, 8) ]");
	// printf("\n%s\n",str);

	pc_pointlist_free(pl);
	pc_patch_free(pa);
	pcfree(wkb);	
}


static void 
test_patch_hex_out()
{
	// 00 endian
	// 00000000 pcid
	// 00000000 compression
	// 00000002 npoints
	// 0000000200000003000000050006 pt1 (XYZi)
	// 0000000200000003000000050008 pt2 (XYZi)
	
	static char *wkt_result = "[ 0 : (0.02, 0.03, 0.05, 6), (0.02, 0.03, 0.05, 8) ]";
	static char *hexresult_xdr = 
	   "0000000000000000000000000200000002000000030000000500060000000200000003000000050008";
	static char *hexresult_ndr = 
	   "0100000000000000000200000002000000030000000500000006000200000003000000050000000800";

	double d0[4] = { 0.02, 0.03, 0.05, 6 };
	double d1[4] = { 0.02, 0.03, 0.05, 8 };

	PCPOINT *pt0 = pc_point_from_double_array(simpleschema, d0, 4);
	PCPOINT *pt1 = pc_point_from_double_array(simpleschema, d1, 4);

	PCPATCH *pa;		
	uint8_t *wkb;
	size_t wkbsize;
	char *hexwkb;
	char *wkt;

	PCPOINTLIST *pl = pc_pointlist_make(2);
	pc_pointlist_add_point(pl, pt0);
	pc_pointlist_add_point(pl, pt1);
	
	pa = pc_patch_from_points(pl);
	wkb = pc_patch_to_wkb(pa, &wkbsize);
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
	
	wkt = pc_patch_to_string(pa);
	CU_ASSERT_STRING_EQUAL(wkt, wkt_result);
	
	pc_pointlist_free(pl);
	pc_patch_free(pa);
	pcfree(hexwkb);
	pcfree(wkb);
	pcfree(wkt);
}

#if 0

/** Convert RLE bytes to value bytes */
uint8_t* pc_bytes_run_length_decode(const uint8_t *bytes_rle, size_t bytes_rle_size, uint32_t interpretation, size_t *bytes_nelems);
#endif

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
	
	bytes = "aaaabbbbccdde";
	nr = pc_bytes_run_count(bytes, PC_UINT8, strlen(bytes));
	CU_ASSERT_EQUAL(nr, 5);

	bytes = "a";
	nr = pc_bytes_run_count(bytes, PC_UINT8, strlen(bytes));
	CU_ASSERT_EQUAL(nr, 1);

	bytes = "aa";
	nr = pc_bytes_run_count(bytes, PC_UINT8, strlen(bytes));
	CU_ASSERT_EQUAL(nr, 1);

	bytes = "ab";
	nr = pc_bytes_run_count(bytes, PC_UINT8, strlen(bytes));
	CU_ASSERT_EQUAL(nr, 2);

	bytes = "abcdefg";
	nr = pc_bytes_run_count(bytes, PC_UINT8, strlen(bytes));
	CU_ASSERT_EQUAL(nr, 7);

	bytes = "aabcdefg";
	nr = pc_bytes_run_count(bytes, PC_UINT8, strlen(bytes));
	CU_ASSERT_EQUAL(nr, 7);

	bytes = "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
	nr = pc_bytes_run_count(bytes, PC_UINT8, strlen(bytes));
	CU_ASSERT_EQUAL(nr, 1);
	
	bytes_rle = pc_bytes_run_length_encode(bytes, PC_UINT8, strlen(bytes), &bytes_rle_size);
	bytes_de_rle = pc_bytes_run_length_decode(bytes_rle, bytes_rle_size, PC_UINT8, &bytes_nelems);
	CU_ASSERT_EQUAL(memcmp(bytes, bytes_de_rle, strlen(bytes)), 0);
	pcfree(bytes_rle);
	pcfree(bytes_de_rle);

	bytes = "aabcdefg";
	interp = PC_UINT8;
	interp_size = INTERPRETATION_SIZES[interp];
	size = strlen(bytes) / interp_size;
	bytes_rle = pc_bytes_run_length_encode(bytes, interp, size, &bytes_rle_size);
	bytes_de_rle = pc_bytes_run_length_decode(bytes_rle, bytes_rle_size, interp, &bytes_nelems);
	CU_ASSERT_EQUAL(memcmp(bytes, bytes_de_rle, strlen(bytes)), 0);
	CU_ASSERT_EQUAL(bytes_nelems, strlen(bytes)/interp_size);
	pcfree(bytes_rle);
	pcfree(bytes_de_rle);

	bytes = "aaaaaabbbbeeeeeeeeeehhiiiiiioooo";
	interp = PC_UINT32;
	interp_size = INTERPRETATION_SIZES[interp];
	size = strlen(bytes) / interp_size;
	bytes_rle = pc_bytes_run_length_encode(bytes, interp, size, &bytes_rle_size);
	bytes_de_rle = pc_bytes_run_length_decode(bytes_rle, bytes_rle_size, interp, &bytes_nelems);
	CU_ASSERT_EQUAL(memcmp(bytes, bytes_de_rle, strlen(bytes)), 0);
	CU_ASSERT_EQUAL(bytes_nelems, strlen(bytes)/interp_size);
	pcfree(bytes_rle);
	pcfree(bytes_de_rle);

	bytes = "aaaaaabbbbeeeeeeeeeehhiiiiiioooo";
	interp = PC_UINT16;
	interp_size = INTERPRETATION_SIZES[interp];
	size = strlen(bytes) / interp_size;
	bytes_rle = pc_bytes_run_length_encode(bytes, interp, size, &bytes_rle_size);
	bytes_de_rle = pc_bytes_run_length_decode(bytes_rle, bytes_rle_size, interp, &bytes_nelems);
	CU_ASSERT_EQUAL(memcmp(bytes, bytes_de_rle, strlen(bytes)), 0);
	CU_ASSERT_EQUAL(bytes_nelems, strlen(bytes)/interp_size);
	pcfree(bytes_rle);
	pcfree(bytes_de_rle);
	
}

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
	
	/*
	01100001 a
	01100010 b
	01100011 c
	01100000 `
	*/
	bytes = "abc";
	common8 = pc_sigbits_count_8(bytes, strlen(bytes), &count);
	CU_ASSERT_EQUAL(count, 6);
	CU_ASSERT_EQUAL(common8, '`');

	bytes = "abcdef";
	common8 = pc_sigbits_count_8(bytes, strlen(bytes), &count);
	CU_ASSERT_EQUAL(count, 5);
	CU_ASSERT_EQUAL(common8, '`');

	/*
	0110000101100001 aa
	0110001001100010 bb
	0110001101100011 cc
	0110000000000000 24576
	*/
	bytes = "aabbcc";
	count = pc_sigbits_count(bytes, PC_UINT16, strlen(bytes)/2);
	CU_ASSERT_EQUAL(count, 6);

	/*
	"abca" encoded:	
	base      a  b  c  a
	01100000 01 10 11 01
	*/
	bytes = "abcaabcaabcbabcc";
    ebytes = pc_bytes_sigbits_encode(bytes, PC_INT8, strlen(bytes), &ebytes_size);
    CU_ASSERT_EQUAL(ebytes[0], 2);   /* unique bit count */
    CU_ASSERT_EQUAL(ebytes[1], 96);  /* common bits */
    CU_ASSERT_EQUAL(ebytes[2], 109); /* packed byte */
    CU_ASSERT_EQUAL(ebytes[3], 109); /* packed byte */
    CU_ASSERT_EQUAL(ebytes[4], 110); /* packed byte */
    CU_ASSERT_EQUAL(ebytes[5], 111); /* packed byte */
    pcfree(ebytes);

	/*
	"abca" encoded:
	base       a   b   c   d   a   b
	01100000 001 010 011 100 001 010
	*/
    bytes = "abcdab";
    ebytes = pc_bytes_sigbits_encode(bytes, PC_INT8, strlen(bytes), &ebytes_size);
    CU_ASSERT_EQUAL(ebytes[0], 3);   /* unique bit count */
    CU_ASSERT_EQUAL(ebytes[1], 96);  /* common bits */
    CU_ASSERT_EQUAL(ebytes[2], 41);  /* packed byte */
    CU_ASSERT_EQUAL(ebytes[3], 194); /* packed byte */

    bytes = pc_bytes_sigbits_decode(ebytes, PC_INT8, strlen(bytes));
    CU_ASSERT_EQUAL(bytes[0], 'a');
    CU_ASSERT_EQUAL(bytes[1], 'b');
    CU_ASSERT_EQUAL(bytes[2], 'c');
    CU_ASSERT_EQUAL(bytes[3], 'd');
    CU_ASSERT_EQUAL(bytes[4], 'a');
    CU_ASSERT_EQUAL(bytes[5], 'b');
    pcfree(bytes);
    pcfree(ebytes);

	/*
	0110000101100001 24929
	0110000101100010 24930
	0110000101100011 24931
	0110000101100100 24932
	0110000101100101 24933
	0110000101100110 24934
	encoded
	0110000101100 001 010 011 100 101 110
    */
    nelems = 6;
	bytes16 = pcalloc(nelems*sizeof(uint16_t)); 
    bytes16[0] = 24929;
    bytes16[1] = 24930;
    bytes16[2] = 24931;
    bytes16[3] = 24932;
    bytes16[4] = 24933;
    bytes16[5] = 24934;
    
    common16 = pc_sigbits_count_16((uint8_t*)bytes16, nelems, &count);
    CU_ASSERT_EQUAL(common16, 24928);
    CU_ASSERT_EQUAL(count, 13);
    ebytes = pc_bytes_sigbits_encode((uint8_t*)bytes16, PC_INT16, nelems, &ebytes_size);
    pcfree(bytes16);
    ebytes16 = (uint16_t*)ebytes;
    // printf("commonbits %d\n", commonbits);
    CU_ASSERT_EQUAL(ebytes16[0], 3);     /* unique bit count */
    CU_ASSERT_EQUAL(ebytes16[1], 24928); /* common bits */
    CU_ASSERT_EQUAL(ebytes16[2], 10699); /* packed uint16 one */

    /* uint8_t* pc_bytes_sigbits_decode(const uint8_t *bytes, uint32_t interpretation, uint32_t nelems) */
    bytes = pc_bytes_sigbits_decode(ebytes, PC_INT16, nelems);
    pcfree(ebytes);
    bytes16 = (uint16_t*)bytes;
    CU_ASSERT_EQUAL(bytes16[0], 24929);
    CU_ASSERT_EQUAL(bytes16[1], 24930);
    CU_ASSERT_EQUAL(bytes16[2], 24931);
    CU_ASSERT_EQUAL(bytes16[3], 24932);
    CU_ASSERT_EQUAL(bytes16[4], 24933);
    CU_ASSERT_EQUAL(bytes16[5], 24934);
    pcfree(bytes);
    
    nelems = 6;
	bytes32 = pcalloc(nelems*sizeof(uint32_t));
    bytes32[0] = 103241; /* 0000000000000001 1001 0011 0100 1001 */
    bytes32[1] = 103251; /* 0000000000000001 1001 0011 0101 0011 */
    bytes32[2] = 103261; /* 0000000000000001 1001 0011 0101 1101 */
    bytes32[3] = 103271; /* 0000000000000001 1001 0011 0110 0111 */
    bytes32[4] = 103281; /* 0000000000000001 1001 0011 0111 0001 */
    bytes32[5] = 103291; /* 0000000000000001 1001 0011 0111 1011 */
    ebytes = pc_bytes_sigbits_encode((uint8_t*)bytes32, PC_INT32, nelems, &ebytes_size);
    pcfree(bytes32);
    ebytes32 = (uint32_t*)ebytes;
    CU_ASSERT_EQUAL(ebytes32[0], 6);     /* unique bit count */
    CU_ASSERT_EQUAL(ebytes32[1], 103232); /* common bits */
    CU_ASSERT_EQUAL(ebytes32[2], 624388039); /* packed uint32 */
    bytes = pc_bytes_sigbits_decode(ebytes, PC_INT32, nelems);
    pcfree(ebytes32);
    bytes32 = (uint32_t*)bytes;
    CU_ASSERT_EQUAL(bytes32[0], 103241);
    CU_ASSERT_EQUAL(bytes32[1], 103251);
    CU_ASSERT_EQUAL(bytes32[2], 103261);
    CU_ASSERT_EQUAL(bytes32[3], 103271);
    CU_ASSERT_EQUAL(bytes32[4], 103281);
    CU_ASSERT_EQUAL(bytes32[5], 103291);
    pcfree(bytes32);
    
    /*
    uint8_t *
    pc_bytes_zlib_encode(const uint8_t *bytes, uint32_t interpretation,  uint32_t nelems)
    uint8_t *
    pc_bytes_zlib_decode(const uint8_t *bytes, uint32_t interpretation)
    */
    bytes = "abcaabcaabcbabcc";
    ebytes = pc_bytes_zlib_encode(bytes, PC_INT8, strlen(bytes));
    memcpy(&i, ebytes+4, 4);
    CU_ASSERT_EQUAL(i, strlen(bytes)); /* original length value */
    bytes = pc_bytes_zlib_decode(ebytes, PC_INT8);
    CU_ASSERT_EQUAL(bytes[0], 'a');
    CU_ASSERT_EQUAL(bytes[2], 'c');
    CU_ASSERT_EQUAL(bytes[5], 'b');
}

/* REGISTER ***********************************************************/

CU_TestInfo patch_tests[] = {
	PC_TEST(test_endian_flip),
	PC_TEST(test_patch_hex_in),
	PC_TEST(test_patch_hex_out),
	PC_TEST(test_run_length_encoding),
    PC_TEST(test_schema_xy),
	PC_TEST(test_sigbits_encoding),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo patch_suite = {"patch", init_suite, clean_suite, patch_tests};
