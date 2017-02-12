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

static PCSCHEMA *simpleschema = NULL;
static const char *simplexmlfile = "data/simple-schema-fine.xml";

/* Setup/teardown for this suite */
static int
init_suite(void)
{
	char *xmlstr = file_to_str(simplexmlfile);
	int rv = pc_schema_from_xml(xmlstr, &simpleschema);
	pcfree(xmlstr);
	if ( rv == PC_FAILURE ) return 1;

	return 0;
}

static int
clean_suite(void)
{
	pc_schema_free(simpleschema);
	return 0;
}

/* TESTS **************************************************************/

#ifdef HAVE_LIBGHT

static void
test_patch_ght()
{
	PCPOINT *pt;
	int i;
	static int npts = 100;
	PCPOINTLIST *pl;
	PCPATCH_GHT *pag;
	PCPATCH_UNCOMPRESSED *pu;

	pl = pc_pointlist_make(npts);

	for ( i = 0; i < npts; i++ )
	{
		pt = pc_point_make(simpleschema);
		pc_point_set_double_by_name(pt, "x", 45 + i*0.000004);
		pc_point_set_double_by_name(pt, "y", 45 + i*0.000001666);
		pc_point_set_double_by_name(pt, "Z", 10 + i*0.34);
		pc_point_set_double_by_name(pt, "intensity", 10);
		pc_pointlist_add_point(pl, pt);
	}

	pag = pc_patch_ght_from_pointlist(pl);
	pc_pointlist_free(pl);

	pu = pc_patch_uncompressed_from_ght(pag);
	CU_ASSERT_EQUAL(npts, pag->npoints);
	CU_ASSERT_EQUAL(npts, pu->npoints);

	CU_ASSERT_DOUBLE_EQUAL(pag->bounds.xmax, 45.0004, 0.0001);
	CU_ASSERT_DOUBLE_EQUAL(pag->bounds.ymax, 45.000165, 0.000001);

	// pl2 = pc_pointlist_from_uncompressed(pu);
	// for ( i = 0; i < npts; i++ )
	// {
	//     PCPOINT *pt = pc_pointlist_get_point(pl2, i);
	//     double x, y, z, ints;
	//     pc_point_get_double_by_name(pt, "x", &x);
	//     pc_point_get_double_by_name(pt, "y", &y);
	//     pc_point_get_double_by_name(pt, "Z", &z);
	//     pc_point_get_double_by_name(pt, "intensity", &ints);
	//     printf("(%g %g %g) %g\n", x, y, z, ints);
	// }
	// pc_pointlist_free(pl2);

	pc_patch_uncompressed_free(pu);
	pc_patch_ght_free(pag);
}

static void
test_patch_ght_filtering()
{
	int dimnum = 2; /* Z */
	PCPOINT *pt;
	int i;
	static int npts = 100;
	PCPOINTLIST *pl;
	PCPATCH_GHT *pag, *pag_filtered;

	pl = pc_pointlist_make(npts);

	for ( i = 0; i < npts; i++ )
	{
		pt = pc_point_make(simpleschema);
		pc_point_set_double_by_name(pt, "x", 45 + i*0.000004);
		pc_point_set_double_by_name(pt, "y", 45 + i*0.000001666);
		pc_point_set_double_by_name(pt, "Z", 10 + i*0.34);
		pc_point_set_double_by_name(pt, "intensity", 10);
		pc_pointlist_add_point(pl, pt);
	}

	pag = pc_patch_ght_from_pointlist(pl);
	pc_pointlist_free(pl);

	pag_filtered = pc_patch_ght_filter(pag, dimnum, PC_LT, 10, 10);
	CU_ASSERT_EQUAL(pag_filtered->npoints, 0);
	pc_patch_ght_free(pag_filtered);

	pag_filtered = pc_patch_ght_filter(pag, dimnum, PC_LT, 11, 11);
	CU_ASSERT_EQUAL(pag_filtered->npoints, 3);
	pc_patch_ght_free(pag_filtered);

	pag_filtered = pc_patch_ght_filter(pag, dimnum, PC_GT, 11, 11);
	CU_ASSERT_EQUAL(pag_filtered->npoints, 97);
	pc_patch_ght_free(pag_filtered);

	pag_filtered = pc_patch_ght_filter(pag, dimnum, PC_BETWEEN, 11, 16);
	CU_ASSERT_EQUAL(pag_filtered->npoints, 15);
	pc_patch_ght_free(pag_filtered);

	pc_patch_ght_free(pag);
}


#endif /* HAVE_LIBGHT */

/* REGISTER ***********************************************************/

CU_TestInfo ght_tests[] = {
#ifdef HAVE_LIBGHT
	PC_TEST(test_patch_ght),
	PC_TEST(test_patch_ght_filtering),
#endif
	CU_TEST_INFO_NULL
};

CU_SuiteInfo ght_suite = {
	.pName = "ght",
	.pInitFunc = init_suite,
	.pCleanupFunc = clean_suite,
	.pTests = ght_tests
};
