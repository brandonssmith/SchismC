/*
 * Comprehensive Testing Framework for SchismC
 * Automated test discovery, execution, and reporting
 */

#ifndef TESTING_H
#define TESTING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "core_structures.h"
#include "debug.h"

/* Test Result Types */
typedef enum {
    TEST_PASS = 0,
    TEST_FAIL = 1,
    TEST_SKIP = 2,
    TEST_ERROR = 3
} TestResult;

/* Test Categories */
typedef enum {
    TEST_CAT_UNIT = 0,
    TEST_CAT_INTEGRATION = 1,
    TEST_CAT_PERFORMANCE = 2,
    TEST_CAT_REGRESSION = 3,
    TEST_CAT_COMPILER = 4,
    TEST_CAT_RUNTIME = 5,
    TEST_CAT_MAX
} TestCategory;

/* Test Status */
typedef enum {
    TEST_STATUS_PENDING = 0,
    TEST_STATUS_RUNNING = 1,
    TEST_STATUS_COMPLETED = 2,
    TEST_STATUS_SKIPPED = 3,
    TEST_STATUS_ERROR = 4
} TestStatus;

/* Individual Test Structure */
typedef struct TestCase {
    char *name;                    /* Test name */
    char *description;             /* Test description */
    TestCategory category;         /* Test category */
    TestStatus status;             /* Current status */
    TestResult result;             /* Test result */
    double execution_time;         /* Execution time in seconds */
    char *error_message;           /* Error message if failed */
    char *file;                    /* Source file */
    int line;                      /* Source line */
    void (*test_function)(void);   /* Test function pointer */
    struct TestCase *next;         /* Linked list */
} TestCase;

/* Test Suite Structure */
typedef struct TestSuite {
    char *name;                    /* Suite name */
    char *description;             /* Suite description */
    TestCase *tests;               /* List of tests */
    int test_count;                /* Total test count */
    int passed_count;              /* Passed test count */
    int failed_count;              /* Failed test count */
    int skipped_count;             /* Skipped test count */
    int error_count;               /* Error test count */
    double total_time;             /* Total execution time */
    struct TestSuite *next;        /* Linked list */
} TestSuite;

/* Test Runner Context */
typedef struct TestRunner {
    TestSuite *suites;             /* List of test suites */
    int suite_count;               /* Total suite count */
    int total_tests;               /* Total test count across all suites */
    int total_passed;              /* Total passed tests */
    int total_failed;              /* Total failed tests */
    int total_skipped;             /* Total skipped tests */
    int total_errors;              /* Total error tests */
    double total_execution_time;   /* Total execution time */
    Bool verbose_output;           /* Verbose test output */
    Bool stop_on_failure;          /* Stop on first failure */
    Bool color_output;             /* Colored output */
    char *output_file;             /* Output file for results */
    FILE *output_fp;               /* Output file pointer */
    DebugContext *debug_ctx;       /* Debug context */
} TestRunner;

/* Global test runner instance */
extern TestRunner *g_test_runner;

/* Test Runner Management */
TestRunner* test_runner_new(void);
void test_runner_free(TestRunner *runner);
void test_runner_set_verbose(TestRunner *runner, Bool verbose);
void test_runner_set_stop_on_failure(TestRunner *runner, Bool stop);
void test_runner_set_color_output(TestRunner *runner, Bool color);
void test_runner_set_output_file(TestRunner *runner, const char *filename);

/* Test Suite Management */
TestSuite* test_suite_new(const char *name, const char *description);
void test_suite_free(TestSuite *suite);
void test_runner_add_suite(TestRunner *runner, TestSuite *suite);

/* Test Case Management */
TestCase* test_case_new(const char *name, const char *description, 
                       TestCategory category, void (*test_func)(void));
void test_case_free(TestCase *test);
void test_suite_add_test(TestSuite *suite, TestCase *test);

