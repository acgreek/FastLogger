#include <ExtremeCUnit.h>
#include "fastlogger.h"
#include <stdio.h>
#include <stdlib.h>


/*
TEST(logg_global_debug_true) {
	int i=0;
	fastlogger_set_default_log_level(FL_DEBUG);
	Log(FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 1);
	fastlogger_close();
	return 0;
}
*/
TEST(logg_global_threaded_debug_true) {
	int i=0;
	fastlogger_set_default_log_level(FL_DEBUG);
	fastlogger_separate_log_per_thread(1) ;
	Log(FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 1);
	fastlogger_close();
	return 0;
}
char * fastlogger_thread_local_file_name(const char * name, int i);
TEST(logg_thread_file_name) {
	char * str =  fastlogger_thread_local_file_name("output.log",0);
	AssertEqStr(str, "output_t0.log");
	free(str);
	return 0;
}
TEST(logg_global_debug_false) {
	int i=0;
	fastlogger_set_default_log_level(FL_ERROR);
	Log(FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 0);
	return 0;
}

FastLoggerNS_t testns_1 = FASTLOGGERNS_INIT(testns_1);

TEST(logg_ns_debug_false) {
	int i=0;
	fastlogger_set_default_log_level(FL_ERROR);
	LogNS(testns_1, FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 0);
	return 0;
}
TEST(logg_ns_global_debug_false) {
	int i=0;
	fastlogger_set_default_log_level(FL_ERROR);
	fastlogger_set_default_log_level(FL_ERROR);
	LogNS(testns_1,FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 0);
	return 0;
}
