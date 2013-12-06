#ifndef FASTLOGGER_APPENDER_H
#define FASTLOGGER_APPENDER_H
#include <stdio.h>
#include "fastlogger.h"
#include "linkedlist.h"
#include "darray.h"

typedef struct _WriteCtx {
	fastlogger_level_t level;
	const char * what;
}WriteCtx;

typedef struct _AppenderParam {
	char * key;
	char * value;
}AppenderParam;

typedef struct _Appender {
	char * name;
	ListNode_t link;
	volatile fastlogger_level_t level;
	void * ctx;
	void * (*init ) (int id, DynaArray ap);
	void (*fini ) (void *ptr);
	int (*write) (void *ptr,WriteCtx *writeCtx);
}Appender;


#endif
