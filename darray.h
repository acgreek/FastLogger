#ifndef DARRAY_H
#define DARRAY_H
typedef void * DynaArray ;

typedef struct _DynaArrayHead_t {
	size_t reserve;
	void (*free_func) (void *);
}DynaArrayHead_t;

static inline DynaArray da_create(size_t reserve, void free_func(void *)) {
	DynaArrayHead_t *headp = malloc (sizeof(DynaArrayHead_t) + sizeof(void *) * reserve );
	headp->reserve = reserve;
	headp->free_func = free_func;
	memset (headp + sizeof(DynaArrayHead_t), 0, sizeof(void *)*reserve);
	return headp + sizeof(DynaArrayHead_t);
}
static inline void da_destroy(DynaArray *ap) {

}

#endif

