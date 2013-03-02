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
static const char *xmlfile = "data/pdal-schema.xml";

/* Setup/teardown for this suite */
static int 
init_suite(void)
{
    char *xmlstr = file_to_str(xmlfile);
	int rv = pc_schema_from_xml(xmlstr, &schema);
	pcfree(xmlstr);
	return rv == PC_FAILURE;
}

static int 
clean_suite(void)
{
	pc_schema_free(schema);
	return 0;
}


/* TESTS **************************************************************/

static void 
test_schema_from_xml() 
{
    static PCSCHEMA *myschema = NULL;
	char *xmlstr = file_to_str(xmlfile);
	int rv = pc_schema_from_xml(xmlstr, &myschema);
	pcfree(xmlstr);

	// char *schemastr = pc_schema_to_json(schema);
	// printf("ndims %d\n", schema->ndims);
	// printf("name0 %s\n", schema->dims[0]->name);
	// printf("%s\n", schemastr);
	
	CU_ASSERT(myschema != NULL);
    pc_schema_free(myschema);
}

static void 
test_schema_size() 
{
	size_t sz = schema->size;
	CU_ASSERT_EQUAL(sz, 37);
}

static void 
test_dimension_get() 
{
	PCDIMENSION *d;
	
	d = pc_schema_get_dimension(schema, 0);
	CU_ASSERT_EQUAL(d->position, 0);
	CU_ASSERT_STRING_EQUAL(d->name, "X");

	d = pc_schema_get_dimension(schema, 1);
	CU_ASSERT_EQUAL(d->position, 1);
	CU_ASSERT_STRING_EQUAL(d->name, "Y");

	d = pc_schema_get_dimension_by_name(schema, "nothinghere");
	CU_ASSERT_EQUAL(d, NULL);

	d = pc_schema_get_dimension_by_name(schema, "Z");
	CU_ASSERT_EQUAL(d->position, 2);
	CU_ASSERT_STRING_EQUAL(d->name, "Z");

	d = pc_schema_get_dimension_by_name(schema, "z");
	CU_ASSERT_EQUAL(d->position, 2);
	CU_ASSERT_STRING_EQUAL(d->name, "Z");

	d = pc_schema_get_dimension_by_name(schema, "y");
	// printf("name %s\n", d->name);
	// printf("position %d\n", d->position);
	CU_ASSERT_EQUAL(d->position, 1);
	CU_ASSERT_STRING_EQUAL(d->name, "Y");
}

static void 
test_dimension_byteoffsets() 
{
	PCDIMENSION *d;
	int i;
	int prev_byteoffset;
	int prev_size;
	int pc_size;
	
	for ( i = 0; i < schema->ndims; i++ )
	{
		d = pc_schema_get_dimension(schema, i);
		// printf("d=%d name='%s' size=%d byteoffset=%d\n", i, d->name, d->size, d->byteoffset);
		if ( i > 0 )
		{
			CU_ASSERT_EQUAL(prev_size, pc_size);
			CU_ASSERT_EQUAL(prev_size, d->byteoffset - prev_byteoffset);
		}
		prev_byteoffset = d->byteoffset;
		prev_size = d->size;
		pc_size = pc_interpretation_size(d->interpretation);
	}
	
}

static void 
test_point_access() 
{
	PCPOINT *pt;
	int rv;
	double a1, a2, a3, a4, b1, b2, b3, b4;
	int idx = 0;
	
	pt = pc_point_make(schema);
	CU_ASSERT( pt != NULL );

	/* One at a time */
	idx = 0;
	a1 = 1.5;
	rv = pc_point_set_double_by_index(pt, idx, a1);
	rv = pc_point_get_double_by_index(pt, idx, &b1);
	// printf("d1=%g, d2=%g\n", a1, b1);
	CU_ASSERT_DOUBLE_EQUAL(a1, b1, 0.0000001);

	idx = 2;
	a2 = 1501500.12;
	rv = pc_point_set_double_by_index(pt, idx, a2);
	rv = pc_point_get_double_by_index(pt, idx, &b2);
	CU_ASSERT_DOUBLE_EQUAL(a2, b2, 0.0000001);

	a3 = 91;
	rv = pc_point_set_double_by_name(pt, "NumberOfReturns", a3);
	rv = pc_point_get_double_by_name(pt, "NumberOfReturns", &b3);
	CU_ASSERT_DOUBLE_EQUAL(a3, b3, 0.0000001);
	
	pc_point_free(pt);
	
	/* All at once */
	pt = pc_point_make(schema);
	a1 = 1.5;
	a2 = 1501500.12;
	a3 = 91;
	a4 = 200;
	rv = pc_point_set_double_by_index(pt, 0, a1);
	rv = pc_point_set_double_by_index(pt, 2, a2);
	rv = pc_point_set_double_by_name(pt, "NumberOfReturns", a3);
	rv = pc_point_set_double_by_name(pt, "UserData", a4);
	rv = pc_point_get_double_by_index(pt, 0, &b1);
	rv = pc_point_get_double_by_index(pt, 2, &b2);
	rv = pc_point_get_double_by_name(pt, "NumberOfReturns", &b3);
	rv = pc_point_get_double_by_name(pt, "UserData", &b4);
	CU_ASSERT_DOUBLE_EQUAL(a1, b1, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a2, b2, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a3, b3, 0.0000001);
	CU_ASSERT_DOUBLE_EQUAL(a4, b4, 0.0000001);
	
	pc_point_free(pt);
	
}

/* REGISTER ***********************************************************/

CU_TestInfo schema_tests[] = {
	PC_TEST(test_schema_from_xml),
	PC_TEST(test_schema_size),
	PC_TEST(test_dimension_get),
	PC_TEST(test_dimension_byteoffsets),
	PC_TEST(test_point_access),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo schema_suite = {"schema", init_suite, clean_suite, schema_tests};
