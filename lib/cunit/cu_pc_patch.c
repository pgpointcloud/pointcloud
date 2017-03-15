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
static PCSCHEMA *simplelazschema = NULL;
static const char *xmlfile = "data/pdal-schema.xml";
static const char *simplexmlfile = "data/simple-schema.xml";
static const char *lasxmlfile = "data/las-schema.xml";
static const char *simplelazxmlfile = "data/simple-schema-laz.xml";

/* Setup/teardown for this suite */
static int
init_suite(void)
{
	char *xmlstr = file_to_str(xmlfile);
	schema = pc_schema_from_xml(xmlstr);
	pcfree(xmlstr);
	if ( !schema ) return 1;

	xmlstr = file_to_str(simplexmlfile);
	simpleschema = pc_schema_from_xml(xmlstr);
	pcfree(xmlstr);
	if ( !simpleschema ) return 1;

	xmlstr = file_to_str(lasxmlfile);
	lasschema = pc_schema_from_xml(xmlstr);
	pcfree(xmlstr);
	if ( !lasschema ) return 1;

	xmlstr = file_to_str(simplelazxmlfile);
	simplelazschema = pc_schema_from_xml(xmlstr);
	pcfree(xmlstr);
	if ( !simplelazschema ) return 1;

	return 0;
}

static int
clean_suite(void)
{
	pc_schema_free(schema);
	pc_schema_free(simpleschema);
	pc_schema_free(lasschema);
	pc_schema_free(simplelazschema);
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
	pcfree(ptr);

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

	pc_point_free(pt);
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
	pcfree(str);

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

	pc_patch_free((PCPATCH*)pa);
	pc_pointlist_free(pl);
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
	pc_point_free(pt);

	pt = pc_point_from_wkb(lasschema, bytes, strlen(hexpt)/2);
	pc_point_get_double_by_name(pt, "y", &val);
	CU_ASSERT_DOUBLE_EQUAL(val, 49.2540081, 0.00001);
	pc_point_free(pt);

	pcfree(bytes);
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


	pc_patch_free((PCPATCH*)pdl);
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

	pc_patch_free((PCPATCH*)pch1);
	pc_patch_free((PCPATCH*)pch2);
	pc_pointlist_free(pl1);
	pc_pointlist_free(pl2);
	if ( pds ) pc_dimstats_free(pds);
}

static void
test_patch_dimensional_extent()
{
	PCPOINT *pt;
	int i, rv;
	int npts = 2;
	PCPOINTLIST *pl1;
	PCPATCH_DIMENSIONAL *pch1;

	pl1 = pc_pointlist_make(npts);

	for ( i = 1; i <= npts; i++ )
	{
		pt = pc_point_make(simpleschema);
		pc_point_set_double_by_name(pt, "x", 5+i*1);
		pc_point_set_double_by_name(pt, "y", -i*10);
		pc_point_set_double_by_name(pt, "Z", i*0.2);
		pc_point_set_double_by_name(pt, "intensity", -5);
		pc_pointlist_add_point(pl1, pt);
	}

	pch1 = pc_patch_dimensional_from_pointlist(pl1);
		CU_ASSERT_EQUAL(pch1->bounds.xmin, 6);
		CU_ASSERT_EQUAL(pch1->bounds.xmax, 7);
		CU_ASSERT_EQUAL(pch1->bounds.ymin, -20);
		CU_ASSERT_EQUAL(pch1->bounds.ymax, -10);

	rv = pc_patch_dimensional_compute_extent(pch1);
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);
		CU_ASSERT_EQUAL(pch1->bounds.xmin, 6);
		CU_ASSERT_EQUAL(pch1->bounds.xmax, 7);
		CU_ASSERT_EQUAL(pch1->bounds.ymin, -20);
		CU_ASSERT_EQUAL(pch1->bounds.ymax, -10);

	pc_patch_free((PCPATCH*)pch1);
	pc_pointlist_free(pl1);
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
	pcfree(wkb2);

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
	pc_patch_free(pa3);
	pc_patch_free(pa4);
	pc_patch_free((PCPATCH*)pu1);
	pc_patch_free((PCPATCH*)pu2);
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

