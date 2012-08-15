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
#include <os/kthread.h>
#include <os/slab.h>

#include <os/drivers/keyboard.h>

#include <os/fs/fs.h>

#include <string.h>

#define PIC1    0x20
#define PIC2    0xA0
#define PIC1_COMMAND    PIC1
#define PIC1_DATA   (PIC1+1)
#define PIC2_COMMAND    PIC2
#define PIC2_DATA   (PIC2+1)

struct {
    unsigned long max_mem;
    unsigned kernel_size;
} statistics;

void *gdt;

static inline void irq_set(int line)
{
    short port;
    char value;

    if (line < 8)
        port = PIC1_DATA;
    else if (line < 16)
        port = PIC2_DATA;
    else
        return;

    value = inb(port) | (1 << line);
    outb(port, value);
}

static inline void irq_clear(int line)
{
    short port;
    char value;

    if (line < 8)
        port = PIC1_DATA;
    else if (line < 16)
        port = PIC2_DATA;
    else
        return;

    value = inb(port) & ~(1 << line);
    outb(port, value);
}

static inline void pic_init(void)
{
    outb(PIC1_COMMAND, 0x11);
    outb(PIC1_DATA, 32);
    outb(PIC1_DATA, 4);
    outb(PIC1_DATA, 1);
    outb(PIC1_DATA, 0xfb); /* cascade */

    outb(PIC2_COMMAND, 0x11);
    outb(PIC2_DATA, 32 + 8);
    outb(PIC2_DATA, 2);
    outb(PIC2_DATA, 1);
    outb(PIC2_DATA, 0xff);

    extern void irq0_entry(void);
    extern void irq1_entry(void);
    trap_gate_set(32, (long)irq0_entry);
    trap_gate_set(33, (long)irq1_entry);

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

    for (i = 0; i < 26; i++)
        clear_line(i);
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

static void thread_test(void *args)
{
    int tmp = -1;

    if (args != NULL)
        kprintf("%s\n", args);

    kprintf("New thread %d created\n", getpid());

    while (1) {
        if (!(current_running->nr_switches % 9000) &&
                tmp != current_running->nr_switches) {
            asm("mov %%esp, %0\n"
                    :"=m"(tmp));
            kprintf("pid %d: %d. esp: 0x%x\n", getpid(), current_running->nr_switches, tmp);

            tmp = current_running->nr_switches;
        }
    }
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
        cli();

        exceptions_init();
        idt_init();

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
    pic_init();
    syscall_init();
    slab_alloc_init();
    fs_init();

    tss_index = gdt_gate_set((unsigned int)&global_tss + sizeof(struct _tss),
            (unsigned int)&global_tss , 0x89, 0xc);
    int tmp = tss_index << 3;
    __asm__ __volatile__("ltr   %%ax\n"
            :: "a"(tmp));

    sti(); /* General protection fault sometimes happen here */

    kprintf("Available memory: %u MB\n", statistics.max_mem / 1024 / 1024);
    kprintf("Kernel size: %d bytes\n", statistics.kernel_size * 512);
    kprintf("System is loaded at 0x%x\n", SYS_LOADPOINT);

    kprintf("ptable @ %p\n", ptable);
    kprintf("partition type: 0x%x, start LBA: %d\n", ptable->type, ptable->start_lba);

    irq_clear(0); /* start the PIT */
    irq_clear(1); /* enable keyboard driven interrupts */

    char buffer[81];
    struct __attribute__((packed)) {
        unsigned char secs, mins, hours, year;
    } time;

    get_current_time((int *)&time);
    unsigned long time_start = (time.hours * 60 * 60) + (time.mins * 60) + time.secs;


    while (1) {
        get_current_time((int *)&time);
        ksprintf(buffer, "%d:%d:%d, uptime: %d - threads: %d", time.hours, time.mins, time.secs,
                ((time.hours * 60 * 60) + (time.mins * 60) + time.secs) - time_start, list_size(&run_queue) - 1);
        status_line(buffer);

        char c = getchar();
        if (c != 0)
            kprintf("%c", c);
    }

    return 0;
}
