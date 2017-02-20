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
static PCSCHEMA *schema_xy = NULL;
static PCSCHEMA *schema_xyz = NULL;
static PCSCHEMA *schema_xym = NULL;
static PCSCHEMA *schema_xyzm = NULL;
static const char *xmlfile  = "data/simple-schema.xml";
static const char *xmlfile_xy   = "data/simple-schema-xy.xml";
static const char *xmlfile_xyz  = "data/simple-schema-xyz.xml";
static const char *xmlfile_xym  = "data/simple-schema-xym.xml";
static const char *xmlfile_xyzm = "data/simple-schema-xyzm.xml";

// SIMPLE SCHEMA
// int32_t x
// int32_t y
// int32_t z
// int16_t intensity

/* Setup/teardown for this suite */
static int
init_suite(void)
{
	char *xmlstr;

	xmlstr = file_to_str(xmlfile);
	schema = pc_schema_from_xml(xmlstr);
	pcfree(xmlstr);
	if ( !schema ) return 1;

	xmlstr = file_to_str(xmlfile_xy);
	schema_xy = pc_schema_from_xml(xmlstr);
	pcfree(xmlstr);
	if ( !schema_xy ) return 1;

	xmlstr = file_to_str(xmlfile_xyz);
	schema_xyz = pc_schema_from_xml(xmlstr);
	pcfree(xmlstr);
	if ( !schema_xyz ) return 1;

	xmlstr = file_to_str(xmlfile_xym);
	schema_xym = pc_schema_from_xml(xmlstr);
	pcfree(xmlstr);
	if ( !schema_xym ) return 1;

	xmlstr = file_to_str(xmlfile_xyzm);
	schema_xyzm = pc_schema_from_xml(xmlstr);
	pcfree(xmlstr);
	if ( !schema_xyzm ) return 1;
	return 0;
}

static int
clean_suite(void)
{
	pc_schema_free(schema);
	pc_schema_free(schema_xy);
	pc_schema_free(schema_xyz);
	pc_schema_free(schema_xym);
	pc_schema_free(schema_xyzm);
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
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "X", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, 0.01, 0.000001);
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "Y", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, 0.02, 0.000001);
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "Z", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, 0.03, 0.000001);
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "Intensity", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, 4, 0.0001);
	CU_ASSERT_FAILURE(pc_point_get_double_by_name(pt, "M", &d));
	pc_point_free(pt);
	pcfree(wkb);

	hexbuf = "01010000000100000002000000030000000500";
	hexsize = strlen(hexbuf);
	wkb = bytes_from_hexbytes(hexbuf, hexsize);
	pt = pc_point_from_wkb(schema, wkb, hexsize/2);
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "X", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, 0.01, 0.000001);
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "Y", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, 0.02, 0.000001);
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "Z", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, 0.03, 0.000001);
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "Intensity", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, 5, 0.0001);
	CU_ASSERT_FAILURE(pc_point_get_double_by_name(pt, "M", &d));
	pc_point_free(pt);
	pcfree(wkb);

}

static void
test_point_access()
{
	PCPOINT *pt;
	double a1, a2, a3, a4, b1, b2, b3, b4;
	int idx = 0;
	double *allvals;

	pt = pc_point_make(schema);
	CU_ASSERT_PTR_NOT_NULL( pt );

	/* One at a time */
	idx = 0;
	a1 = 1.5;
	CU_ASSERT_SUCCESS(pc_point_set_double_by_index(pt, idx, a1));
	CU_ASSERT_SUCCESS(pc_point_get_double_by_index(pt, idx, &b1));
	// printf("d1=%g, d2=%g\n", a1, b1);
	CU_ASSERT_DOUBLE_EQUAL(a1, b1, 0.0000001);

	idx = 2;
	a2 = 1501500.12;
	CU_ASSERT_SUCCESS(pc_point_set_double_by_index(pt, idx, a2));
	CU_ASSERT_SUCCESS(pc_point_get_double_by_index(pt, idx, &b2));
	CU_ASSERT_DOUBLE_EQUAL(a2, b2, 0.0000001);

	a3 = 91;
	CU_ASSERT_SUCCESS(pc_point_set_double_by_name(pt, "Intensity", a3));
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "Intensity", &b3));
	CU_ASSERT_DOUBLE_EQUAL(a3, b3, 0.0000001);

	pc_point_free(pt);

	/* All at once */
	pt = pc_point_make(schema);
	a1 = 1.5;
	a2 = 1501500.12;
	a3 = 91;
	a4 = 200;
	CU_ASSERT_SUCCESS(pc_point_set_double_by_index(pt, 0, a1));
	CU_ASSERT_SUCCESS(pc_point_set_double_by_index(pt, 1, a2));
	CU_ASSERT_SUCCESS(pc_point_set_double_by_name(pt, "Intensity", a3));
	CU_ASSERT_SUCCESS(pc_point_set_double_by_name(pt, "Z", a4));
	CU_ASSERT_SUCCESS(pc_point_get_double_by_index(pt, 0, &b1));
	CU_ASSERT_SUCCESS(pc_point_get_double_by_index(pt, 1, &b2));
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "Intensity", &b3));
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "Z", &b4));
	CU_ASSERT_DOUBLE_EQUAL(a1, b1, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a2, b2, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a3, b3, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a4, b4, 0.0000001);

	/* as a double array */
	pc_point_set_double_by_index(pt, 0, a1);
	pc_point_set_double_by_index(pt, 1, a2);
	pc_point_set_double_by_index(pt, 2, a3);
	pc_point_set_double_by_index(pt, 3, a4);
	allvals = pc_point_to_double_array(pt);
	CU_ASSERT_DOUBLE_EQUAL(allvals[0], a1, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(allvals[1], a2, 0.0000001);
	//printf("allvals[2]:%g\n", allvals[2]);
	CU_ASSERT_DOUBLE_EQUAL(allvals[2], a3, 0.0000001);
	//printf("allvals[3]:%g\n", allvals[3]);
	CU_ASSERT_DOUBLE_EQUAL(allvals[3], a4, 0.0000001);
	pcfree(allvals);

	pc_point_free(pt);

}

