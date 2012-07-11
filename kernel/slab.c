#include <os/slab.h>
#include <os/page.h>

#define BUFCTL_END ~0

#define SLAB_FULL 0

#define SLINFO_OFF_SLAB 1

#define SLAB_OBJ(mem, index, siz) (void *)(((char *)(mem)) + ((index) * (siz)))

static dllist_t cache_chain;
static struct kmem_cache cache_cache = { .name = "kmem_cache" };
static struct kmem_cache size_caches[] = {
    {.name = "size_8", .obj_size = 8, .num_pages = 1 },
    {.name = "size_16", .obj_size = 16, .num_pages = 1},
    {.name = "size_32", .obj_size = 32, .num_pages = 1},
    {.name = "size_64", .obj_size = 64, .num_pages = 1},
    {.name = "size_128", .obj_size = 128, .num_pages = 1},
    {.name = "size_256", .obj_size = 256, .num_pages = 1},
    {.name = "size_512", .obj_size = 512, .num_pages = 1},
    {.name = "size_1024", .obj_size = 1024, .num_pages = 1},
    {.name = "size_2048", .obj_size = 2048, .num_pages = 1},
    {.name = "size_4096", .obj_size = 4096, .num_pages = 1},
    {.name = "size_8192", .obj_size = 8192, .num_pages = 2},
    {.name = NULL}
};

static inline unsigned int kmem_cache_num_objs(struct kmem_cache *cache);

//static inline struct kmem_cache *kmem_getcache(unsigned long slabaddr)
//{
//    struct page_descriptor *ret = paddr_to_page(slabaddr & ~PAGE_OFFSET);
//
//    return ret ? ret->cache : NULL;
//}

static inline void pageclr(unsigned int *addr)
{
    unsigned int *end;

    end = addr + (4096 >> 2);

    while (addr < end)
        *addr++ = 0;
}

static inline void kmem_bufctl_init(kmem_bufctl_t *ptr, unsigned int n)
{
    unsigned int i;
    for (i = 0; i < n - 1; i++, ++ptr)
        *ptr = i + 1;
    *ptr = BUFCTL_END;
}

static inline struct slab *slab_descriptor_alloc(unsigned int size)
{
    struct kmem_cache *tmp = size_caches;
    for (; tmp; ++tmp) {
        if (size > tmp->obj_size)
            continue;

        return kmem_cache_alloc(tmp);
    }

    return NULL;
}

static int kmem_cache_grow(struct kmem_cache *cache)
{
    struct slab *slabp;
    struct slab *newslab;
    struct page_descriptor *page;

    /* if slab info is on the slab itself */
    if (!(cache->flags & SLINFO_OFF_SLAB)) {
        page = page_alloc(PAGEFL_SLAB);
//        page->cache = cache;
        newslab = (struct slab *)page->paddr;
        pageclr((unsigned int *)newslab);
        newslab->mem = ((char *)newslab) + (sizeof(struct slab) + 
                (sizeof(kmem_bufctl_t) * cache->num_objs));

    } else {
        /* slab is stored off-slab. allocate space for it from one of the size_N
         * caches */
        newslab = slab_descriptor_alloc(sizeof(struct slab) +
                (sizeof(kmem_bufctl_t) * cache->num_objs));
        if (newslab == NULL)
            return 0; /* error */
        pageclr((newslab->mem = (void *)((struct page_descriptor *)page_alloc(PAGEFL_SLAB))->paddr));
    }
    newslab->inuse = newslab->free = 0;
    INIT_LIST(&(newslab->list));

    /* initialize the bufctl array */
    kmem_bufctl_init(slab_bufctl(newslab), cache->num_objs);

    spinlock_acquire(&cache->spinlock);

    slabp = cache->slabs_empty;
    if (slabp == NULL) {
        cache->slabs_empty = newslab;
    } else {
        list_add_tail(&newslab->list, &slabp->list);
    }

    if (cache->ctor) {
        char *ptr;
        for (ptr = newslab->mem;
                ptr < ((char *)newslab->mem) + (cache->num_objs * cache->obj_size);
                ptr += cache->obj_size)
            cache->ctor(ptr);
    }
    spinlock_release(&cache->spinlock);

    return 1;
}

/* update a slabs bufctl after an object free.
 * @return 1 on success, -1 on error */
static inline void kmem_update_bufctl_free(struct slab *slabp, unsigned int ind)
{
    slab_bufctl(slabp)[ind] = slabp->free;
    slabp->free = ind;
}

/* update a slabs bufctl after an object allocation.
 * @return 1 on success, -1 on error, SLAB_FULL if slab is full */
