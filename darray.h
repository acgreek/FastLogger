#ifndef DARRAY_H
#define DARRAY_H
#include <string.h> // memset
typedef void * DynaArray ;

typedef struct _DynaArrayHead_t {
	size_t reserve;
	void (*free_func) (void *);
}DynaArrayHead_t;

static inline DynaArray da_create(size_t reserve, void free_func(void *)) {
	DynaArrayHead_t *headp = malloc (sizeof(DynaArrayHead_t) + sizeof(void *) * reserve );
	headp->reserve = reserve;
	headp->free_func = free_func;
	memset (headp+1, 0, sizeof(void *)*reserve);
	return headp +1;
}
static inline void da_destroy(DynaArray *ap) {
	DynaArrayHead_t *headp = (DynaArrayHead_t *)ap;
	headp = headp-1;
	int i=0;
	while (ap[i]!=NULL) {
		headp->free_func(ap[i]);
		ap[i] = NULL;
		i++;
	}
	free(headp);
}

#endif

