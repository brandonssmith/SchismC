/*
 * Comprehensive Testing Framework Implementation for SchismC
 * Automated test discovery, execution, and reporting
 */

#include "testing.h"
#include <sys/stat.h>
#include <dirent.h>


/* Global test runner instance */
TestRunner *g_test_runner = NULL;

/* Color codes for test output */
#define TEST_COLOR_RESET   "\033[0m"
#define TEST_COLOR_RED     "\033[31m"
#define TEST_COLOR_GREEN   "\033[32m"
#define TEST_COLOR_YELLOW  "\033[33m"
#define TEST_COLOR_BLUE    "\033[34m"
#define TEST_COLOR_MAGENTA "\033[35m"
#define TEST_COLOR_CYAN    "\033[36m"
#define TEST_COLOR_BOLD    "\033[1m"

/* Test Runner Management */
TestRunner* test_runner_new(void) {
    TestRunner *runner = (TestRunner*)malloc(sizeof(TestRunner));
    if (!runner) return NULL;
    
    memset(runner, 0, sizeof(TestRunner));
    runner->verbose_output = false;
    runner->stop_on_failure = false;
    runner->color_output = true;
    runner->output_file = NULL;
    runner->output_fp = NULL;
    runner->debug_ctx = g_debug_ctx;
    
    return runner;
}

void test_runner_free(TestRunner *runner) {
    if (!runner) return;
    
    /* Free all test suites */
    TestSuite *suite = runner->suites;
    while (suite) {
        TestSuite *next = suite->next;
        test_suite_free(suite);
        suite = next;
    }
    
    /* Close output file */
    if (runner->output_fp && runner->output_fp != stdout) {
        fclose(runner->output_fp);
    }
    
    if (runner->output_file) {
        free(runner->output_file);
    }
    
    free(runner);
}

void test_runner_set_verbose(TestRunner *runner, Bool verbose) {
    if (runner) runner->verbose_output = verbose;
}

void test_runner_set_stop_on_failure(TestRunner *runner, Bool stop) {
    if (runner) runner->stop_on_failure = stop;
}

void test_runner_set_color_output(TestRunner *runner, Bool color) {
    if (runner) runner->color_output = color;
}

void test_runner_set_output_file(TestRunner *runner, const char *filename) {
    if (!runner) return;
    
    if (runner->output_file) {
        free(runner->output_file);
    }
    
    if (filename) {
        runner->output_file = strdup(filename);
        runner->output_fp = fopen(filename, "w");
        if (!runner->output_fp) {
            runner->output_fp = stdout;
        }
    } else {
        runner->output_file = NULL;
        runner->output_fp = stdout;
    }
}

/* Test Suite Management */
TestSuite* test_suite_new(const char *name, const char *description) {
    TestSuite *suite = (TestSuite*)malloc(sizeof(TestSuite));
    if (!suite) return NULL;
    
    suite->name = strdup(name);
    suite->description = strdup(description);
    suite->tests = NULL;
    suite->test_count = 0;
    suite->passed_count = 0;
    suite->failed_count = 0;
    suite->skipped_count = 0;
    suite->error_count = 0;
    suite->total_time = 0.0;
    suite->next = NULL;
    
    return suite;
}

void test_suite_free(TestSuite *suite) {
    if (!suite) return;
    
    /* Free all test cases */
    TestCase *test = suite->tests;
    while (test) {
        TestCase *next = test->next;
        test_case_free(test);
        test = next;
    }
    
    if (suite->name) free(suite->name);
    if (suite->description) free(suite->description);
    free(suite);
}

void test_runner_add_suite(TestRunner *runner, TestSuite *suite) {
    if (!runner || !suite) return;
    
    suite->next = runner->suites;
    runner->suites = suite;
    runner->suite_count++;
}

/* Test Case Management */
TestCase* test_case_new(const char *name, const char *description, 
                       TestCategory category, void (*test_func)(void)) {
    TestCase *test = (TestCase*)malloc(sizeof(TestCase));
    if (!test) return NULL;
    
    test->name = strdup(name);
    test->description = strdup(description);
    test->category = category;
    test->status = TEST_STATUS_PENDING;
    test->result = TEST_PASS;
    test->execution_time = 0.0;
    test->error_message = NULL;
    test->file = NULL;
    test->line = 0;
    test->test_function = test_func;
    test->next = NULL;
    
    return test;
}

