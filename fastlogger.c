#include "fastlogger.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#define __USE_GNU 
#include <pthread.h>
#include <time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>


volatile fastlogger_level_t _global_log_level= FL_ERROR;
volatile int global_fastlogger_load_level=0;

static volatile int _global_separate_log_per_thread = 0;

typedef struct _LoggerContext_t {
	char log_file_name[1024];
	FILE * log_fd;
	size_t log_file_size;
} LoggerContext_t;

static LoggerContext_t g_logger_context =  {"output.log", NULL,0};
static pthread_mutex_t g_logger_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static void _fastlogger_close(LoggerContext_t * logp);

char * fastlogger_thread_local_file_name(const char * name, int i) {
	int len = strlen (name) +10;
	char * new_file =malloc (len);
	char *offset;
	strcpy(new_file, name);
	offset = strstr(new_file,".log");
	if (offset) {
		sprintf(offset, "_t%d.log", i);
	}
	return new_file;
}

typedef struct _thread_context_t {
	struct _thread_context_t *nextp;
	struct _thread_context_t *prevp;
	pthread_t thread_id;
	LoggerContext_t ctx;
} thread_context_t ;

thread_context_t thread_context_head =  {NULL,NULL, 0, {"output.log", NULL,0}};

pthread_key_t key;
pthread_once_t once_control = PTHREAD_ONCE_INIT;

void destructor(void *ptr) {
	thread_context_t * thread_ctx = (thread_context_t *) ptr;
	_fastlogger_close(&thread_ctx->ctx);
	thread_ctx->prevp = thread_ctx->nextp;
	free(thread_ctx);
	ptr=NULL;
	pthread_setspecific(key, NULL);
}
void init_routine(void) {
	pthread_key_create(&key,destructor);
}
static LoggerContext_t * addThreadLocalLoggerContext() {
	thread_context_t *curp, *prevp;
	int i=0;
	pthread_t thread_id = pthread_self();
	pthread_mutex_lock(&g_logger_lock);
	curp = thread_context_head.nextp;
	prevp = &thread_context_head;
	while (NULL != curp) {
		if (thread_id == curp->thread_id) {
			break;
		}
		prevp= curp;
		curp = curp->nextp;
		i++;
	}
	if (NULL == curp) {
		char *str = fastlogger_thread_local_file_name(g_logger_context.log_file_name, i);
		curp = malloc(sizeof(thread_context_t));
		curp->thread_id =  thread_id;

		strncpy (curp->ctx.log_file_name, str,sizeof(curp->ctx.log_file_name)-1);
		curp->ctx.log_fd= NULL;
		free(str);
		curp->nextp=NULL;
		prevp->nextp= curp;
		curp->prevp = prevp;

		pthread_setspecific(key, curp);
	}
	pthread_mutex_unlock(&g_logger_lock);
	return &curp->ctx;	
}
static LoggerContext_t * getThreadLocalLoggerContext () {
	pthread_once(&once_control, init_routine);
	thread_context_t * ctxp = (thread_context_t *) pthread_getspecific(key);
	if (NULL == ctxp)
		return addThreadLocalLoggerContext();
	return &ctxp->ctx;
}
static LoggerContext_t * getLoggerContext () {
	if (_global_separate_log_per_thread)
		return getThreadLocalLoggerContext ();
	return &g_logger_context;
}


void  fastlogger_separate_log_per_thread(int i) {
	_global_separate_log_per_thread=1;
}

size_t fastlogger_current_log_file_size() {
	pthread_mutex_lock(&g_logger_lock);
	LoggerContext_t * logp = getLoggerContext () ;
	size_t log_file_size = logp->log_file_size;	
	pthread_mutex_unlock(&g_logger_lock);
	return log_file_size;
}

void fastlogger_set_log_filename(const char *file_name){
	pthread_mutex_lock(&g_logger_lock);
	LoggerContext_t * logp = getLoggerContext () ;
	snprintf(logp->log_file_name,sizeof(logp->log_file_name)-1,"%s.log",file_name);
	pthread_mutex_unlock(&g_logger_lock);
}
void fastlogger_set_default_log_level(fastlogger_level_t level) {
	_global_log_level = level;
}
static void _fastlogger_close(LoggerContext_t * logp) {
	if (logp->log_fd) {
		fclose (logp->log_fd);
		logp->log_fd = NULL;
	}
}
void fastlogger_close(void) {
	pthread_mutex_lock(&g_logger_lock);
	LoggerContext_t * logp = getLoggerContext () ;
	_fastlogger_close(logp);
	pthread_mutex_unlock(&g_logger_lock);

	thread_context_t * ctxp = (thread_context_t *) pthread_getspecific(key);
	if (ctxp)
		destructor(ctxp);

}
static void open_log_file(void ) {
	LoggerContext_t * logp = getLoggerContext () ;
	logp->log_fd = fopen (logp->log_file_name,"a");
	struct stat ss;
	if (logp->log_fd != NULL && 0 == fstat(fileno(logp->log_fd), &ss)) {
		logp->log_file_size = ss.st_size;
	}
	atexit(fastlogger_close);
}

static int write_log_message(FILE * fid, const char * fmt, va_list ap) {
	int rtn = 0;
	char time_str[200];
	time_t now;
	now = time(NULL);
	struct tm tm;
	gmtime_r(&now, &tm);
	strftime(time_str, sizeof(time_str) -1, "%Y-%m-%d %H:%M:%S ", &tm);
	rtn +=fputs(time_str,fid);
	rtn +=vfprintf(fid, fmt, ap);
	return rtn;
}


int _real_logger(const char * fmt, ...) {
	pthread_mutex_lock(&g_logger_lock);
	LoggerContext_t * logp = getLoggerContext () ;
	if (NULL == logp->log_fd) {
		open_log_file();
	}
	va_list ap;
	va_start(ap, fmt); 
	int rtn=write_log_message(logp->log_fd, fmt, ap);
        va_end(ap);
	logp->log_file_size += rtn;
	pthread_mutex_unlock(&g_logger_lock);
	return rtn;
}

fastlogger_level_t _fastlogger_ns_load(FastLoggerNS_t *nsp){
	pthread_mutex_lock(&g_logger_lock);
	nsp->level= _global_log_level;
	pthread_mutex_unlock(&g_logger_lock);
	return nsp->level;
}



