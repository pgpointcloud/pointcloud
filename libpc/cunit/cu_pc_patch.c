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

static char* file_to_str(const char *fname);

/* GLOBALS ************************************************************/

static PCSCHEMA *schema = NULL;
const char *xmlfile = "data/pdal-schema.xml";

/* Setup/teardown for this suite */
static int 
init_suite(void)
{
	char *xmlstr = file_to_str(xmlfile);
	schema = pc_schema_from_xml(xmlstr);
	pcfree(xmlstr);
	return 0;
}

static int 
clean_suite(void)
{
	pc_schema_free(schema);
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
	fclose(fr);
	return str;
}

/* TESTS **************************************************************/

static void 
test_endian_flip() 
{
	size_t sz = schema->size;
	CU_ASSERT_EQUAL(sz, 37);
}





/* REGISTER ***********************************************************/

CU_TestInfo patch_tests[] = {
	PG_TEST(test_endian_flip),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo patch_suite = {"patch", init_suite, clean_suite, patch_tests};
