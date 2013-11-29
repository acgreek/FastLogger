
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include "fastlogger.h"
#include "appender_factory.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>


#include "linkedlist.h"
#define MAX_PATH_LENGTH 1024
#define IFFN(x) do {if (x) {free(x);x=NULL;}} while(0)

typedef struct _NameSpacePrivate_t {
	void *appenders_[2];
} NameSpacePrivate_t;

typedef struct _LoggerContext_t {
	char log_file_name[MAX_PATH_LENGTH+1];
	FILE * log_fd;
	size_t log_file_size;
} LoggerContext_t;

static LoggerContext_t g_logger_context =  {"output.log", NULL,0};
static pthread_mutex_t g_logger_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static ListNode_t  name_space_head = {NULL, NULL};
static ListNode_t  appender_head = {NULL, NULL};

FastLoggerNS_t _global_name_base = {NULL, "", FASTLOGGER_LEVEL( FL_ERROR), 0, NULL};

volatile int _global_fastlogger_load_level=1;

static volatile int _global_separate_log_per_thread = 0;
//static int _global_max_files=10;

typedef struct _NameSpaceSetting{
	char * name_;
	fastlogger_level_t log_level_;
	ListNode_t link;
	ListNode_t appender_name_link;
	void * appender_name_array[10];
}NameSpaceSetting;

static void * initPrivateNameSpace();

static int findNameSpace(ListNode_t * ap, void * datap) {
	const char * name = (const char *) datap;
	NameSpaceSetting* a =  NODE_TO_ENTRY(NameSpaceSetting, link, ap);
	return (0 == strcmp(a->name_, name));

}
typedef struct _AppenderName {
	char * name;
	ListNode_t link;
}AppenderName;

static int findAppender(ListNode_t * ap, void * datap) {
	const char * name = (const char *) datap;
	AppenderName * a =  NODE_TO_ENTRY(AppenderName, link, ap);
	return (0 == strcmp(a->name, name));
}

static void _NameSpaceHeadInit(void) {
	if (NULL == name_space_head.nextp)
		ListInitialize(&name_space_head);
}
static NameSpaceSetting* create_namespace_settings(const char * namespace) {
	NameSpaceSetting* matching_node;
	matching_node = malloc( sizeof(NameSpaceSetting));
	matching_node->name_ = strdup(namespace);
	matching_node->log_level_ = 0;
	ListAddEnd(&name_space_head, &matching_node->link);
	ListInitialize(&matching_node->appender_name_link);
	matching_node->appender_name_array[0] =NULL;
	matching_node->appender_name_array[1] =NULL;
	return matching_node;

}

static NameSpaceSetting* getNameSpaceSetting(const char * namespace) {
	_NameSpaceHeadInit();
	ListNode_t * matching_node_link = ListFind(&name_space_head, findNameSpace, (void*)namespace);
	NameSpaceSetting* matching_node;
	if (NULL == matching_node_link) {
			matching_node = create_namespace_settings(namespace);
	}
	else
		matching_node =  NODE_TO_ENTRY(NameSpaceSetting, link, matching_node_link);
	return matching_node;
}

void fastlogger_set_min_log_level(const char * namespace, fastlogger_level_t level) {
	pthread_mutex_lock(&g_logger_lock);
	NameSpaceSetting* matching_node = getNameSpaceSetting(namespace);
	fastlogger_level_t f = FASTLOGGER_LEVEL(level);
	if (f >1)
		matching_node->log_level_ = f | (f-1);
	else
		matching_node->log_level_ = f;
	_global_fastlogger_load_level++;

	pthread_mutex_unlock(&g_logger_lock);
}

void fastlogger_ns_enable_log_level(const char * namespace, fastlogger_level_t level) {
	pthread_mutex_lock(&g_logger_lock);
	NameSpaceSetting* matching_node = getNameSpaceSetting(namespace);
	fastlogger_level_t f = FASTLOGGER_LEVEL(level);
	matching_node->log_level_|= f ;
	_global_fastlogger_load_level++;
	pthread_mutex_unlock(&g_logger_lock);
}
void fastlogger_enable_log_level(fastlogger_level_t level) {
	fastlogger_ns_enable_log_level("", level);
}
void fastlogger_ns_disable_log_level(const char * namespace, fastlogger_level_t level) {
	pthread_mutex_lock(&g_logger_lock);
	NameSpaceSetting* matching_node = getNameSpaceSetting(namespace);
	fastlogger_ns_disable_log_level("", level);
	fastlogger_level_t f = FASTLOGGER_LEVEL(level);
	matching_node->log_level_ &= ~f ;
	_global_fastlogger_load_level++;
	pthread_mutex_unlock(&g_logger_lock);
}
void fastlogger_disable_log_level(fastlogger_level_t level) {
	fastlogger_ns_disable_log_level("", level);
}

void fastlogger_set_loglevel(const char * loglevel_str);

void fastlogger_set_min_default_log_level(fastlogger_level_t level) {
	fastlogger_set_min_log_level("", level);
}

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


