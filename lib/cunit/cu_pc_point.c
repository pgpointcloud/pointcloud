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
static const char *xmlfile = "data/simple-schema.xml";

// SIMPLE SCHEMA
// int32_t x
// int32_t y
// int32_t z
// int16_t intensity

/* Setup/teardown for this suite */
static int
init_suite(void)
{
	char *xmlstr = file_to_str(xmlfile);
	int rv = pc_schema_from_xml(xmlstr, &schema);
	pcfree(xmlstr);
	if ( rv == PC_FAILURE ) return 1;
	return 0;
}

static int
clean_suite(void)
{
	pc_schema_free(schema);
	return 0;
}


/* TESTS **************************************************************/

static void
test_point_hex_inout()
{
    // byte:     endianness (1 = NDR, 0 = XDR)
    // uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    // uchar[]:  pointdata (interpret relative to pcid)

	double d;
	char *hexbuf = "00000000010000000100000002000000030004";
	size_t hexsize = strlen(hexbuf);
	uint8_t *wkb = bytes_from_hexbytes(hexbuf, hexsize);
	PCPOINT *pt = pc_point_from_wkb(schema, wkb, hexsize/2);
	pc_point_get_double_by_name(pt, "X", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 0.01, 0.000001);
	pc_point_get_double_by_name(pt, "Y", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 0.02, 0.000001);
	pc_point_get_double_by_name(pt, "Z", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 0.03, 0.000001);
	pc_point_get_double_by_name(pt, "Intensity", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 4, 0.0001);
	pc_point_free(pt);
	pcfree(wkb);

	hexbuf = "01010000000100000002000000030000000500";
	hexsize = strlen(hexbuf);
	wkb = bytes_from_hexbytes(hexbuf, hexsize);
	pt = pc_point_from_wkb(schema, wkb, hexsize/2);
	pc_point_get_double_by_name(pt, "X", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 0.01, 0.000001);
	pc_point_get_double_by_name(pt, "Y", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 0.02, 0.000001);
	pc_point_get_double_by_name(pt, "Z", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 0.03, 0.000001);
	pc_point_get_double_by_name(pt, "Intensity", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 5, 0.0001);
	pc_point_free(pt);
	pcfree(wkb);

}


/* REGISTER ***********************************************************/

CU_TestInfo point_tests[] = {
	PC_TEST(test_point_hex_inout),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo point_suite = {"point", init_suite, clean_suite, point_tests};