void test_case_free(TestCase *test) {
    if (!test) return;
    
    if (test->name) free(test->name);
    if (test->description) free(test->description);
    if (test->error_message) free(test->error_message);
    if (test->file) free(test->file);
    free(test);
}

void test_suite_add_test(TestSuite *suite, TestCase *test) {
    if (!suite || !test) return;
    
    test->next = suite->tests;
    suite->tests = test;
    suite->test_count++;
}

/* Test Result Functions */
static TestCase *g_current_test = NULL;

void test_pass(const char *message, ...) {
    if (!g_current_test) return;
    
    g_current_test->result = TEST_PASS;
    g_current_test->status = TEST_STATUS_COMPLETED;
    
    if (g_test_runner && g_test_runner->verbose_output) {
        va_list args;
        va_start(args, message);
        
        if (g_test_runner->color_output) {
            printf("%s[PASS]%s ", TEST_COLOR_GREEN, TEST_COLOR_RESET);
        } else {
            printf("[PASS] ");
        }
        vprintf(message, args);
        printf("\n");
        
        va_end(args);
    }
}

void test_fail(const char *message, ...) {
    if (!g_current_test) return;
    
    g_current_test->result = TEST_FAIL;
    g_current_test->status = TEST_STATUS_COMPLETED;
    
    /* Allocate error message */
    va_list args;
    va_start(args, message);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), message, args);
    g_current_test->error_message = strdup(buffer);
    
    va_end(args);
    
    if (g_test_runner && g_test_runner->verbose_output) {
        if (g_test_runner->color_output) {
            printf("%s[FAIL]%s %s\n", TEST_COLOR_RED, TEST_COLOR_RESET, buffer);
        } else {
            printf("[FAIL] %s\n", buffer);
        }
    }
}

void test_skip(const char *message, ...) {
    if (!g_current_test) return;
    
    g_current_test->result = TEST_SKIP;
    g_current_test->status = TEST_STATUS_SKIPPED;
    
    if (g_test_runner && g_test_runner->verbose_output) {
        va_list args;
        va_start(args, message);
        
        if (g_test_runner->color_output) {
            printf("%s[SKIP]%s ", TEST_COLOR_YELLOW, TEST_COLOR_RESET);
        } else {
            printf("[SKIP] ");
        }
        vprintf(message, args);
        printf("\n");
        
        va_end(args);
    }
}

void test_error(const char *message, ...) {
    if (!g_current_test) return;
    
    g_current_test->result = TEST_ERROR;
    g_current_test->status = TEST_STATUS_ERROR;
    
    /* Allocate error message */
    va_list args;
    va_start(args, message);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), message, args);
    g_current_test->error_message = strdup(buffer);
    
    va_end(args);
    
    if (g_test_runner && g_test_runner->verbose_output) {
        if (g_test_runner->color_output) {
            printf("%s[ERROR]%s %s\n", TEST_COLOR_RED, TEST_COLOR_RESET, buffer);
        } else {
            printf("[ERROR] %s\n", buffer);
        }
    }
}

/* Test Execution */
Bool test_runner_run_all(TestRunner *runner) {
    if (!runner) return false;
    
    printf("Running all test suites...\n");
    printf("========================\n");
    
    clock_t start_time = clock();
    
    TestSuite *suite = runner->suites;
    while (suite) {
        if (!test_runner_run_suite(runner, suite->name)) {
            if (runner->stop_on_failure) {
                break;
            }
        }
        suite = suite->next;
    }
    
    clock_t end_time = clock();
    runner->total_execution_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    test_runner_print_summary(runner);
    return (runner->total_failed == 0 && runner->total_errors == 0);
}

Bool test_runner_run_suite(TestRunner *runner, const char *suite_name) {
    if (!runner || !suite_name) return false;
    
    TestSuite *suite = runner->suites;
    while (suite) {
        if (strcmp(suite->name, suite_name) == 0) {
            break;
        }
        suite = suite->next;
    }
    
    if (!suite) {
        printf("Test suite '%s' not found\n", suite_name);
        return false;
    }
    
    printf("\nRunning test suite: %s\n", suite->name);
    printf("Description: %s\n", suite->description);
    printf("Tests: %d\n", suite->test_count);
    printf("----------------------------------------\n");
    
    clock_t start_time = clock();
    
    TestCase *test = suite->tests;
    while (test) {
        if (!test_runner_run_test(runner, suite_name, test->name)) {
            if (runner->stop_on_failure) {
                break;
            }
        }
        test = test->next;
    }
    
    clock_t end_time = clock();
    suite->total_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    /* Update runner totals */
    runner->total_tests += suite->test_count;
    runner->total_passed += suite->passed_count;
    runner->total_failed += suite->failed_count;
    runner->total_skipped += suite->skipped_count;
    runner->total_errors += suite->error_count;
    
    return (suite->failed_count == 0 && suite->error_count == 0);
}