static void nameSpaceFree(ListNode_t * ap, void * datap) {
	if (datap == ap)
		return;
	NameSpaceSetting* a =  NODE_TO_ENTRY(NameSpaceSetting, link, ap);
	IFFN(a->name_);
	ListRemove(ap);
	free(a);
}

void fastlogger_close(void) {
	pthread_mutex_lock(&g_logger_lock);
	LoggerContext_t * logp = &g_logger_context;
	_fastlogger_close(logp);
	if (name_space_head.nextp)
		ListApplyAll(&name_space_head,nameSpaceFree, &name_space_head);

	pthread_mutex_unlock(&g_logger_lock);
	fastlogger_close_thread_local();

}
/*
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


static void rotateFiles(LoggerContext_t *logp) {
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
*/
void writeToAppender(ListNode_t * nodep, void * datap) {
	const char * what= datap;
	Appender * appender =  NODE_TO_ENTRY(Appender, link, nodep);
	appender->write(appender->ctx,what);
}

static NameSpaceSetting * findNameSpaceSettings(const char * name) {
	NameSpaceSetting* matching_node = NULL;
	ListNode_t * matching_node_link = ListFind(&name_space_head, findNameSpace, (void*)name);
	if (matching_node_link){
		matching_node =  NODE_TO_ENTRY(NameSpaceSetting, link, matching_node_link);
	}
	return matching_node;

}

static int _ns_write (FastLoggerNS_t *nsp,const char * what) {
	if (NULL == nsp->private_datap_  ) {
		nsp->private_datap_ = initPrivateNameSpace (findNameSpaceSettings(nsp->name));
	}
	NameSpacePrivate_t * c = (NameSpacePrivate_t *) nsp->private_datap_;
	//ListApplyAll(&c->appenders_, writeToAppender,(char *) what);
	if (c->appenders_[0] ) {
		Appender * appender = (Appender * ) c->appenders_[0];
		appender->write(appender->ctx, what);
	}
	if (nsp->parentp)
		_ns_write (nsp->parentp,what);
	return 0;
}

int _real_logger(FastLoggerNS_t *nsp, const char * fmt, ...) {
	char * what = NULL;
	va_list ap;
	va_start(ap, fmt);
	vasprintf(&what,fmt, ap);
    va_end(ap);
	int rtn=  _ns_write(nsp,what) ;
	free(what);
	return rtn;
}
static void * initPrivateNameSpace(NameSpaceSetting * matching_node) {
		void * ptr =  malloc (sizeof(NameSpacePrivate_t));
		memset(ptr, 0, sizeof(NameSpacePrivate_t));
		NameSpacePrivate_t * c = (NameSpacePrivate_t *) ptr;
		//ListInitialize(&c->appenders_);
		c->appenders_[0] = NULL;
		c->appenders_[1] = NULL;
		if (matching_node && matching_node->appender_name_array[0] && appender_head.nextp) {
			ListNode_t * matching_appender_link = ListFind(&appender_head, findAppender,  matching_node->appender_name_array[0]);
			if (matching_appender_link) {

				c->appenders_[0] =NODE_TO_ENTRY(Appender, link, matching_appender_link);;
			}
		}
		return ptr;

}

fastlogger_level_t _fastlogger_ns_load(FastLoggerNS_t *nsp){
	pthread_mutex_lock(&g_logger_lock);
	_NameSpaceHeadInit();
	ListNode_t * matching_node_link = ListFind(&name_space_head, findNameSpace, (void*)nsp->name );
	NameSpaceSetting * matching_node= NULL;
	if (NULL != matching_node_link ) {
		matching_node =  NODE_TO_ENTRY(NameSpaceSetting, link, matching_node_link);
		nsp->level = matching_node->log_level_;
	}
	else if (nsp->parentp) {
		if (_global_fastlogger_load_level != nsp->parentp->load_level)
			_fastlogger_ns_load(nsp->parentp);
			nsp->level = nsp->parentp->level;
	}
	else { //must be root
		exit(-1); //should never happen
	}
	if (NULL == nsp->private_datap_) {
		nsp->private_datap_ = initPrivateNameSpace (matching_node);
	}
	pthread_mutex_unlock(&g_logger_lock);
	return nsp->level;
}

void fastlogger_create_appender(const char * name,const char * type ,...) {
	va_list ap;
	va_start(ap, type);
	Appender * appenderp = make_file_apender(type, ap);
    va_end(ap);
	appenderp->name = strdup(name);
	if (appender_head.nextp == NULL)
		ListInitialize(&appender_head);
	ListAddEnd(&appender_head, &appenderp->link);
}


void fastlogger_add_default_appender(const char * name) {
	_NameSpaceHeadInit();
	ListNode_t * matching_node_link = ListFind(&name_space_head, findNameSpace, (void*)"");
	NameSpaceSetting* matching_node;
	if (NULL == matching_node_link) {
		matching_node = create_namespace_settings("");
	}
	else {
		matching_node =  NODE_TO_ENTRY(NameSpaceSetting, link, matching_node_link);
	}
	AppenderName * an = malloc (sizeof(AppenderName));
	an->name  = strdup(name);
	ListAddEnd(&matching_node->appender_name_link, &an->link);
	matching_node->appender_name_array[0] = an->name;
}

