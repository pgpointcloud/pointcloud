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
test_schema_from_xml_with_empty_description()
{
    PCSCHEMA *myschema = NULL;
    char *myxmlfile = "data/simple-schema-empty-description.xml";
    char *xmlstr = file_to_str(myxmlfile);
    int rv = pc_schema_from_xml(xmlstr, &myschema);

    CU_ASSERT(rv == PC_SUCCESS);

    pc_schema_free(myschema);
    pcfree(xmlstr);
}

static void
test_schema_from_xml_with_no_name()
{
    PCSCHEMA *myschema = NULL;
    char *myxmlfile = "data/simple-schema-no-name.xml";
    char *xmlstr = file_to_str(myxmlfile);
    int rv = pc_schema_from_xml(xmlstr, &myschema);

    CU_ASSERT(rv == PC_SUCCESS);

    pc_schema_free(myschema);
    pcfree(xmlstr);
}

static void
test_schema_from_xml_with_empty_name()
{
    PCSCHEMA *myschema = NULL;
    char *myxmlfile = "data/simple-schema-empty-name.xml";
    char *xmlstr = file_to_str(myxmlfile);
    int rv = pc_schema_from_xml(xmlstr, &myschema);

    CU_ASSERT(rv == PC_SUCCESS);

    pc_schema_free(myschema);
    pcfree(xmlstr);
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
test_schema_invalid_xy()
{
	static PCSCHEMA *myschema = NULL;
	char *xmlstr;
	int rv;

	xmlstr = "<pc:PointCloudSchema xmlns:pc='x'><pc:dimension>1</pc:dimension></pc:PointCloudSchema>";
	rv = pc_schema_from_xml(xmlstr, &myschema);
	CU_ASSERT_EQUAL(rv, PC_FAILURE);
	CU_ASSERT_EQUAL(myschema, NULL);
}


static void
test_schema_missing_dimension()
{
    static PCSCHEMA *myschema = NULL;
    char *myxmlfile = "data/simple-schema-missing-dimension.xml";
    char *xmlstr = file_to_str(myxmlfile);
    int rv = pc_schema_from_xml(xmlstr, &myschema);
    cu_error_msg_reset();
    CU_ASSERT_EQUAL(rv, PC_FAILURE);
    CU_ASSERT_EQUAL(myschema, NULL);

    pcfree(xmlstr);
}


static void
test_schema_empty()
{
	static PCSCHEMA *myschema = NULL;
	char *xmlstr;
	int rv;

	xmlstr = "";
	rv = pc_schema_from_xml(xmlstr, &myschema);
	CU_ASSERT_EQUAL(rv, PC_FAILURE);
	CU_ASSERT_EQUAL(myschema, NULL);	
}

static void
test_schema_compression(void)
{
    int compression = schema->compression;
    CU_ASSERT_EQUAL(compression, PC_DIMENSIONAL);
}

static void
test_schema_clone(void)
{
    int i;
    PCSCHEMA *clone = pc_schema_clone(schema);
    hashtable *hash, *chash;
    char *xmlstr;
    CU_ASSERT_EQUAL(clone->pcid, schema->pcid);
    CU_ASSERT_EQUAL(clone->ndims, schema->ndims);
    CU_ASSERT_EQUAL(clone->size, schema->size);
    CU_ASSERT_EQUAL(clone->srid, schema->srid);
    CU_ASSERT_EQUAL(clone->x_position, schema->x_position);
    CU_ASSERT_EQUAL(clone->y_position, schema->y_position);
    CU_ASSERT_EQUAL(clone->compression, schema->compression);
    CU_ASSERT(clone->dims != schema->dims); /* deep clone */
    CU_ASSERT(clone->namehash != schema->namehash); /* deep clone */
    hash = schema->namehash;
    chash = clone->namehash;
    CU_ASSERT_EQUAL(chash->tablelength, hash->tablelength);
    CU_ASSERT_EQUAL(chash->entrycount, hash->entrycount);
    CU_ASSERT_EQUAL(chash->loadlimit, hash->loadlimit);
    CU_ASSERT_EQUAL(chash->primeindex, hash->primeindex);
    CU_ASSERT_EQUAL(chash->hashfn, hash->hashfn);
    CU_ASSERT_EQUAL(chash->eqfn, hash->eqfn);
    CU_ASSERT(chash->table != hash->table); /* deep clone */
    for (i=0; i<schema->ndims; ++i) {
      PCDIMENSION *dim = schema->dims[i];
      PCDIMENSION *cdim = clone->dims[i];
      CU_ASSERT(dim != cdim); /* deep clone */
      CU_ASSERT_STRING_EQUAL(cdim->name, dim->name);
      CU_ASSERT_STRING_EQUAL(cdim->description, dim->description);
      CU_ASSERT_EQUAL(cdim->position, dim->position);
      CU_ASSERT_EQUAL(cdim->size, dim->size);
      CU_ASSERT_EQUAL(cdim->byteoffset, dim->byteoffset);
      CU_ASSERT_EQUAL(cdim->interpretation, dim->interpretation);
      CU_ASSERT_EQUAL(cdim->scale, dim->scale);
      CU_ASSERT_EQUAL(cdim->offset, dim->offset);
      CU_ASSERT_EQUAL(cdim->active, dim->active);
      /* hash table is correctly setup */
      CU_ASSERT_EQUAL(cdim, hashtable_search(clone->namehash, dim->name) );
    }

    pc_schema_free(clone);
}

static void
test_schema_clone_empty_description(void)
{
    int i;
    PCSCHEMA *myschema, *clone;

    /* See https://github.com/pgpointcloud/pointcloud/issues/66 */
    char *myxmlfile = "data/simple-schema-empty-description.xml";
    char *xmlstr = file_to_str(myxmlfile);

    i = pc_schema_from_xml(xmlstr, &myschema);
	CU_ASSERT_EQUAL(i, PC_SUCCESS);
    clone = pc_schema_clone(myschema);
    CU_ASSERT_EQUAL(clone->ndims, myschema->ndims);
    CU_ASSERT_NOT_EQUAL(clone->dims[0]->name, myschema->dims[0]->name);
    CU_ASSERT_STRING_EQUAL(clone->dims[0]->name, myschema->dims[0]->name);
    CU_ASSERT_EQUAL(clone->dims[0]->description, myschema->dims[0]->description);
    pc_schema_free(myschema);
    pc_schema_free(clone);
    pcfree(xmlstr);	
}

static void
test_schema_clone_no_name(void)
{
    int i;
    PCSCHEMA *myschema, *clone;

    /* See https://github.com/pgpointcloud/pointcloud/issues/66 */
    char *myxmlfile = "data/simple-schema-no-name.xml";
    char *xmlstr = file_to_str(myxmlfile);

    i = pc_schema_from_xml(xmlstr, &myschema);
	CU_ASSERT_EQUAL(i, PC_SUCCESS);
    clone = pc_schema_clone(myschema);
    CU_ASSERT_EQUAL(clone->ndims, myschema->ndims);
    CU_ASSERT_EQUAL(clone->dims[0]->name, myschema->dims[0]->name);
    CU_ASSERT_EQUAL(clone->dims[0]->description, myschema->dims[0]->description);
    pc_schema_free(myschema);
    pc_schema_free(clone);
    pcfree(xmlstr);	
}

static void
test_schema_clone_empty_name(void)
{
    int i;
    PCSCHEMA *myschema, *clone;

    char *myxmlfile = "data/simple-schema-empty-name.xml";
    char *xmlstr = file_to_str(myxmlfile);

    i = pc_schema_from_xml(xmlstr, &myschema);
	CU_ASSERT_EQUAL(i, PC_SUCCESS);
    clone = pc_schema_clone(myschema);
    CU_ASSERT_EQUAL(clone->ndims, myschema->ndims);
    CU_ASSERT_EQUAL(clone->dims[0]->name, myschema->dims[0]->name);
    CU_ASSERT_EQUAL(clone->dims[0]->description, myschema->dims[0]->description);
    pc_schema_free(myschema);
    pc_schema_free(clone);
    pcfree(xmlstr);	
}

/* REGISTER ***********************************************************/

CU_TestInfo schema_tests[] = {
	PC_TEST(test_schema_from_xml),
	PC_TEST(test_schema_from_xml_with_empty_description),
	PC_TEST(test_schema_from_xml_with_empty_name),
	PC_TEST(test_schema_from_xml_with_no_name),
	PC_TEST(test_schema_size),
	PC_TEST(test_dimension_get),
	PC_TEST(test_dimension_byteoffsets),
	PC_TEST(test_schema_compression),
	PC_TEST(test_schema_invalid_xy),
	PC_TEST(test_schema_missing_dimension),
	PC_TEST(test_schema_empty),
	PC_TEST(test_schema_clone),
	PC_TEST(test_schema_clone_empty_description),
	PC_TEST(test_schema_clone_no_name),
	PC_TEST(test_schema_clone_empty_name),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo schema_suite = {
    .pName = "schema",
    .pInitFunc = init_suite,
    .pCleanupFunc = clean_suite,
    .pTests = schema_tests
};