static inline int kmem_update_bufctl_alloc(struct slab *slabp)
{
    slabp->free = slab_bufctl(slabp)[slabp->free];

    return slabp->free == BUFCTL_END ? SLAB_FULL : 1;
}

static inline struct slab *cache_get_slab(struct kmem_cache *cachep, void *ptr)
{
    dllist_t *tmp;
    struct slab *i;

    for_each_item(tmp, &cachep->slabs_partial->list) {
        i = list_get_item(tmp, struct slab, list);
        if (ptr >= i->mem && ptr < i->mem +
                (cachep->num_objs * cachep->obj_size))
            return i;
    }

    for_each_item(tmp, &cachep->slabs_full->list) {
        i = list_get_item(tmp, struct slab, list);
        if (ptr >= i->mem && ptr < i->mem +
                (cachep->num_objs * cachep->obj_size))
            return i;
    }

    return NULL;
}

int kmem_cache_free(struct kmem_cache *cachep, void *ptr)
{
    struct slab *slabp;
    unsigned long objnr;

    slabp = cache_get_slab(cachep, ptr);
    if (slabp == NULL) {
        /* critical error */
        return 0;
    }
    objnr = ((unsigned long)slabp->mem - (unsigned long)ptr)/ cachep->obj_size;

    if (slabp->inuse == cachep->num_objs)
        list_move(&slabp->list, &cachep->slabs_partial->list);

    --slabp->inuse;
    kmem_update_bufctl_free(slabp, objnr);

    return 1;
}

static inline int kmem_slab_descriptor_free(struct kmem_cache *cachep,
        struct slab *slabp)
{
    struct kmem_cache *tmp = size_caches;
    int desc_siz = sizeof(struct slab) +
        (cachep->num_objs * sizeof(kmem_bufctl_t));
    int i;

    for (i = 1; tmp->name; i <<= 1)
        if (desc_siz < tmp->obj_size) {
            kmem_cache_free(tmp, slabp);
            return 1;
        }

    return 0;
}

static inline int kmem_slab_destroy(struct kmem_cache *cachep, struct slab *slabp)
{
    int i;
    if (slabp->inuse) {
        /* not all objects are free()ed */
        return 0;
    }

    list_unlink(&slabp->list);
    if (cachep->flags & SLINFO_OFF_SLAB) {
        for (i = 0; i < cachep->num_pages; ++i)
            page_free((unsigned long *)(((char *)slabp->mem) + (PAGE_SIZE * i)));
        kmem_slab_descriptor_free(cachep, slabp);
    } else {
        page_free((unsigned long *)slabp);
    }
}

int kmem_cache_destroy(struct kmem_cache *cachep)
{
    dllist_t *list;
    struct slab *slabp = NULL;

    if (cachep->slabs_full || cachep->slabs_partial) {
        /* error: some objects in the cache are beeing used */
        return 0;
    }

    for_each_item(list, &cachep->slabs_empty->list) {
        slabp = list_get_item(slabp, struct slab, list);
        kmem_slab_descriptor_free(cachep, slabp);
    }

    kmem_cache_free(&cache_cache, cachep);

    return 1;
}

static inline void *_cache_alloc(struct kmem_cache *cachep)
{
    void *ret;
    struct slab *slabp = cachep->slabs_partial;
    unsigned int objsiz = cachep->obj_size;

    ret = SLAB_OBJ(slabp->mem, slabp->free, objsiz);
    if (kmem_update_bufctl_alloc(slabp) == SLAB_FULL) {
        list_move(&slabp->list, &cachep->slabs_full->list);
    }

    return ret;
}

void *kmem_size_caches_alloc(unsigned int size)
{
    struct kmem_cache *tmp = size_caches;

    while (tmp && tmp->obj_size < size)
        tmp++;

    if (!tmp)
        return NULL;

    return kmem_cache_alloc(tmp);
}

void *kmem_cache_alloc(struct kmem_cache *cachp)
{
    void *ret;

    if (cachp->slabs_empty == NULL && cachp->slabs_partial == NULL)
        kmem_cache_grow(cachp);

    if (cachp->slabs_partial == NULL) {
        if (list_size(&cachp->slabs_empty->list) == 1) {
            cachp->slabs_partial = cachp->slabs_empty;
            cachp->slabs_empty = NULL;
        } else {
            list_move(cachp->slabs_empty->list.next, &cachp->slabs_partial->list);
        }
    }

    ret = _cache_alloc(cachp);

    cachp->num_allocs++;
    cachp->num_active_objs++;
    if (cachp->highest_active < cachp->num_active_objs)
        cachp->highest_active = cachp->num_active_objs;
    cachp->slabs_partial->inuse++;

    return ret;
}

