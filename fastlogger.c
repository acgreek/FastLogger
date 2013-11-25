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

#include "linkedlist.h"
#define MAX_PATH_LENGTH 1024

typedef struct _LoggerContext_t {
	char log_file_name[MAX_PATH_LENGTH+1];
	FILE * log_fd;
	size_t log_file_size;
} LoggerContext_t;

static LoggerContext_t g_logger_context =  {"output.log", NULL,0};
static pthread_mutex_t g_logger_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static ListNode_t  name_space_head = {NULL, NULL};

typedef struct _NameSpaceSetting{
	char * name_;
	fastlogger_level_t log_level_;
	ListNode_t node;
}NameSpaceSetting;

int findNameSpace(ListNode_t * ap, void * datap) {
	const char * name = (const char *) datap;
	NameSpaceSetting* a =  NODE_TO_ENTRY(NameSpaceSetting, node, ap);
	return (0 == strcmp(a->name_, name));

}
static void _NameSpaceHeadInit(void) {
	if (NULL == name_space_head.nextp)
		ListInitialize(&name_space_head);
}

void fastlogger_set_min_log_level(const char * namespace, fastlogger_level_t level) {
	pthread_mutex_lock(&g_logger_lock);
	_NameSpaceHeadInit();
	ListNode_t * matching_node_link = ListFind(&name_space_head, findNameSpace, (void*)namespace);
	NameSpaceSetting* matching_node;
	if (NULL == matching_node_link) {
			matching_node = malloc( sizeof(NameSpaceSetting));
			matching_node->name_ = strdup(namespace);
			ListAddEnd(&name_space_head, &matching_node->node);
	}
	else
		matching_node =  NODE_TO_ENTRY(NameSpaceSetting, node, matching_node_link);
	fastlogger_level_t f = FASTLOGGER_LEVEL(level);
	if (f >1)
		matching_node->log_level_ = f | (f-1);
	else
		matching_node->log_level_ = f;
	pthread_mutex_unlock(&g_logger_lock);
}


volatile fastlogger_level_t _global_log_level = FASTLOGGER_LEVEL(FL_ERROR);
volatile int global_fastlogger_load_level=0;

static volatile int _global_separate_log_per_thread = 0;
static size_t _global_max_bytes_per_file=10000;
static int _global_max_files=10;




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
	ListNode_t node;
	pthread_t thread_id;
	int id;
	LoggerContext_t ctx;
} thread_context_t ;

static ListNode_t  thread_context_head;

static pthread_key_t key;
static pthread_once_t once_control = PTHREAD_ONCE_INIT;

static void destructor(void *ptr) {
	thread_context_t * thread_ctx = (thread_context_t *) ptr;
	_fastlogger_close(&thread_ctx->ctx);
	ListRemove(&thread_ctx->node);
	free(thread_ctx);
	ptr=NULL;
	pthread_setspecific(key, NULL);
}
static void init_routine(void) {
	pthread_key_create(&key,destructor);
	ListInitialize(&thread_context_head);
}
static int thread_compare(ListNode_t * ap, ListNode_t *  dp, UNUSED void * datap) {
	dp= dp;
	thread_context_t * a =  NODE_TO_ENTRY(thread_context_t, node, ap);
	thread_context_t * b =  NODE_TO_ENTRY(thread_context_t, node, ap);
	return  a->id-b->id;
}
static thread_context_t*createThreadLocalContext(int i) {
	thread_context_t *curp;
	pthread_t thread_id = pthread_self();
	char *str = fastlogger_thread_local_file_name(g_logger_context.log_file_name, i);
	curp = malloc(sizeof(thread_context_t));
	curp->thread_id =  thread_id;
	strncpy (curp->ctx.log_file_name, str,sizeof(curp->ctx.log_file_name)-1);
	curp->ctx.log_fd= NULL;
	free(str);
	curp->id = i;
	ListAddSorted(&thread_context_head,&curp->node, thread_compare, NULL);
	pthread_setspecific(key, curp);
	return curp;
}
static int firstIdNotMatch(ListNode_t * nodep, void * datap) {
	thread_context_t * a =  NODE_TO_ENTRY(thread_context_t, node, nodep);
	int * idp = (int *)datap;
	if (a->id != *idp) {
		(*idp)++;
		return 0;
	}
	return 1;

}
static thread_context_t* addThreadLocalLoggerContext(void) {
	int i=0;
	thread_context_t *curp;
	pthread_mutex_lock(&g_logger_lock);

	ListFind(&thread_context_head, firstIdNotMatch, &i);
	curp = createThreadLocalContext(i);

	pthread_mutex_unlock(&g_logger_lock);
	return curp;
}
static thread_context_t* _getThreadLocalContext (int create) {
	pthread_once(&once_control, init_routine);
	thread_context_t * ctxp = (thread_context_t *) pthread_getspecific(key);
	if (NULL == ctxp) {
		if (0 == create)
			return NULL;
		return addThreadLocalLoggerContext();
	}
	return ctxp;

}
static LoggerContext_t * _getThreadLocalLoggerContext (int create) {
	thread_context_t * ctxp = _getThreadLocalContext (create);
	if (NULL == ctxp)
		return NULL;
	return &ctxp->ctx;
}
static LoggerContext_t * getThreadLocalLoggerContext (void) {
	return _getThreadLocalLoggerContext (1);
}
static LoggerContext_t * getLoggerContext (void) {
	if (_global_separate_log_per_thread)
		return getThreadLocalLoggerContext ();
	return &g_logger_context;
}


