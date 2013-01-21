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
const char *xmlfile = "data/pdal-schema.xml";

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
	
	ptr = bytes_flip_endian(pt->data, schema, 1);
	pcfree(pt->data);
	pt->data = bytes_flip_endian(ptr, schema, 1);
	
	b1 = pc_point_get_double_by_name(pt, "X");
	b2 = pc_point_get_double_by_name(pt, "Z");
	b3 = pc_point_get_double_by_name(pt, "Intensity");
	b4 = pc_point_get_double_by_name(pt, "UserData");
	CU_ASSERT_DOUBLE_EQUAL(a1, b1, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a2, b2, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a3, b3, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a4, b4, 0.0000001);		
}





/* REGISTER ***********************************************************/

CU_TestInfo patch_tests[] = {
	PG_TEST(test_endian_flip),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo patch_suite = {"patch", init_suite, clean_suite, patch_tests};
