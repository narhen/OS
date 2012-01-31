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
    trap_gate_set(0x32, (long)irq0_entry);
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
    char buf[81];
    set_line(0);

    kmemset(buf, ' ', sizeof buf - 1);
    buf[sizeof buf - 1] = 0;
    for (i = 0; i < 25; i++)
        kputs(buf);
    set_line(0);
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
int init(struct memory_map *map, int n, unsigned long max_addr, int kern_size)
{
    static volatile int first_time = 1;
    if (first_time) { /* map is always non-NULL the first time its called */
        sti();
        first_time = 0;
        statistics.max_mem = max_addr;
        statistics.kernel_size = kern_size;

        clear_screen();
        kprintf("init() @ %p", init);
        kprintf("Setting up page allocator..");
        paging_init(map, n, max_addr, kern_size);
        kprintf("Initializing pid allocator..");
        pid_init();
        kprintf("Starting the architect..");
        the_architect_init(); /* never returns */
    }
    /* now the architect (the parent of all processes) is running */
    scheduler_init();
    exceptions_init();
    syscall_init();
    idt_init();

    tss_index = gdt_gate_set((unsigned int)&global_tss + sizeof(struct _tss),
            (unsigned int)&global_tss , 0x89, 0xc);
    int tmp = tss_index << 3;
    __asm__ __volatile__("ltr   %%ax\n"
            :: "a"(tmp));

    kprintf("Available memory: %u MB", statistics.max_mem / 1024 / 1024);
    kprintf("Kernel size: %d bytes", statistics.kernel_size * 512);
    kprintf("pid: %d                    ", getpid());

    while (1); /* double fault!? D: */
    return 0;
}
