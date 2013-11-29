#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "fastlogger_appender.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h> //for unlink
#define MAX_PATH_LENGTH 1024

typedef struct _FileAppender_t {
	int id_;
	char *filename_;
	FILE *fid_;
	size_t filesize_;
	size_t max_bytes_per_file_;
	int max_files_;
} FileAppender_t ;


static void * file_appender_init(int id, ...) {
	FileAppender_t * fap = malloc (sizeof(FileAppender_t));
	fap->id_= id;
	fap->filename_ = strdup("output.log");
	fap->fid_ = 0;
	fap->filesize_ = 0;
	fap->max_bytes_per_file_ = 100000;
	fap->max_files_= 10;
	return fap;
}
static void file_appender_fini(void * ptr) {
	FileAppender_t * fap = (FileAppender_t* )ptr;
	if (fap->filename_) free(fap->filename_); fap->filename_=NULL;
	free(fap);
}

static void open_log_file(FileAppender_t *fap) {
	fap->fid_ = fopen (fap->filename_,"a");
	struct stat ss;
	if (fap->fid_ != NULL && 0 == fstat(fileno(fap->fid_), &ss)) {
		fap->filesize_ = ss.st_size;
	}
}

static int write_log_message(FILE * fid, const char * what) {
	int rtn = 0;
	char time_str[200];
	time_t now;
	now = time(NULL);
	struct tm tm;
	gmtime_r(&now, &tm);
	strftime(time_str, sizeof(time_str) -1, "%Y-%m-%d %H:%M:%S ", &tm);
	rtn +=fputs(time_str,fid);
	rtn +=fputs(what, fid);
	return rtn;
}


static void rotateFiles(FileAppender_t *fap) {
	int files =fap->max_files_;
	if (files > 1) {
		char log_file_name[MAX_PATH_LENGTH];
		char log_file_name_next[MAX_PATH_LENGTH];
		files--;
		snprintf(log_file_name, sizeof(log_file_name)-1,"%s.%d", fap->filename_, files);
		unlink(log_file_name);
		while (files > 0) {
			snprintf(log_file_name_next, sizeof(log_file_name_next)-1,"%s.%d", fap->filename_, files+1);
			snprintf(log_file_name, sizeof(log_file_name)-1,"%s.%d", fap->filename_, files);
			rename(log_file_name,log_file_name_next);
			files--;
		}
		snprintf(log_file_name_next, sizeof(log_file_name_next)-1,"%s.%d", fap->filename_, files+1);
		snprintf(log_file_name, sizeof(log_file_name)-1,"%s", fap->filename_);
		rename(log_file_name,log_file_name_next);
	}
	else {
		unlink(fap->filename_);
	}
	fclose (fap->fid_);
	fap->fid_ = NULL;

}
static int file_appender_write(void * ptr, const char * what) {
	FileAppender_t * fap = (FileAppender_t* )ptr;
	if (NULL == fap->fid_) {
		open_log_file(fap);
	}
	int rtn=write_log_message(fap->fid_, what);
	fap->filesize_ += rtn;
	if (fap->filesize_ > fap->max_bytes_per_file_){
		rotateFiles(fap);
	}
	return 0;
}

Appender* make_file_apender(const char * type, va_list arg) {
	arg= arg;
	type=type;
	Appender * ap = malloc (sizeof(Appender));
	ap->init = file_appender_init;
	ap->fini = file_appender_fini;
	ap->write = file_appender_write;
	ap->ctx  = ap->init(0);
	return ap;
}

