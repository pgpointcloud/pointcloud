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
static const char *xmlfile = "data/pdal-schema.xml";
static const char *simplexmlfile = "data/simple-schema.xml";

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

	return 0;
}

static int 
clean_suite(void)
{
	pc_schema_free(schema);
	pc_schema_free(simpleschema);
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
	b1 = pc_point_get_double_by_name(pt, "X");
	b2 = pc_point_get_double_by_name(pt, "Z");
	b3 = pc_point_get_double_by_name(pt, "Intensity");
	b4 = pc_point_get_double_by_name(pt, "UserData");
	CU_ASSERT_DOUBLE_EQUAL(a1, b1, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a2, b2, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a3, b3, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a4, b4, 0.0000001);	
	
	ptr = uncompressed_bytes_flip_endian(pt->data, schema, 1);
	pcfree(pt->data);
	pt->data = uncompressed_bytes_flip_endian(ptr, schema, 1);
	
	b1 = pc_point_get_double_by_name(pt, "X");
	b2 = pc_point_get_double_by_name(pt, "Z");
	b3 = pc_point_get_double_by_name(pt, "Intensity");
	b4 = pc_point_get_double_by_name(pt, "UserData");
	CU_ASSERT_DOUBLE_EQUAL(a1, b1, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a2, b2, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a3, b3, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a4, b4, 0.0000001);		
}

static void 
test_patch_hex_inout()
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
	d = pc_point_get_double_by_name(pl->points[0], "X");
	CU_ASSERT_DOUBLE_EQUAL(d, 0.02, 0.000001);
	d = pc_point_get_double_by_name(pl->points[1], "Intensity");
	CU_ASSERT_DOUBLE_EQUAL(d, 8, 0.000001);

	str = pc_patch_to_string(pa);
	CU_ASSERT_STRING_EQUAL(str, "[ 0 : (0.02, 0.03, 0.05, 6), (0.02, 0.03, 0.05, 8) ]");
	// printf("\n%s\n",str);

	pc_pointlist_free(pl);
	pc_patch_free(pa);
	pcfree(wkb);	
}



/* REGISTER ***********************************************************/

CU_TestInfo patch_tests[] = {
	PG_TEST(test_endian_flip),
	PG_TEST(test_patch_hex_inout),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo patch_suite = {"patch", init_suite, clean_suite, patch_tests};