/* Test Execution */
Bool test_runner_run_all(TestRunner *runner);
Bool test_runner_run_suite(TestRunner *runner, const char *suite_name);
Bool test_runner_run_test(TestRunner *runner, const char *suite_name, const char *test_name);
Bool test_runner_run_category(TestRunner *runner, TestCategory category);

/* Test Discovery */
void test_discover_tests(TestRunner *runner, const char *test_directory);
void test_discover_holyc_tests(TestRunner *runner, const char *test_directory);

/* Assertion Macros */
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            test_fail("Assertion failed: " #condition " is not true"); \
            return; \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            test_fail("Assertion failed: " #condition " is not false"); \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            test_fail("Assertion failed: expected %lld, got %lld", (long long)(expected), (long long)(actual)); \
            return; \
        } \
    } while(0)

#define ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            test_fail("Assertion failed: expected not %lld, got %lld", (long long)(expected), (long long)(actual)); \
            return; \
        } \
    } while(0)

#define ASSERT_STR_EQ(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            test_fail("Assertion failed: expected \"%s\", got \"%s\"", (expected), (actual)); \
            return; \
        } \
    } while(0)

#define ASSERT_STR_NE(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) == 0) { \
            test_fail("Assertion failed: expected not \"%s\", got \"%s\"", (expected), (actual)); \
            return; \
        } \
    } while(0)

#define ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            test_fail("Assertion failed: expected NULL, got %p", (ptr)); \
            return; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            test_fail("Assertion failed: expected non-NULL, got NULL"); \
            return; \
        } \
    } while(0)

/* Test Result Functions */
void test_pass(const char *message, ...);
void test_fail(const char *message, ...);
void test_skip(const char *message, ...);
void test_error(const char *message, ...);

/* Test Registration Macros */
#define TEST_SUITE(name, description) \
    static TestSuite *__test_suite_##name = NULL; \
    static void __test_suite_init_##name(void) { \
        __test_suite_##name = test_suite_new(#name, description); \
        test_runner_add_suite(g_test_runner, __test_suite_##name); \
    }

#define TEST_CASE(suite_name, test_name, description, category) \
    static void __test_##suite_name##_##test_name(void); \
    static void __test_register_##suite_name##_##test_name(void) { \
        if (!__test_suite_##suite_name) { \
            __test_suite_init_##suite_name(); \
        } \
        TestCase *test = test_case_new(#test_name, description, category, __test_##suite_name##_##test_name); \
        test_suite_add_test(__test_suite_##suite_name, test); \
    } \
    static void __test_##suite_name##_##test_name(void)

/* Test Registration Functions */
void test_register_all_tests(void);
void test_register_compiler_tests(void);
void test_register_runtime_tests(void);
void test_register_integration_tests(void);

/* Test Reporting */
void test_runner_print_summary(TestRunner *runner);
void test_runner_print_detailed_report(TestRunner *runner);
void test_runner_export_junit_xml(TestRunner *runner, const char *filename);
void test_runner_export_json(TestRunner *runner, const char *filename);

/* Test Utilities */
void test_setup(void);
void test_teardown(void);
void test_cleanup(void);

/* Performance Testing */
typedef struct {
    const char *name;
    double min_time;
    double max_time;
    double avg_time;
    int iterations;
} PerformanceTest;

void test_performance_start(const char *test_name);
void test_performance_end(const char *test_name);
void test_performance_assert(const char *test_name, double max_time);

/* Mock and Fixture Support */
typedef struct {
    char *name;
    void *data;
    void (*setup)(void *data);
    void (*teardown)(void *data);
} TestFixture;

void test_fixture_register(const char *name, void *data, 
                          void (*setup)(void *data), void (*teardown)(void *data));
void test_fixture_setup(const char *name);
void test_fixture_teardown(const char *name);

/* Test Initialization and Cleanup */
void test_framework_init(void);
void test_framework_cleanup(void);

/* Command Line Test Runner */
int test_main(int argc, char *argv[]);

#endif /* TESTING_H */
