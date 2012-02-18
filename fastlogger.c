#include "fastlogger.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#define __USE_GNU 
#include <pthread.h>
#include <time.h>

volatile fastlogger_level_t _global_log_level= FL_ERROR;
volatile int _global_fastlogger_load_level=0;

static char g_log_file_name[1024] ="output.log";
static FILE * g_log_fd = NULL;
static pthread_mutex_t g_logger_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

void fastlogger_set_default_log_level(fastlogger_level_t level) {
	_global_log_level= level;
}
void fastlogger_close(void) {

	pthread_mutex_lock(&g_logger_lock);
	if (g_log_fd) {
		fclose (g_log_fd);
		g_log_fd = NULL;
	}
	pthread_mutex_unlock(&g_logger_lock);
}
static void open_log_file(void ) {
	g_log_fd = fopen (g_log_file_name,"a");
	atexit(fastlogger_close);
}
int _real_logger(const char * fmt, ...) {
	pthread_mutex_lock(&g_logger_lock);
	if (NULL == g_log_fd) {
		open_log_file();
	}
	va_list ap;
	va_start(ap, fmt); 
	int rtn= vfprintf(g_log_fd, fmt, ap);
        va_end(ap);
	pthread_mutex_unlock(&g_logger_lock);
	return rtn;
}
void _fastlogger_ns_load(FastLoggerNS_t *nsp){
	pthread_mutex_lock(&g_logger_lock);
	nsp->level= _global_log_level;
	pthread_mutex_unlock(&g_logger_lock);
}


