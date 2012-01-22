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
}

static inline void syscall_init(void)
{
    extern void syscall_entry(void);
    reg_sys_gate(0x63, (long)syscall_entry);

    syscall_table[SC_GETPID] = (void *)getpid;

}

/* @arg map: an array over the memory of the computer. NOT SORTED!
 * @arg n: number of elements in the map array
 * @arg max_addr: highest available address
 * @arg kern_size: kernel size in blocks (512 bytes)
 *
 * TODO: 
 *  - sort the memory map
 */
int init(struct memory_map *map, int n, unsigned long max_addr, int kern_size)
{
    if (map) { /* map is always non-NULL the first time its called */
        statistics.max_mem = max_addr;
        statistics.kernel_size = kern_size;

        paging_init(map, n, max_addr, kern_size);
        pid_init();
        the_architect_init(); /* never returns */
    }
    /* now the architect (the parent of all processes) is running */
    scheduler_init();
    idt_init();
    exceptions_init();
    syscall_init();

    set_line(1);
    kprintf("Available memory: %u MB", statistics.max_mem / 1024 / 1024);
    kprintf("Kernel size: %d bytes", statistics.kernel_size * 512);
    kprintf("pid: %d                    ", getpid());


    while (1);
    return 0;
}
