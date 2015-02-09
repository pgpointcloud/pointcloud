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
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);
	rv = pc_point_set_double_by_name(pt, "Z", a2);
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);
	rv = pc_point_set_double_by_name(pt, "Intensity", a3);
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);
	rv = pc_point_set_double_by_name(pt, "UserData", a4);
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);
	rv = pc_point_get_double_by_name(pt, "X", &b1);
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);
	rv = pc_point_get_double_by_name(pt, "Z", &b2);
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);
	rv = pc_point_get_double_by_name(pt, "Intensity", &b3);
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);
	rv = pc_point_get_double_by_name(pt, "UserData", &b4);
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);
	CU_ASSERT_DOUBLE_EQUAL(a1, b1, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a2, b2, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a3, b3, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a4, b4, 0.0000001);

	ptr = uncompressed_bytes_flip_endian(pt->data, schema, 1);
	pcfree(pt->data);
	pt->data = uncompressed_bytes_flip_endian(ptr, schema, 1);

	rv = pc_point_get_double_by_name(pt, "X", &b1);
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);
	rv = pc_point_get_double_by_name(pt, "Z", &b2);
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);
	rv = pc_point_get_double_by_name(pt, "Intensity", &b3);
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);
	rv = pc_point_get_double_by_name(pt, "UserData", &b4);
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);
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

	pc_point_get_double_by_name(&(pa->stats->min), "Intensity", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 6, 0.000001);
	pc_point_get_double_by_name(&(pa->stats->max), "Intensity", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 8, 0.000001);
	pc_point_get_double_by_name(&(pa->stats->avg), "Intensity", &d);
	CU_ASSERT_DOUBLE_EQUAL(d, 7, 0.000001);

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
    //size_t z1, z2;
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
    // z1 = pc_patch_dimensional_serialized_size(pch1);
    // printf("z1 %ld\n", z1);

    pds = pc_dimstats_make(simpleschema);
    pc_dimstats_update(pds, pch1);
    pc_dimstats_update(pds, pch1);
    pch2 = pc_patch_dimensional_compress(pch1, pds);
    // z2 = pc_patch_dimensional_serialized_size(pch2);
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
    size_t z1, z2;
    uint8_t *wkb1, *wkb2;

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
    // str = hexbytes_from_bytes(wkb1, z1);
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


static void
test_patch_filter()
{
    int i;
    int npts = 20;
    PCPOINTLIST *pl1, *pl2;
    PCPATCH *pa1, *pa2, *pa3, *pa4;
    char *str1, *str2;

    pl1 = pc_pointlist_make(npts);
    pl2 = pc_pointlist_make(npts);

    for ( i = 0; i < npts; i++ )
    {
        PCPOINT *pt1 = pc_point_make(simpleschema);
        PCPOINT *pt2 = pc_point_make(simpleschema);
        pc_point_set_double_by_name(pt1, "x", i);
        pc_point_set_double_by_name(pt1, "y", i);
        pc_point_set_double_by_name(pt1, "Z", i*0.1);
        pc_point_set_double_by_name(pt1, "intensity", 100-i);
        pc_pointlist_add_point(pl1, pt1);
        pc_point_set_double_by_name(pt2, "x", i);
        pc_point_set_double_by_name(pt2, "y", i);
        pc_point_set_double_by_name(pt2, "Z", i*0.1);
        pc_point_set_double_by_name(pt2, "intensity", 100-i);
        pc_pointlist_add_point(pl2, pt2);
    }

    // PCPATCH* pc_patch_filter(const PCPATCH *pa, uint32_t dimnum, PC_FILTERTYPE filter, double val1, double val2);

    pa1 = (PCPATCH*)pc_patch_dimensional_from_pointlist(pl1);
    // printf("pa1\n%s\n", pc_patch_to_string(pa1));
    pa2 = pc_patch_filter(pa1, 0, PC_GT, 17, 20);
    str1 = pc_patch_to_string(pa2);
    // printf("pa2\n%s\n", str1);
    CU_ASSERT_STRING_EQUAL(str1, "{\"pcid\":0,\"pts\":[[18,18,1.8,82],[19,19,1.9,81]]}");

    pa3 = (PCPATCH*)pc_patch_uncompressed_from_pointlist(pl2);
    // printf("\npa3\n%s\n", pc_patch_to_string(pa3));
    pa4 = pc_patch_filter(pa3, 0, PC_GT, 17, 20);
    str2 = pc_patch_to_string(pa4);
    // printf("\npa4\n%s\n", str2);
    CU_ASSERT_STRING_EQUAL(str2, "{\"pcid\":0,\"pts\":[[18,18,1.8,82],[19,19,1.9,81]]}");

    pcfree(str1);
    pcfree(str2);

    pc_pointlist_free(pl1);
    pc_pointlist_free(pl2);
    pc_patch_free(pa1);
    pc_patch_free(pa3);
    pc_patch_free(pa4);
    pc_patch_free(pa2);
    
    return;



}

/* REGISTER ***********************************************************/

CU_TestInfo patch_tests[] = {
	PC_TEST(test_endian_flip),
	PC_TEST(test_patch_hex_in),
	PC_TEST(test_patch_hex_out),
    PC_TEST(test_schema_xy),
	PC_TEST(test_patch_dimensional),
	PC_TEST(test_patch_dimensional_compression),
	PC_TEST(test_patch_union),
	PC_TEST(test_patch_wkb),
	PC_TEST(test_patch_filter),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo patch_suite = {"patch", init_suite, clean_suite, patch_tests};