Bool test_runner_run_test(TestRunner *runner, const char *suite_name, const char *test_name) {
    if (!runner || !suite_name || !test_name) return false;
    
    TestSuite *suite = runner->suites;
    while (suite) {
        if (strcmp(suite->name, suite_name) == 0) {
            break;
        }
        suite = suite->next;
    }
    
    if (!suite) return false;
    
    TestCase *test = suite->tests;
    while (test) {
        if (strcmp(test->name, test_name) == 0) {
            break;
        }
        test = test->next;
    }
    
    if (!test) return false;
    
    /* Set current test for assertion macros */
    g_current_test = test;
    test->status = TEST_STATUS_RUNNING;
    
    if (runner->verbose_output) {
        printf("Running test: %s.%s\n", suite_name, test_name);
    }
    
    clock_t start_time = clock();
    
    /* Run the test */
    if (test->test_function) {
        test->test_function();
    }
    
    clock_t end_time = clock();
    test->execution_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    /* Update suite counts */
    switch (test->result) {
        case TEST_PASS:
            suite->passed_count++;
            break;
        case TEST_FAIL:
            suite->failed_count++;
            break;
        case TEST_SKIP:
            suite->skipped_count++;
            break;
        case TEST_ERROR:
            suite->error_count++;
            break;
    }
    
    g_current_test = NULL;
    return (test->result == TEST_PASS);
}

Bool test_runner_run_category(TestRunner *runner, TestCategory category) {
    if (!runner) return false;
    
    printf("Running tests in category: %d\n", category);
    printf("============================\n");
    
    TestSuite *suite = runner->suites;
    while (suite) {
        TestCase *test = suite->tests;
        while (test) {
            if (test->category == category) {
                test_runner_run_test(runner, suite->name, test->name);
            }
            test = test->next;
        }
        suite = suite->next;
    }
    
    return true;
}

/* Test Reporting */
void test_runner_print_summary(TestRunner *runner) {
    if (!runner) return;
    
    printf("\n");
    printf("Test Summary\n");
    printf("============\n");
    printf("Total Suites: %d\n", runner->suite_count);
    printf("Total Tests:  %d\n", runner->total_tests);
    
    if (runner->color_output) {
        printf("Passed:      %s%d%s\n", TEST_COLOR_GREEN, runner->total_passed, TEST_COLOR_RESET);
        printf("Failed:      %s%d%s\n", TEST_COLOR_RED, runner->total_failed, TEST_COLOR_RESET);
        printf("Skipped:     %s%d%s\n", TEST_COLOR_YELLOW, runner->total_skipped, TEST_COLOR_RESET);
        printf("Errors:      %s%d%s\n", TEST_COLOR_RED, runner->total_errors, TEST_COLOR_RESET);
    } else {
        printf("Passed:      %d\n", runner->total_passed);
        printf("Failed:      %d\n", runner->total_failed);
        printf("Skipped:     %d\n", runner->total_skipped);
        printf("Errors:      %d\n", runner->total_errors);
    }
    
    printf("Total Time:  %.3f seconds\n", runner->total_execution_time);
    
    if (runner->total_failed > 0 || runner->total_errors > 0) {
        printf("\nFailed Tests:\n");
        printf("-------------\n");
        
        TestSuite *suite = runner->suites;
        while (suite) {
            TestCase *test = suite->tests;
            while (test) {
                if (test->result == TEST_FAIL || test->result == TEST_ERROR) {
                    printf("%s.%s: %s\n", suite->name, test->name, 
                           test->error_message ? test->error_message : "Unknown error");
                }
                test = test->next;
            }
            suite = suite->next;
        }
    }
}

