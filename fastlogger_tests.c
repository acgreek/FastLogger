#include <ExtremeCUnit.h>
#include "fastlogger.h"
#include <stdio.h>


TEST(logg_global_debug_true) {
	int i=0;
	fastlogger_set_default_log_level(FL_DEBUG);
	Log(FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 1);
	fastlogger_close();
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
	Log(FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 0);
	return 0;
}
TEST(logg_ns_global_debug_false) {
	int i=0;
	fastlogger_set_default_log_level(FL_ERROR);
	fastlogger_set_default_log_level(FL_ERROR);
	Log(FL_KEY_INFO, "test log %d\n", i++);
	AssertEqInt(i, 0);
	return 0;
}
