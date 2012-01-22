#include <os/page.h>
#include <string.h>

static struct mem_chunk mem_map[1];

#define set_bit(map, n) (((char *)map)[n / 8] |= 1 << (n % 8))
#define clear_bit(map, n) (((char *)map)[n / 8] &= ~(1 << (n % 8)))

inline struct page_descriptor *pgnum_to_page(struct mem_chunk *chunk, int n)
{
    return &chunk->page[n];
}

inline struct page_descriptor *paddr_to_page(unsigned long paddr)
{
    return pgnum_to_page(&mem_map[paddr / (CHUNK_SIZE * PAGE_SIZE)],
            paddr / PAGE_SIZE);
}

static int get_free(int num, unsigned char *bitmap, unsigned siz)
{
    unsigned mask = 1, tmp, i, j;
    int tmp2;

    if (num == 0)
        return -1;

    tmp2 = num - 1;
    while (tmp2 > 0)
        mask |= 1 << tmp2--;

    for (i = 0; i < siz; ++i, bitmap++) {
        tmp = mask;
        for (j = 0; j < 8 - num; ++j, tmp <<= 1)
            if ((tmp & *bitmap) == 0) {
                /* set bits and return number */
                *bitmap |= tmp;
                return i * 8 + j;
            }
    }

    return -1;
}

static inline void free_allocated(int num, unsigned char *bitmap)
{
    clear_bit(bitmap, num);
}

int paging_init(struct memory_map *map, int n, unsigned long max_addr, int kern_siz)
{
    int i, j = 0;
    unsigned long addr = 0;
    struct page_descriptor *tmp;

    kmemset(mem_map[0].bitmap, 0, PAGE_SIZE);
    mem_map[0].chunk_num = 0;
    for (i = 0; i < CHUNK_SIZE; ++i, addr += PAGE_SIZE) {
        mem_map[0].page[i].pagenum = i;
        mem_map[0].page[i].paddr = addr;
        mem_map[0].page[i].num_refs = 0;

        if (j < n) {
            if (map[j].addr_lo + map[j].length_lo < addr)
                j++;

            switch (map[j].type) {
                case MEM_AREA_NORMAL:
                    mem_map[0].page[i].flag = PAGEFL_RECLAIMABLE;
                    break;
                case MEM_AREA_UNUSABLE:
                    mem_map[0].page[i].flag = PAGEFL_UNUSABLE;
                    set_bit(mem_map[0].bitmap, i);
                    break;
                default:
                    mem_map[0].page[i].flag = PAGEFL_UNUSABLE;
                    break;
            }
        } else
            mem_map[0].page[i].flag = PAGEFL_UNUSABLE;
    }

    /*  mark all kernel-code pages and page directory/tables PAGEFL_PINNED */
    tmp = paddr_to_page(SYS_LOADPOINT - 0x5000);
    j = kern_siz / 2;
    if (kern_siz % 2)
        ++j;
    j += 5;
    for (i = 0; i < j; ++i, ++tmp)
        tmp->flag |= PAGEFL_PINNED;


    return 1;
}

/* returns NULL on error, address of first physical address otherwise */
struct page_descriptor *pages_alloc(int num, unsigned flag)
{
    struct mem_chunk *chunk;
    struct page_descriptor *page;
    int lim;

    if (flag & PAGEFL_DMA) {
        chunk = &mem_map[0];
        if (chunk->flag & MEM_CHUNKFL_FULL)
            return NULL;
        lim = PAGE_SIZE;
    } else {
        chunk = &mem_map[0];
        lim = PAGE_SIZE;
    }

    if ((num = get_free(num, chunk->bitmap, lim)) == -1)
        return NULL;

    page = pgnum_to_page(&mem_map[0], num);
    page->flag |= flag;
    page->num_refs++;

    return page;
}

inline struct page_descriptor *page_alloc(unsigned flag)
{
    return pages_alloc(1, flag);
}

int page_free(unsigned long *paddr)
{
    int pagenum;
    struct mem_chunk *chunk;
    struct page_descriptor *page;

    chunk = &mem_map[((unsigned long)paddr) / CHUNK_NUM_PAGES];
    pagenum = ((unsigned long)paddr - chunk->page[0].paddr) / PAGE_SIZE;
    page = &chunk->page[pagenum];

    page->num_refs--;
    free_allocated(pagenum, chunk->bitmap);

    return 1;
}
