#ifndef __SLAB_H /* start of include guard */
#define __SLAB_H

#include <os/list.h>
#include <os/sync.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#define slab_bufctl(slabp) ((kmem_bufctl_t *)(((struct slab *)(slabp)) + 1))

typedef unsigned int kmem_bufctl_t;

typedef struct slab {
    dllist_t list;

    unsigned int inuse; /* number of objects in use */
    kmem_bufctl_t free; /* next free object, (index in the slab) */

    void *mem; /* pointer to the first object in the slab */
} slab_t;

typedef struct kmem_cache {
    dllist_t list;

    dllist_t slabs_full; /* slablist of full slabs (all objects allocated) */
    dllist_t slabs_partial; /* slablist of partially allocated slabs */
    dllist_t slabs_empty; /* slablist of empty slabs (no objects allocated */

    unsigned int obj_size; /* object size */
    unsigned int num_objs; /* slabsize in objects */
    unsigned int num_pages;  /* slabsize in pages */

    unsigned int flags; /* cache flags */

    void (*ctor)(void *); /* constructor for the objects */
    void (*dtor)(void *); /* destructor for the objects */

    spinlock_t spinlock; /* spinlock to prevent concurrent access to the cache */

    const char *name;

    /* cache status */
    unsigned int bytes_wasted; /* number of wasted bytes per slab */
    unsigned int num_allocs; /* total number of allocs on the cache */
    unsigned int num_active_objs;
    unsigned int highest_active; /* highest number of active objects */
} kmem_cache_t;


/* function declarations */
extern int slab_alloc_init(void); /* is called only once at kernel startup */
extern struct kmem_cache *kmem_cache_create(const char *name, unsigned int flags,
        unsigned int obj_siz, void (*ctor)(void *), void (*dtor)(void *));

extern void *kmem_cache_alloc(struct kmem_cache *cachp);
extern int kmem_cache_destroy(struct kmem_cache *cachep);
extern int kmem_cache_free(struct kmem_cache *cachep, void *ptr);
extern void *kmem_size_caches_alloc(unsigned int size);
extern void *kmalloc(int size);
extern void kfree(void *ptr);

#endif /* end of include guard: __SLAB_H */