void test_runner_print_detailed_report(TestRunner *runner) {
    if (!runner) return;
    
    printf("\nDetailed Test Report\n");
    printf("===================\n");
    
    TestSuite *suite = runner->suites;
    while (suite) {
        printf("\nSuite: %s\n", suite->name);
        printf("Description: %s\n", suite->description);
        printf("Total Time: %.3f seconds\n", suite->total_time);
        printf("Tests: %d passed, %d failed, %d skipped, %d errors\n", 
               suite->passed_count, suite->failed_count, suite->skipped_count, suite->error_count);
        printf("----------------------------------------\n");
        
        TestCase *test = suite->tests;
        while (test) {
            const char *status_str;
            const char *color_start = "";
            const char *color_end = "";
            
            if (runner->color_output) {
                switch (test->result) {
                    case TEST_PASS:
                        color_start = TEST_COLOR_GREEN;
                        status_str = "PASS";
                        break;
                    case TEST_FAIL:
                        color_start = TEST_COLOR_RED;
                        status_str = "FAIL";
                        break;
                    case TEST_SKIP:
                        color_start = TEST_COLOR_YELLOW;
                        status_str = "SKIP";
                        break;
                    case TEST_ERROR:
                        color_start = TEST_COLOR_RED;
                        status_str = "ERROR";
                        break;
                    default:
                        status_str = "UNKNOWN";
                        break;
                }
                color_end = TEST_COLOR_RESET;
            } else {
                switch (test->result) {
                    case TEST_PASS: status_str = "PASS"; break;
                    case TEST_FAIL: status_str = "FAIL"; break;
                    case TEST_SKIP: status_str = "SKIP"; break;
                    case TEST_ERROR: status_str = "ERROR"; break;
                    default: status_str = "UNKNOWN"; break;
                }
            }
            
            printf("  %s[%s]%s %s (%.3fs)\n", color_start, status_str, color_end, 
                   test->name, test->execution_time);
            
            if (test->error_message) {
                printf("    Error: %s\n", test->error_message);
            }
            
            test = test->next;
        }
        
        suite = suite->next;
    }
}

/* Test Registration Functions */
void test_register_all_tests(void) {
    test_register_compiler_tests();
    test_register_runtime_tests();
    test_register_integration_tests();
}

void test_register_compiler_tests(void) {
    /* Compiler tests will be registered here */
}

void test_register_runtime_tests(void) {
    /* Runtime tests will be registered here */
}

void test_register_integration_tests(void) {
    /* Integration tests will be registered here */
}

/* Test Utilities */
void test_setup(void) {
    /* Global test setup */
}

void test_teardown(void) {
    /* Global test teardown */
}

void test_cleanup(void) {
    /* Global test cleanup */
}

/* Test Initialization and Cleanup */
void test_framework_init(void) {
    if (g_test_runner) return;
    
    g_test_runner = test_runner_new();
    if (g_test_runner) {
        test_register_all_tests();
    }
}

void test_framework_cleanup(void) {
    if (g_test_runner) {
        test_runner_free(g_test_runner);
        g_test_runner = NULL;
    }
}

/* Command Line Test Runner */
int test_main(int argc, char *argv[]) {
    test_framework_init();
    
    if (!g_test_runner) {
        printf("Failed to initialize test framework\n");
        return 1;
    }
    
    /* Parse command line arguments */
    Bool run_all = true;
    Bool verbose = false;
    Bool stop_on_failure = false;
    const char *suite_name = NULL;
    const char *test_name = NULL;
    const char *output_file = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "--stop-on-failure") == 0) {
            stop_on_failure = true;
        } else if (strcmp(argv[i], "--suite") == 0 && i + 1 < argc) {
            suite_name = argv[++i];
            run_all = false;
        } else if (strcmp(argv[i], "--test") == 0 && i + 1 < argc) {
            test_name = argv[++i];
            run_all = false;
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("SchismC Test Runner\n");
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -v, --verbose           Verbose output\n");
            printf("  --stop-on-failure       Stop on first failure\n");
            printf("  --suite <name>          Run specific test suite\n");
            printf("  --test <name>           Run specific test\n");
            printf("  --output <file>         Output results to file\n");
            printf("  -h, --help              Show this help\n");
            return 0;
        }
    }
    
    /* Configure test runner */
    test_runner_set_verbose(g_test_runner, verbose);
    test_runner_set_stop_on_failure(g_test_runner, stop_on_failure);
    if (output_file) {
        test_runner_set_output_file(g_test_runner, output_file);
    }
    
    /* Run tests */
    Bool success = false;
    if (run_all) {
        success = test_runner_run_all(g_test_runner);
    } else if (suite_name && test_name) {
        success = test_runner_run_test(g_test_runner, suite_name, test_name);
    } else if (suite_name) {
        success = test_runner_run_suite(g_test_runner, suite_name);
    } else {
        success = test_runner_run_all(g_test_runner);
    }
    
    test_framework_cleanup();
    return success ? 0 : 1;
}
