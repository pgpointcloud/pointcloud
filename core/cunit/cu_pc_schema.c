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

PCSCHEMA *pcs = NULL;
const char *xmlfile = "data/pdal-schema.xml";

/* Setup/teardown for this suite */
static int 
init_schema_suite(void)
{
	pcs = NULL;
	return 0;
}

static int 
clean_schema_suite(void)
{
	pc_schema_free(pcs);
	return 0;
}

/* UTILITY ************************************************************/

static char*
file_to_str(const char *fname)
{
	FILE *fr;
	size_t lnsz;
	size_t sz = 8192;
	char *str = pcalloc(sz);
	char *ptr = str;
	char *ln;
	
	fr = fopen (fname, "rt");
	while( ln = fgetln(fr, &lnsz) )
	{
		if ( ptr - str + lnsz > sz )
		{
			size_t bsz = ptr - str;
			sz *= 2;
			str = pcrealloc(str, sz);
			ptr = str + bsz;
		}
		memcpy(ptr, ln, lnsz);
		ptr += lnsz;		
	}
	
	return str;
}

/* TESTS **************************************************************/

static void 
test_schema_from_xml() 
{
	char *xmlstr = file_to_str(xmlfile);
	pcs = pc_schema_construct_from_xml(xmlstr);
	pcfree(xmlstr);

	// char *pcsstr = pc_schema_to_json(pcs);
	// printf("ndims %d\n", pcs->ndims);
	// printf("name0 %s\n", pcs->dims[0]->name);
	// printf("%s\n", pcsstr);
	
	CU_ASSERT(pcs != NULL);
}

static void 
test_schema_size() 
{
	size_t sz = pc_schema_get_size(pcs);
	CU_ASSERT_EQUAL(sz, 37);
}

static void 
test_dimension_get() 
{
	PCDIMENSION *d;
	
	d = pc_schema_get_dimension(pcs, 0);
	CU_ASSERT_EQUAL(d->position, 0);
	CU_ASSERT_STRING_EQUAL(d->name, "X");

	d = pc_schema_get_dimension(pcs, 1);
	CU_ASSERT_EQUAL(d->position, 1);
	CU_ASSERT_STRING_EQUAL(d->name, "Y");

	d = pc_schema_get_dimension_by_name(pcs, "nothinghere");
	CU_ASSERT_EQUAL(d, NULL);

	d = pc_schema_get_dimension_by_name(pcs, "Z");
	CU_ASSERT_EQUAL(d->position, 2);
	CU_ASSERT_STRING_EQUAL(d->name, "Z");

	d = pc_schema_get_dimension_by_name(pcs, "z");
	CU_ASSERT_EQUAL(d->position, 2);
	CU_ASSERT_STRING_EQUAL(d->name, "Z");

	d = pc_schema_get_dimension_by_name(pcs, "y");
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
	
	for ( i = 0; i < pcs->ndims; i++ )
	{
		d = pc_schema_get_dimension(pcs, i);
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

/* REGISTER ***********************************************************/

CU_TestInfo schema[] = {
	PG_TEST(test_schema_from_xml),
	PG_TEST(test_schema_size),
	PG_TEST(test_dimension_get),
	PG_TEST(test_dimension_byteoffsets),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo schema_suite = {"schema", init_schema_suite, clean_schema_suite, schema};
