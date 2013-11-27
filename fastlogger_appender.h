#ifndef FASTLOGGER_APPENDER_H
#define FASTLOGGER_APPENDER_H
#include <stdio.h>
#include "fastlogger.h"
#include "linkedlist.h"

typedef struct _Appender {
	ListNode_t link;
	volatile fastlogger_level_t level;
	void * ctx;
	void * (*init ) (int id, ...);
	void (*fini ) (void *ptr);
	int (*write) (void *ptr, const char * what);
}Appender;
#endif