struct kmem_cache *kmem_cache_create(const char *name, unsigned int flags,
        unsigned int obj_siz, void (*ctor)(void *), void (*dtor)(void *))
{
    struct kmem_cache *new_cache;
    unsigned int i, *j;

    new_cache = kmem_cache_alloc(&cache_cache);

    new_cache->name = name;
    new_cache->obj_size = obj_siz;

    /* calculate number of pages to use */
    j = &new_cache->num_pages;
    for (i = PAGE_SIZE, *j = 0; i <= obj_siz; i += PAGE_SIZE, *j++);

    new_cache->flags = flags;
    new_cache->ctor = ctor;
    new_cache->dtor = dtor;

    new_cache->bytes_wasted = kmem_cache_num_objs(new_cache);

    list_add_tail(&cache_chain, &new_cache->list);

    return new_cache;
}

static inline void kmem_setup_size_caches(void)
{
    struct kmem_cache *tmp = size_caches;

    while (tmp->name) {
        spinlock_init(&tmp->spinlock);
        tmp->bytes_wasted = kmem_cache_num_objs(tmp);
        tmp->ctor = tmp->dtor = NULL;
        INIT_LIST(&tmp->list);
        tmp->num_allocs = tmp->highest_active = tmp->num_active_objs = 0;
        tmp->slabs_full = tmp->slabs_partial = tmp->slabs_empty = NULL;
        tmp++;
    }
}

/* Calculate how many objects will fit in a slab.
 * Take in to consideration that the slabinfo (including the buctl array) may be
 * stored in the slab.
 * @return the number of bytes wasted */
static inline unsigned int kmem_cache_num_objs(struct kmem_cache *cache)
{
    /* temporary number of objects to fit in the slab */
    unsigned int *num_objs = &cache->num_objs;
    /* number of bytes left over in the page */
    unsigned int leftover = (cache->num_pages * PAGE_SIZE)  % cache->obj_size;

    *num_objs = (cache->num_pages * PAGE_SIZE) / cache->obj_size;

    /* calculate the number of object to fit into the slab, in order to make
     * room for the slab-data */
    for (; leftover < sizeof(struct slab) + (sizeof(kmem_bufctl_t) * *num_objs);
            leftover -= cache->obj_size, --*num_objs);

    return (PAGE_SIZE * cache->num_pages) -
        (cache->num_objs * cache->obj_size) -
        (sizeof(struct slab) + (sizeof(kmem_bufctl_t) * *num_objs));
}

/* must be called from _start (kernel.c) before any of the other slab functions
 * can be used */
int slab_alloc_init(void)
{
    void tor(void *cache)
    {
        spinlock_init(&(((struct kmem_cache *)cache)->spinlock));
    }
    struct kmem_cache *cache = &cache_cache;

    INIT_LIST(&cache->list);
    cache->slabs_full = cache->slabs_partial = cache->slabs_empty = NULL;
    cache->ctor = tor;
    cache->dtor = tor;

    cache->obj_size = sizeof(struct kmem_cache);
    cache->num_pages = 1;

    cache->flags = 0;

    cache->num_allocs = 0;
    cache->num_active_objs = 0;
    cache->highest_active = 0;

    spinlock_init(&cache->spinlock);

    cache->bytes_wasted = kmem_cache_num_objs(cache);
    INIT_LIST(&cache_chain);

    kmem_cache_grow(cache);

    kmem_setup_size_caches();

    return 1;
}

void *kmalloc(int size)
{
    if (size <= 0)
        return NULL;
    
    if (size > 8192)
        return NULL;
    else if (size > 4096)
        return kmem_cache_alloc(size_caches + 10);
    else if (size > 2048)
        return kmem_cache_alloc(size_caches + 9);
    else if (size > 1024)
        return kmem_cache_alloc(size_caches + 8);
    else if (size > 512)
        return kmem_cache_alloc(size_caches + 7);
    else if (size > 256)
        return kmem_cache_alloc(size_caches + 6);
    else if (size > 128)
        return kmem_cache_alloc(size_caches + 5);
    else if (size > 64)
        return kmem_cache_alloc(size_caches + 4);
    else if (size > 32)
        return kmem_cache_alloc(size_caches + 3);
    else if (size > 16)
        return kmem_cache_alloc(size_caches + 2);
    else if (size > 8)
        return kmem_cache_alloc(size_caches + 1);
    else if (size > 0)
        return kmem_cache_alloc(size_caches);

    return NULL;
}

//void kfree(void *ptr)
//{
//    kmem_cache_free(kmem_getcache((unsigned long)ptr), ptr);
//}
