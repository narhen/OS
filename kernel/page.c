#include <os/boot.h>
#include <os/page.h>
#include <string.h>

static struct mem_chunk mem_map[1];

#define set_bit(map, n) (((char *)map)[n / 8] |= 1 << (n % 8))
#define clear_bit(map, n) (((char *)map)[n / 8] &= ~(1 << (n % 8)))

#define pgnum_to_page(chunk, n) (&(chunk)->page[(n)])

static inline int get_free(int num, unsigned char *bitmap, unsigned siz)
{
    unsigned mask = 1, tmp, i, j;

    if (num == 0)
        return -1;

    while (num <= 0)
        mask |= 1 << num--;

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

int paging_init(struct memory_map *map, int n, unsigned long max_addr)
{
    int i, j = 0;
    unsigned long addr = 0;

    memset(mem_map[0].bitmap, 0, PAGE_SIZE);
    mem_map[0].chunk_num = 0;
    for (i = 0; i < CHUNK_SIZE; ++i, addr += PAGE_SIZE) {
        mem_map[0].page[i].pagenum = i;
        mem_map[0].page[i].paddr = addr;
        mem_map[0].page[i].num_refs = 0;

        if (map[j].addr_lo + map[j].length_lo < addr)
            j++;

        switch (map[j].type) {
            case MEM_AREA_NORMAL:
                mem_map[i].flag = PAGEFL_RECLAIMABLE;
                break;
            case MEM_AREA_UNUSABLE:
                mem_map[i].flag = PAGEFL_UNUSABLE;
                set_bit(mem_map[i].bitmap, i);
                break;
            default:
                mem_map[i].flag = PAGEFL_UNUSABLE;
                break;
        }
    }

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
        lim = 4096;
    } else {
        chunk = &mem_map[0];
        lim = PAGE_SIZE * 8;
    }

    if ((num = get_free(num, chunk->bitmap, lim)) == -1)
        return NULL;

    page = pgnum_to_page(&mem_map[0], num);
    page->flag |= flag;
    page->num_refs++;

    return page;
}

void *page_alloc(unsigned flag)
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
