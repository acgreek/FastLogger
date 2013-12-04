#define UNIT_TEST
#include <ExtremeCUnit.h>
#include "fastlogger.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

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
SUITE_TEST(FL,writtenToFile) {
	int i=0;
	unlink("output.log");
	fastlogger_create_appender("FileOutput","file", NULL);
	fastlogger_add_default_appender("FileOutput");
	fastlogger_set_min_default_log_level(FL_ERROR);
	fastlogger_set_min_log_level("namespace_root::child", FL_DEBUG);
	LogNS(testns_1_child,FL_DEBUG, "test log %d\n", i++);
	AssertEqInt(i, 1);

	FILE *file;
	Assert((file = fopen("output.log", "r")) != NULL) ;
	fclose (file);
	return 0;
}



#include "darray.h"
TEST(create) {
	DynaArray ap = da_create(10, free);
	Assert(ap != NULL);
	da_destroy(&ap);
	return 0;
}
TEST(addOne) {
	DynaArray ap = da_create(10, free);
	Assert(ap != NULL);
	da_add_item(&ap,strdup("ff"));
	AssertEqInt(da_len(ap), 1);
	da_destroy(&ap);
	return 0;
}
TEST(addTwo) {
	DynaArray ap = da_create(10, free);
	Assert(ap != NULL);
	da_add_item(&ap,strdup("ff"));
	da_add_item(&ap,strdup("dd"));
	AssertEqInt(da_len(ap), 2);
	AssertEqStr(ap[0], "ff");
	AssertEqStr(ap[1], "dd");
	Assert(ap[2]==NULL );
	da_destroy(&ap);
	return 0;
}
TEST(addthreeGrow) {
	DynaArray ap = da_create(3, free);
	Assert(ap != NULL);
	da_add_item(&ap,strdup("ff"));
	da_add_item(&ap,strdup("dd"));
	da_add_item(&ap,strdup("cc"));
	AssertEqInt(da_len(ap), 3);
	AssertEqStr(ap[0], "ff");
	AssertEqStr(ap[1], "dd");
	AssertEqStr(ap[2], "cc");
	Assert(ap[3]==NULL );
	da_add_item(&ap,strdup("dd"));
	AssertEqStr(ap[0], "ff");
	AssertEqStr(ap[1], "dd");
	AssertEqStr(ap[2], "cc");
	AssertEqStr(ap[3], "dd");
	Assert(ap[4]==NULL );
	da_destroy(&ap);
	return 0;
}
void setFirstTo(void * ptr, void * extrap) {
	extrap= extrap;
	char * str = (char *)ptr;
	str[0] = '0';
}
TEST(foreach) {
	DynaArray ap = da_create(10, free);
	Assert(ap != NULL);
	da_add_item(&ap,strdup("ff"));
	da_foreach( ap,setFirstTo , NULL );
	AssertEqStr(ap[0], "0f");
	da_destroy(&ap);
	return 0;
}