void  fastlogger_separate_log_per_thread(size_t i) {
	_global_separate_log_per_thread=i;
}

size_t fastlogger_current_log_file_size(void) {
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
void fastlogger_set_min_default_log_level(fastlogger_level_t level) {
	fastlogger_level_t f = FASTLOGGER_LEVEL(level);
	if (f >1)
		_global_log_level = f | (f-1);
	else
		_global_log_level = f;
}
void fastlogger_enable_log_level(fastlogger_level_t level) {
	fastlogger_level_t f = FASTLOGGER_LEVEL(level);
	_global_log_level |= f ;
}
void fastlogger_disable_log_level(fastlogger_level_t level) {
	fastlogger_level_t f = FASTLOGGER_LEVEL(level);
	_global_log_level &= ~f ;
}
void fastlogger_set_loglevel(const char * loglevel_str);

static void _fastlogger_close(LoggerContext_t * logp) {
	if (logp->log_fd) {
		fclose (logp->log_fd);
		logp->log_fd = NULL;
	}
}
void fastlogger_close_thread_local(void) {
	thread_context_t * ctxp = _getThreadLocalContext (0);
	if (ctxp)
		destructor(ctxp);
}
void fastlogger_close(void) {
	pthread_mutex_lock(&g_logger_lock);
	LoggerContext_t * logp = &g_logger_context;
	_fastlogger_close(logp);
	pthread_mutex_unlock(&g_logger_lock);
	fastlogger_close_thread_local();

}
static void open_log_file(void) {
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


void rotateFiles(LoggerContext_t *logp) {
	int files =_global_max_files;
	if (files > 1) {
		char log_file_name[MAX_PATH_LENGTH];
		char log_file_name_next[MAX_PATH_LENGTH];
		files--;
		snprintf(log_file_name, sizeof(log_file_name)-1,"%s.%d", logp->log_file_name, files);
		unlink(log_file_name);
		while (files > 0) {
			snprintf(log_file_name_next, sizeof(log_file_name_next)-1,"%s.%d", logp->log_file_name, files+1);
			snprintf(log_file_name, sizeof(log_file_name)-1,"%s.%d", logp->log_file_name, files);
			rename(log_file_name,log_file_name_next);
			files--;
		}
		snprintf(log_file_name_next, sizeof(log_file_name_next)-1,"%s.%d", logp->log_file_name, files+1);
		snprintf(log_file_name, sizeof(log_file_name)-1,"%s", logp->log_file_name);
		rename(log_file_name,log_file_name_next);
	}
	else {
		unlink(logp->log_file_name);
	}
	fclose (logp->log_fd);
	logp->log_fd = NULL;

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
	if (logp->log_file_size > _global_max_bytes_per_file){
		rotateFiles(logp);
	}

	pthread_mutex_unlock(&g_logger_lock);
	return rtn;
}

fastlogger_level_t _fastlogger_ns_load(FastLoggerNS_t *nsp){
	pthread_mutex_lock(&g_logger_lock);
	_NameSpaceHeadInit();
	ListNode_t * matching_node_link = ListFind(&name_space_head, findNameSpace, (void*)nsp->name );
	if (NULL != matching_node_link ) {
		NameSpaceSetting * matching_node =  NODE_TO_ENTRY(NameSpaceSetting, node, matching_node_link);
		nsp->level = matching_node->log_level_;
	}
	else if (nsp->parentp) {
		if (_global_fastlogger_load_level != nsp->parentp->load_level)
			_fastlogger_ns_load(nsp->parentp);
			nsp->level = nsp->parentp->level;
	}
	else {
		nsp->level= _global_log_level;
	}
	pthread_mutex_unlock(&g_logger_lock);
	return nsp->level;
}

