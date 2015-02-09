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
	return rv == PC_FAILURE ? -1 : 0;
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
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);
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
test_schema_is_valid()
{
	static PCSCHEMA *myschema = NULL;
	char *xmlstr;
	int rv;

	// See https://github.com/pgpointcloud/pointcloud/issues/28
	xmlstr = "<pc:PointCloudSchema xmlns:pc='x'><pc:dimension>1</pc:dimension></pc:PointCloudSchema>";
	rv = pc_schema_from_xml(xmlstr, &myschema);
	CU_ASSERT_EQUAL(rv, PC_SUCCESS);

  cu_error_msg_reset();
  rv = pc_schema_is_valid(myschema);
	CU_ASSERT_EQUAL(rv, PC_FAILURE);
}


static void
test_schema_compression(void)
{
    int compression = schema->compression;
    CU_ASSERT_EQUAL(compression, PC_DIMENSIONAL);
}

/* REGISTER ***********************************************************/

CU_TestInfo schema_tests[] = {
	PC_TEST(test_schema_from_xml),
	PC_TEST(test_schema_size),
	PC_TEST(test_dimension_get),
	PC_TEST(test_dimension_byteoffsets),
	PC_TEST(test_schema_compression),
	PC_TEST(test_schema_is_valid),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo schema_suite = {"schema", init_suite, clean_suite, schema_tests};