#if defined(HAVE_LIBGHT) && defined(HAVE_LAZPERF)
static void
test_patch_compress_from_ght_to_lazperf()
{
	PCPOINT *point;
	PCPOINTLIST *pointlist;
	PCPATCH_GHT *patch_ght;
	PCPATCH_LAZPERF *patch_lazperf;

	pointlist = pc_pointlist_make(1);
	point = pc_point_make(simplelazschema);
	pc_point_set_double_by_name(point, "x", 2.0);
	pc_point_set_double_by_name(point, "y", 1.9);
	pc_point_set_double_by_name(point, "Z", 0.34);
	pc_point_set_double_by_name(point, "intensity", 10);
	pc_pointlist_add_point(pointlist, point);

	patch_ght = pc_patch_ght_from_pointlist(pointlist);
	CU_ASSERT(patch_ght->type == PC_GHT);

	patch_lazperf = (PCPATCH_LAZPERF *)pc_patch_compress((PCPATCH *)patch_ght, NULL);
	CU_ASSERT(patch_lazperf != NULL);
	CU_ASSERT(patch_lazperf->type == PC_LAZPERF);

	pc_pointlist_free(pointlist);
	pc_patch_free((PCPATCH *)patch_ght);
	pc_patch_free((PCPATCH *)patch_lazperf);
}
#endif  /* defined(HAVE_LIBGHT) && defined(HAVE_LAZPERF) */

static void
test_patch_pointn_last_first()
{
	// 00 endian (big)
	// 00000000 pcid
	// 00000000 compression
	// 00000003 npoints
	// 0000000800000003000000050006 pt1 (XYZi)
	// 0000000200000003000000040008 pt2 (XYZi)
	// 0000000200000003000000040009 pt3 (XYZi)

	char *hexbuf = "00000000000000000000000003000000080000000300000005000600000002000000030000000400080000000200000003000000040009";
	size_t hexsize = strlen(hexbuf);
	uint8_t *wkb = bytes_from_hexbytes(hexbuf, hexsize);
	char *str;

	PCPATCH *pa = pc_patch_from_wkb(simpleschema, wkb, hexsize/2);

	PCPOINT *pt = pc_patch_pointn(pa, -1);
	str = pc_point_to_string(pt);
	CU_ASSERT_STRING_EQUAL(str, "{\"pcid\":0,\"pt\":[0.02,0.03,0.04,9]}");
	pc_point_free(pt);
	free(str);

	pt = pc_patch_pointn(pa, -3);
	str = pc_point_to_string(pt);
	CU_ASSERT_STRING_EQUAL(str, "{\"pcid\":0,\"pt\":[0.08,0.03,0.05,6]}");
	pc_point_free(pt);
	free(str);

	pc_patch_free(pa);
	pcfree(wkb);
}

static void
test_patch_pointn_no_compression()
{
	// 00 endian (big)
	// 00000000 pcid
	// 00000000 compression
	// 00000003 npoints
	// 0000000800000003000000050006 pt1 (XYZi)
	// 0000000200000003000000040008 pt2 (XYZi)
	// 0000000200000003000000040009 pt3 (XYZi)

	char *hexbuf = "00000000000000000000000003000000080000000300000005000600000002000000030000000400080000000200000003000000040009";
	size_t hexsize = strlen(hexbuf);
	uint8_t *wkb = bytes_from_hexbytes(hexbuf, hexsize);

	PCPATCH *pa = pc_patch_from_wkb(simpleschema, wkb, hexsize/2);
	PCPOINTLIST *li = pc_pointlist_from_patch(pa);

	PCPOINT *pt = pc_patch_pointn(pa, 2);
	CU_ASSERT(pt != NULL);
	char *str = pc_point_to_string(pt);
	CU_ASSERT_STRING_EQUAL(str, "{\"pcid\":0,\"pt\":[0.02,0.03,0.04,8]}");

	// free
	free(str);
	pcfree(wkb);
	pc_point_free(pt);
	pc_patch_free(pa);
	pc_pointlist_free(li);
}

