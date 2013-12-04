#ifndef DARRAY_H
#define DARRAY_H
#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED __attribute__ ((unused))
#elif defined(__LCLINT__)
# define UNUSED
#else
# define UNUSED
#endif
#include <string.h> // memset
#include <stdio.h>
#include <stdlib.h>
typedef void ** DynaArray ;

typedef struct _DynaArrayHead_t {
	size_t growby;
	size_t reserve;
	size_t filled;
	void (*free_func) (void *);
}DynaArrayHead_t;

static inline DynaArray UNUSED da_create(size_t reserve, void free_func(void *))  {
	DynaArrayHead_t *headp = malloc (sizeof(DynaArrayHead_t) + sizeof(void *) * reserve );
	headp->reserve = reserve;
	headp->growby= reserve;
	headp->filled = 0;

	headp->free_func = free_func;
	memset (headp+1, 0, sizeof(void *)*reserve);
	return (DynaArray)(headp +1);
}
static inline void  UNUSED da_destroy(DynaArray *ap) {
	DynaArrayHead_t *headp = (DynaArrayHead_t *)*ap;
	headp = headp-1;
	int i=0;
	if (headp->free_func) {
		while ((*ap)[i]!=NULL) {
			headp->free_func((*ap)[i]);
			(*ap)[i] = NULL;
			i++;
		}
	}
	free(headp);
	*ap = NULL;
}

static size_t UNUSED da_add_item(DynaArray *a,void * ptr) {
	DynaArrayHead_t *headp = (DynaArrayHead_t *)*a;
	headp = headp-1;
	if ((headp->reserve - 1) == headp->filled) {
		headp->reserve +=headp->growby;
		headp = realloc(headp, sizeof(DynaArrayHead_t) + sizeof(void *) * headp->reserve);
	}
	*a =(DynaArray) (headp+1);
	(*a)[headp->filled] = ptr;
	headp->filled++;
	(*a)[headp->filled] = NULL;
	return headp->filled -1;
}
static void UNUSED da_foreach(DynaArray ap, void func(void * ptr, void * extrap), void * extrap) {
	int i=0;
	while (ap[i]!=NULL) {
		func(ap[i], extrap);
		i++;
	}
}

static size_t UNUSED da_len(DynaArray a) {
	DynaArrayHead_t *headp = (DynaArrayHead_t *)a;
	headp = headp-1;
	return headp->filled;
}
#endif

