/*
 * init.c  - this is where it all starts
 *
 */

#include <os/kernel.h>
#include <os/util.h>
#include <os/boot.h>
#include <os/page.h>
#include <os/pid.h>
#include <os/scheduler.h>
#include <os/interrupt.h>
#include <os/util.h>
#include <os/print.h>
#include <os/syscall.h>
#include <string.h>

struct {
    unsigned long max_mem;
    unsigned kernel_size;
} statistics;

void *gdt;

static inline void pic_init(void)
{
    outb(0x20, 0x11);
    outb(0x21, 32);
    outb(0x21, 4);
    outb(0x21, 1);
    outb(0x21, 0xfb);

    outb(0xa0, 0x11);
    outb(0xa1, 32 + 8);
    outb(0xa1, 2);
    outb(0xa1, 1);
    outb(0xa1, 0xff);

    extern void irq0_entry(void);
    trap_gate_set(32, (long)irq0_entry);

    outb(0x40, (unsigned char)11932);
    outb(0x40, (unsigned char)11932 >> 8);
}

static inline void syscall_init(void)
{
    extern void syscall_entry(void);
    sys_gate_set(0x63, (long)syscall_entry);

    syscall_table[SC_GETPID] = (void *)getpid;

}

int gdt_gate_set(unsigned int limit, unsigned int base, char access, char flags)
{
    static int next_free = 5;
    struct __attribute__((packed)) {
        unsigned short limit_lo;
        unsigned short base_lo;
        unsigned char base_mid;
        unsigned char access;
        unsigned char limit_hi_flags;
        unsigned char base_hi;
    } *entry = gdt + (next_free << 3);

    entry->limit_lo = (unsigned short)limit & 0xffff;
    entry->base_lo = (unsigned short)base & 0xffff;
    entry->base_mid = (unsigned char)((base >> 16) & 0xff);
    entry->access = (unsigned char)access;
    entry->limit_hi_flags = (unsigned char)((limit >> 16 & 0xf) | ((flags & 0xf) << 4));
    entry->base_hi = (unsigned char)((base >> 24) & 0xff);

    return next_free++;
}

static void clear_screen(void)
{
    int i;
    char buf[80];
    set_line(0);
//    set_color(0x17);

    kmemset(buf, ' ', sizeof buf - 1);
    buf[sizeof buf - 1] = 0;
    for (i = 0; i < 26; i++)
        kputs(buf);
    set_line(0);
}

unsigned long available_memory(struct memory_map *map, int n)
{
    int i;
    unsigned long ret = 0, tmp;
    #define shl(n, i) (n << i)

    for (i = 0; i < n; i++, ++map) {
        tmp = map->addr_lo + map->length_lo;
        if (tmp > ret)
            ret = tmp;

        if (tmp == 0 && map->addr_lo + (map->length_lo - 1) != 0)
            ret = map->addr_lo + (map->length_lo - 1);
    }
    return ret;
}

/* @arg map: an array over the memory of the computer. NOT SORTED!
 * @arg n: number of elements in the map array
 * @arg max_addr: highest available address
 * @arg kern_size: kernel size in blocks (512 bytes)
 *
 * Interrupts are still disabled
 * paging is set up and enabled (first 16 MB is 1-to-1 mapped)
 *
 * TODO: 
 *  - sort the memory map
 */
int init(struct memory_map *map, int n, int kern_size)
{
    static volatile int first_time = 1;
    if (first_time) { /* map is always non-NULL the first time its called */
        sti();
        clear_screen();
        first_time = 0;
        statistics.kernel_size = kern_size;

        statistics.max_mem = available_memory(map, n);

        int i = 0;
        while (i < n) {
            kprintf("map[%d] - addr: 0x%x%x, length: 0x%x%x, type: 0x%x\n",
                    i, map[i].addr_hi, map[i].addr_lo, map[i].length_hi, map[i].length_lo, map[i].type);
            ++i;
        }

        kprintf("Setting up page allocator..\n");
        paging_init(map, n, statistics.max_mem, kern_size);
        kprintf("Initializing pid allocator..\n");
        pid_init();
        kprintf("Starting the architect..\n");
        the_architect_init(); /* never returns */
    }
    /* now the architect (the parent of all processes) is running */
    scheduler_init();
    exceptions_init();
    syscall_init();
    idt_init();
    pic_init();

    tss_index = gdt_gate_set((unsigned int)&global_tss + sizeof(struct _tss),
            (unsigned int)&global_tss , 0x89, 0xc);
    int tmp = tss_index << 3;
    __asm__ __volatile__("ltr   %%ax\n"
            :: "a"(tmp));



    kprintf("Available memory: %u MB\n", statistics.max_mem / 1024 / 1024);
    kprintf("Kernel size: %d bytes\n", statistics.kernel_size * 512);
    kprintf("pid: %d\n", getpid());

    outb(0x21, 0xfe); /* start the PIC */

    tmp = -1;
    while (1) {
        if (!(current_running->nr_switches % 3000) &&
                tmp != current_running->nr_switches) {
            kprintf("%d\n", current_running->nr_switches);
            tmp = current_running->nr_switches;
        }
    }

    return 0;
}
