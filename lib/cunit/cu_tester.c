/***********************************************************************
* cu_tester.c
*
*        Testing harness for PgSQL PointClouds
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include <stdio.h>
#include <string.h>
#include "CUnit/Basic.h"
#include "cu_tester.h"

/* ADD YOUR SUITE HERE (1 of 2) */
extern CU_SuiteInfo schema_suite;
extern CU_SuiteInfo patch_suite;
extern CU_SuiteInfo point_suite;
extern CU_SuiteInfo ght_suite;
extern CU_SuiteInfo bytes_suite;
extern CU_SuiteInfo lazperf_suite;
extern CU_SuiteInfo sort_suite;
extern CU_SuiteInfo util_suite;

/**
* CUnit error handler
* Log message in a global var instead of printing in stderr
*
* CAUTION: Not stop execution on pcerror case !!!
*/
static void cu_error_reporter(const char *fmt, va_list ap)
{
	vsnprintf(cu_error_msg, MAX_CUNIT_MSG_LENGTH - 1, fmt, ap);
	cu_error_msg[MAX_CUNIT_MSG_LENGTH - 1] = '\0';
	va_end(ap);
}

void cu_error_msg_reset()
{
	memset(cu_error_msg, '\0', MAX_CUNIT_MSG_LENGTH);
}


/*
** The main() function for setting up and running the tests.
** Returns a CUE_SUCCESS on successful running, another
** CUnit error code on failure.
*/
int main(int argc, char *argv[])
{
	/* ADD YOUR SUITE HERE (2 of 2) */
	CU_SuiteInfo suites[] =
	{
		schema_suite,
		patch_suite,
		point_suite,
		ght_suite,
		bytes_suite,
		lazperf_suite,
		sort_suite,
		util_suite,
		CU_SUITE_INFO_NULL
	};

	int index;
	char *suite_name;
	CU_pSuite suite_to_run;
	char *test_name;
	CU_pTest test_to_run;
	CU_ErrorCode errCode = 0;
	CU_pTestRegistry registry;
	int num_run;
	int num_failed;

	/* Set up to use the system memory management / logging */
	pc_install_default_handlers();

	pc_set_handlers(0, 0, 0, cu_error_reporter, 0, 0);

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
	{
		errCode = CU_get_error();
		printf("    Error attempting to initialize registry: %d.  See CUError.h for error code list.\n", errCode);
		return errCode;
	}

	/* Register all the test suites. */
	if (CUE_SUCCESS != CU_register_suites(suites))
	{
		errCode = CU_get_error();
		printf("    Error attempting to register test suites: %d.  See CUError.h for error code list.\n", errCode);
		return errCode;
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	if (argc <= 1)
	{
		errCode = CU_basic_run_tests();
	}
	else
	{
		/* NOTE: The cunit functions used here (CU_get_registry, CU_get_suite_by_name, and CU_get_test_by_name) are
		 *       listed with the following warning: "Internal CUnit system functions.  Should not be routinely called by users."
		 *       However, there didn't seem to be any other way to get tests by name, so we're calling them. */
		registry = CU_get_registry();
		for (index = 1; index < argc; index++)
		{
			suite_name = argv[index];
			test_name = NULL;
			suite_to_run = CU_get_suite_by_name(suite_name, registry);
			if (NULL == suite_to_run)
			{
				/* See if it's a test name instead of a suite name. */
				suite_to_run = registry->pSuite;
				while (suite_to_run != NULL)
				{
					test_to_run = CU_get_test_by_name(suite_name, suite_to_run);
					if (test_to_run != NULL)
					{
						/* It was a test name. */
						test_name = suite_name;
						suite_name = suite_to_run->pName;
						break;
					}
					suite_to_run = suite_to_run->pNext;
				}
			}
			if (suite_to_run == NULL)
			{
				printf("\n'%s' does not appear to be either a suite name or a test name.\n\n", suite_name);
			}
			else
			{
				if (test_name != NULL)
				{
					/* Run only this test. */
					printf("\nRunning test '%s' in suite '%s'.\n", test_name, suite_name);
					/* This should be CU_basic_run_test, but that method is broken, see:
					 *     https://sourceforge.net/tracker/?func=detail&aid=2851925&group_id=32992&atid=407088
					 * This one doesn't output anything for success, so we have to do it manually. */
					errCode = CU_run_test(suite_to_run, test_to_run);
					if (errCode != CUE_SUCCESS)
					{
						printf("    Error attempting to run tests: %d.  See CUError.h for error code list.\n", errCode);
					}
					else
					{
						num_run = CU_get_number_of_asserts();
						num_failed = CU_get_number_of_failures();
						printf("\n    %s - asserts - %3d passed, %3d failed, %3d total.\n\n",
							   (0 == num_failed ? "PASSED" : "FAILED"), (num_run - num_failed), num_failed, num_run);
					}
				}
				else
				{
					/* Run all the tests in the suite. */
					printf("\nRunning all tests in suite '%s'.\n", suite_name);
					/* This should be CU_basic_run_suite, but that method is broken, see:
					 *     https://sourceforge.net/tracker/?func=detail&aid=2851925&group_id=32992&atid=407088
					 * This one doesn't output anything for success, so we have to do it manually. */
					errCode = CU_run_suite(suite_to_run);
					if (errCode != CUE_SUCCESS)
					{
						printf("    Error attempting to run tests: %d.  See CUError.h for error code list.\n", errCode);
					}
					else
					{
						num_run = CU_get_number_of_tests_run();
						num_failed = CU_get_number_of_tests_failed();
						printf("\n    %s -   tests - %3d passed, %3d failed, %3d total.\n",
							   (0 == num_failed ? "PASSED" : "FAILED"), (num_run - num_failed), num_failed, num_run);
						num_run = CU_get_number_of_asserts();
						num_failed = CU_get_number_of_failures();
						printf("           - asserts - %3d passed, %3d failed, %3d total.\n\n",
							   (num_run - num_failed), num_failed, num_run);
					}
				}
			}
		}
		/* Presumably if the CU_basic_run_[test|suite] functions worked, we wouldn't have to do this. */
		CU_basic_show_failures(CU_get_failure_list());
		printf("\n\n"); /* basic_show_failures leaves off line breaks. */
	}
	num_failed = CU_get_number_of_failures();
	CU_cleanup_registry();
	return num_failed;
}

/* UTILITY ************************************************************/

char*
file_to_str(const char *fname)
{
	FILE *fr;
	char fullpath[512];
	size_t lnsz;
	size_t sz = 8192;
	char *str = pcalloc(sz);
	char *ptr = str;
	size_t MAXLINELEN = 8192;
	char buf[MAXLINELEN];

	snprintf(fullpath, 512, "%s/lib/cunit/%s", PROJECT_SOURCE_DIR, fname);
	fr = fopen(fullpath, "rt");

	while (fr && fgets(buf, MAXLINELEN, fr) != NULL)
	{
		if (buf[0] == '\0')
			continue;
		lnsz = strlen(buf);
		if (ptr - str + lnsz > sz)
		{
			size_t bsz = ptr - str;
			sz *= 2;
			str = pcrealloc(str, sz);
			ptr = str + bsz;
		}
		memcpy(ptr, buf, lnsz);
		ptr += lnsz;
	}

	*ptr = '\0';
	fclose(fr);

	return str;
}