static void
test_point_xyzm()
{
	PCPOINT *pt;
	double x = 1;
	double y = 40;
	double z = 160;
	double m = 640;
	double d;

	pt = pc_point_make(schema_xyz);
	CU_ASSERT_PTR_NOT_NULL( pt );

	CU_ASSERT_SUCCESS(pc_point_set_x(pt, x));
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "X", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, x, 0.000001);
	CU_ASSERT_SUCCESS(pc_point_get_x(pt, &d));
	CU_ASSERT_DOUBLE_EQUAL(d, x, 0.000001);

	CU_ASSERT_SUCCESS(pc_point_set_y(pt, y));
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "Y", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, y, 0.000001);
	CU_ASSERT_SUCCESS(pc_point_get_y(pt, &d));
	CU_ASSERT_DOUBLE_EQUAL(d, y, 0.000001);

	CU_ASSERT_SUCCESS(pc_point_set_z(pt, z));
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "Z", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, z, 0.000001);
	CU_ASSERT_SUCCESS(pc_point_get_z(pt, &d));
	CU_ASSERT_DOUBLE_EQUAL(d, z, 0.000001);

	CU_ASSERT_FAILURE(pc_point_set_m(pt, m));
	CU_ASSERT_FAILURE(pc_point_get_double_by_name(pt, "M", &d));
	CU_ASSERT_FAILURE(pc_point_get_m(pt, &d));

	pc_point_free(pt);

	pt = pc_point_make(schema_xyzm);
	CU_ASSERT_PTR_NOT_NULL( pt );

	CU_ASSERT_SUCCESS(pc_point_set_x(pt, x));
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "X", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, x, 0.000001);
	CU_ASSERT_SUCCESS(pc_point_get_x(pt, &d));
	CU_ASSERT_DOUBLE_EQUAL(d, x, 0.000001);

	CU_ASSERT_SUCCESS(pc_point_set_y(pt, y));
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "Y", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, y, 0.000001);
	CU_ASSERT_SUCCESS(pc_point_get_y(pt, &d));
	CU_ASSERT_DOUBLE_EQUAL(d, y, 0.000001);

	CU_ASSERT_SUCCESS(pc_point_set_z(pt, z));
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "Z", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, z, 0.000001);
	CU_ASSERT_SUCCESS(pc_point_get_z(pt, &d));
	CU_ASSERT_DOUBLE_EQUAL(d, z, 0.000001);

	CU_ASSERT_SUCCESS(pc_point_set_m(pt, m));
	CU_ASSERT_SUCCESS(pc_point_get_double_by_name(pt, "M", &d));
	CU_ASSERT_DOUBLE_EQUAL(d, m, 0.000001);
	CU_ASSERT_SUCCESS(pc_point_get_m(pt, &d));
	CU_ASSERT_DOUBLE_EQUAL(d, m, 0.000001);

	pc_point_free(pt);
}

/* REGISTER ***********************************************************/

CU_TestInfo point_tests[] = {
	PC_TEST(test_point_hex_inout),
	PC_TEST(test_point_access),
	PC_TEST(test_point_xyzm),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo point_suite = {
	.pName = "point",
	.pInitFunc = init_suite,
	.pCleanupFunc = clean_suite,
	.pTests = point_tests
};
