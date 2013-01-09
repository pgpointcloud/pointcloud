#include "CUnit/Basic.h"
#include "cu_tester.h"

static void test_schema() {
        CU_ASSERT(1);
}


/* register tests */
CU_TestInfo schema[] = {
        PG_TEST(test_schema),
        CU_TEST_INFO_NULL
};
CU_SuiteInfo schema_suite = {"schema",  NULL,  NULL, schema};