static void
test_patch_pointn_dimensional_compression(enum DIMCOMPRESSIONS dimcomp)
{
	// init data
	PCPATCH_DIMENSIONAL *padim1, *padim2;
	PCPOINT *pt;
	PCPOINTLIST *pl;
	char *str;
	int i;
	int npts = PCDIMSTATS_MIN_SAMPLE+1; // force to keep custom compression

	// build a dimensional patch
	pl = pc_pointlist_make(npts);

	for ( i = npts; i >= 0; i-- )
	{
		pt = pc_point_make(simpleschema);
		pc_point_set_double_by_name(pt, "x", i);
		pc_point_set_double_by_name(pt, "y", i);
		pc_point_set_double_by_name(pt, "Z", i);
		pc_point_set_double_by_name(pt, "intensity", 10);
		pc_pointlist_add_point(pl, pt);
	}

	padim1 = pc_patch_dimensional_from_pointlist(pl);

	// set dimensional compression for each dimension
	PCDIMSTATS *stats = pc_dimstats_make(simpleschema);
	pc_dimstats_update(stats, padim1);
	for ( i = 0; i<padim1->schema->ndims; i++ )
		stats->stats[i].recommended_compression = dimcomp;

	// compress patch
	padim2 = pc_patch_dimensional_compress(padim1, stats);

	pt = pc_patch_pointn((PCPATCH*) padim2, npts-3);
	str = pc_point_to_string(pt);
	CU_ASSERT_STRING_EQUAL(str, "{\"pcid\":0,\"pt\":[4,4,4,10]}");

	free(str);
	pc_dimstats_free(stats);
	pc_pointlist_free(pl);
	pc_point_free(pt);
	pc_patch_free((PCPATCH *)padim1);
	pc_patch_free((PCPATCH *)padim2);
}

static void
test_patch_pointn_dimensional_compression_none()
{
	test_patch_pointn_dimensional_compression(PC_DIM_NONE);
}

static void
test_patch_pointn_dimensional_compression_zlib()
{
	test_patch_pointn_dimensional_compression(PC_DIM_ZLIB);
}

static void
test_patch_pointn_dimensional_compression_sigbits()
{
	test_patch_pointn_dimensional_compression(PC_DIM_SIGBITS);
}

static void
test_patch_pointn_dimensional_compression_rle()
{
	test_patch_pointn_dimensional_compression(PC_DIM_RLE);
}

static void
test_patch_pointn_ght_compression()
{
	// 00 endian (big)
	// 00000000 pcid
	// 00000000 compression
	// 00000003 npoints
	// 0000000800000003000000050006 pt1 (XYZi)
	// 0000000200000003000000040008 pt2 (XYZi)
	// 0000000200000003000000040009 pt3 (XYZi)

	char *hexbuf = "00000000000000000000000003000000080000000300000005000600000002000000030000000400080000000200000003000000040009";
	size_t hexsize = strlen(hexbuf);
	uint8_t *wkb = bytes_from_hexbytes(hexbuf, hexsize);

	PCPATCH *pa = pc_patch_from_wkb(simpleschema, wkb, hexsize/2);
	PCPOINTLIST *li = pc_pointlist_from_patch(pa);

#ifdef HAVE_LIBGHT
	PCPATCH_GHT* pag = pc_patch_ght_from_pointlist(li);
	PCPOINT *pt = pc_patch_pointn((PCPATCH*) pag, 2);
	CU_ASSERT(pt != NULL);
	CU_ASSERT_STRING_EQUAL(pc_point_to_string(pt), "{\"pcid\":0,\"pt\":[0.02,0.03,0.04,8]}");
	pc_patch_free( (PCPATCH*) pag );
	pc_point_free(pt);
#endif

	// free
	pcfree(wkb);
	pc_patch_free(pa);
	pc_pointlist_free(li);
}

/* REGISTER ***********************************************************/

CU_TestInfo patch_tests[] = {
	PC_TEST(test_endian_flip),
	PC_TEST(test_patch_hex_in),
	PC_TEST(test_patch_hex_out),
	PC_TEST(test_schema_xy),
	PC_TEST(test_patch_dimensional),
	PC_TEST(test_patch_dimensional_compression),
	PC_TEST(test_patch_dimensional_extent),
	PC_TEST(test_patch_union),
	PC_TEST(test_patch_wkb),
	PC_TEST(test_patch_filter),
#if defined(HAVE_LIBGHT) && defined(HAVE_LAZPERF)
	PC_TEST(test_patch_compress_from_ght_to_lazperf),
#endif
	PC_TEST(test_patch_pointn_last_first),
	PC_TEST(test_patch_pointn_no_compression),
	PC_TEST(test_patch_pointn_dimensional_compression_none),
	PC_TEST(test_patch_pointn_dimensional_compression_zlib),
	PC_TEST(test_patch_pointn_dimensional_compression_sigbits),
	PC_TEST(test_patch_pointn_dimensional_compression_rle),
	PC_TEST(test_patch_pointn_ght_compression),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo patch_suite = {
	.pName = "patch",
	.pInitFunc = init_suite,
	.pCleanupFunc = clean_suite,
	.pTests = patch_tests
};
