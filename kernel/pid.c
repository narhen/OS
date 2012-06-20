#include <string.h>
#include <os/kpanic.h>
#include <os/pid.h>
#include <os/page.h>
#include <os/scheduler.h>

#define set_bit(map, n) (((char *)map)[n / 8] |= 1 << (n % 8))
#define clear_bit(map, n) (((char *)map)[n / 8] &= ~(1 << (n % 8)))

static unsigned char *pidmap;
static unsigned maxpid;

inline void pid_init(void)
{
    /* Reserve a page for the pidmap.
     * Which gives us 4096*8=32768 available pids */
    pidmap = (unsigned char *)((struct page_descriptor *)page_alloc(PAGEFL_PINNED))->paddr;
    kmemset(pidmap, 0, PAGE_SIZE);
    maxpid = PAGE_SIZE * 8 - 1;
}

static pid_t next_free(unsigned n)
{
    unsigned char *bitmap;
    unsigned tmp, i, j;

    ++n;
    bitmap = pidmap + (n / 8);

    for (i = n / 8; i < PAGE_SIZE - (n / 8); ++i, bitmap++)
        for (j = n % 8, tmp = 1 << j; j < 8; ++j, tmp <<= 1)
            if (!(tmp & *bitmap))
                return i * 8 + j;

    return -1;
}

pid_t pid_alloc(void)
{
    static int next = 0;
    if ((next = next_free(next)) == -1)
        next = next_free(0);
    
    if (next == -1)
        kpanic("Out of PIDs!!");

    set_bit(pidmap, next);

    return next;
}

inline void pid_free(pid_t pid)
{
    clear_bit(pidmap, pid);
}

pid_t getpid(void)
{
    return current_running->pid;
}
