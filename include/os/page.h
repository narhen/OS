#ifndef __PAGE_H
#define __PAGE_H

#define PAGE_SIZE 4096
#define PAGE_OFFSET 0xfff


#define PAGEFL_RECLAIMABLE  (1 << 0)
#define PAGEFL_PINNED       (1 << 1)
#define PAGEFL_DMA          (1 << 2)
#define PAGEFL_UNUSABLE     (1 << 3)

typedef struct page_descriptor {
    unsigned pagenum;
    unsigned long paddr;
    int num_refs;
    unsigned flag;
} page_descriptor_t;


#define MEM_CHUNKFL_FULL    (1 << 0)

#define CHUNK_SIZE (PAGE_SIZE * 8)
#define CHUNK_NUM_PAGES (PAGE_SIZE * 8)
/* contains 32768 page descriptors, and a bitmap with the same amount of bits */
struct mem_chunk {
    unsigned chunk_num;
    struct page_descriptor page[CHUNK_NUM_PAGES];
    unsigned char bitmap[PAGE_SIZE];
    unsigned flag;
};

extern int paging_init(struct memory_map *, int, unsigned long);
extern struct page_descriptor *pages_alloc(int num, unsigned flag);
extern inline struct page_descriptor *page_alloc(unsigned flag);
extern int page_free(unsigned long *paddr);

extern inline struct page_descriptor *pgnum_to_page(struct mem_chunk *chunk, int n);
extern inline struct page_descriptor *paddr_to_page(unsigned long paddr);

#endif
