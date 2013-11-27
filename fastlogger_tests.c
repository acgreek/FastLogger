#define UNIT_TEST
#include <ExtremeCUnit.h>
#include "fastlogger.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

SUITE_DESTROY(FL) {
	fastlogger_close();
	return 0;
}

SUITE_TEST(FL,logg_global_debug_true) {
	int i=0;
	fastlogger_set_min_default_log_level(FL_DEBUG);
	Log(FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 1);
	return 0;
}
SUITE_TEST(FL,logg_global_threaded_debug_true) {
	int i=0;
	fastlogger_set_min_default_log_level(FL_DEBUG);
	fastlogger_separate_log_per_thread(1) ;
	Log(FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 1);
	return 0;
}
void * thread_func(void *ptr) {
	ptr = ptr;
//	int i=0;;
//	Log(FL_KEY_INFO, "test log %d\n", i++);
//
//	pthread_exit(NULL);
	return NULL;
}

SUITE_TEST(FL,logg_global_threaded_debug_true_two_threads) {
	int i=0;
	fastlogger_set_min_default_log_level(FL_DEBUG);
	fastlogger_separate_log_per_thread(1) ;
	Log(FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 1);
	pthread_t thread_id = 0;
	pthread_create(&thread_id,NULL ,thread_func,0);
	void * result;
	pthread_join(thread_id, &result);
	pthread_detach (thread_id);
	return 0;
}
SUITE_TEST(FL,logg_global_threaded_debug_true_secondtime) {
	int i=0;
	fastlogger_set_min_default_log_level(FL_DEBUG);
	fastlogger_separate_log_per_thread(1) ;
	Log(FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 1);
	Log(FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 2);
	fastlogger_close();
	return 0;
}
char * fastlogger_thread_local_file_name(const char * name, int i);
char * fastlogger_thread_local_file_name(const char * name, int i);
SUITE_TEST(FL,logg_thread_file_name) {
	char * str =  fastlogger_thread_local_file_name("output.log",0);
	AssertEqStr(str, "output_t0.log");
	free(str);
	return 0;
}
SUITE_TEST(FL,logg_global_debug_false) {
	int i=0;
	fastlogger_set_min_default_log_level(FL_ERROR);
	Log(FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 0);
	return 0;
}

FastLoggerNS_t testns_1 = FASTLOGGERNS_INIT(namespace_root);

SUITE_TEST(FL,logg_ns_debug_false) {
	int i=0;
	//fastlogger_set_loglevel("FL_ERROR,namespace_root,FL_ERROR");
	fastlogger_set_min_default_log_level(FL_DEBUG);
	fastlogger_set_min_log_level("namespace_root", FL_ERROR);

	LogNS(testns_1, FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 0);
	return 0;
}
SUITE_TEST(FL,logg_ns_global_debug_false) {
	int i=0;
	fastlogger_set_min_default_log_level(FL_ERROR);
	LogNS(testns_1,FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 0);
	return 0;
}
SUITE_TEST(FL,logg_ns_debug_true) {
	int i=0;
	fastlogger_set_min_default_log_level(FL_KEY_INFO);
	LogNS(testns_1, FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 1);
	return 0;
}
FastLoggerNS_t testns_1_child = FASTLOGGERNS_CHILD_INIT(testns_1, namespace_root::child);

SUITE_TEST(FL,logg_ns_parent_debug_true) {
	int i=0;
	fastlogger_set_min_log_level("namespace_root", FL_DEBUG);
	fastlogger_set_min_default_log_level(FL_ERROR);
	LogNS(testns_1_child, FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 1);
	return 0;
}

SUITE_TEST(FL,logg_ns_global_debug_true) {
	int i=0;
	fastlogger_set_min_default_log_level(FL_ERROR);
	fastlogger_set_min_log_level("namespace_root::child", FL_DEBUG);
	LogNS(testns_1_child,FL_DEBUG, "test log %d\n", i++);
	AssertEqInt(i, 1);
	return 0;
}

#include "darray.h"
TEST(create) {
	DynaArray ap = da_create(10, free);
	Assert(ap != NULL);
	da_destroy(ap);
	return 0;
}
