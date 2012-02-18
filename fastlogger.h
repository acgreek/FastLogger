#ifndef FASTLOGGER_H
#define FASTLOGGER_H

typedef enum {FL_ERROR, FL_KEY_INFO, FL_EXTRA_INFO, FL_DEBUG, FL_DEBUG_EXCESSIVE} fastlogger_level_t;

extern volatile fastlogger_level_t _global_log_level;
void fastlogger_close(void);
volatile int _global_fastlogger_load_level;


void fastlogger_set_default_log_level(fastlogger_level_t level);

int _real_logger(const char * fmt, ...) ;

#define Log(LVL, FMT, ...) ((LVL) <= _global_log_level) ? _real_logger(FMT, ##__VA_ARGS__) : 0

typedef struct _FastLoggerNS_t {
	struct _FastLoggerNS_t * parentp;
	const char * name;
	volatile fastlogger_level_t level;
	volatile int load_level;
} FastLoggerNS_t;
void _fastlogger_ns_load(FastLoggerNS_t *nsp);

#define FASTLOGGERNS_INIT(name) {NULL, #name, -1, -1 }
#define FASTLOGGERNS_CHILD_INIT(parentns, name) {&parentns, #name, -1, -1 }

#define LogNS(NS, LVL, FMT, ...) (((LVL) <= (_global_fastlogger_load_level != NS.load_level? _fastlogger_ns_load(&NS) : NS.level))) ? _real_logger("%s: " FMT,NS.name, ##__VA_ARGS__) : 0

#endif
