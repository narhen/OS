#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <os/pid.h>
#include <os/list.h>
#include <os/util.h>

#define JOB_RUNNING         (1 << 0)
#define JOB_SLEEPING        (1 << 1)
#define JOB_INTERRUPTABLE   (1 << 2)
#define JOB_BLOCKED         (1 << 3)
#define JOB_EXITING         (1 << 4)
#define JOB_ZOMBIE          (1 << 5)
#define JOB_FIRST           (1 << 6)

#define KERNEL_CS   0x08
#define KERNEL_DS   0x10

struct pcb {
    pid_t pid;
    struct pcb *parent, *child;
    dllist_t siblings;

    unsigned int status;
    void *p_dir;
    struct _tss *tss;

    long kern_esp, kern_ebp;
    long user_esp, user_ebp;

    unsigned long entry;

    unsigned nr_switches;

    dllist_t run_queue;
};

struct __attribute__((packed)) _tss {
    short link;
    short reserved0;
    int esp0;
    short ss0;
    short reserved1;
    int esp1;
    short ss1;
    short reserved2;
    int esp2;
    short ss2;
    short reserved3;
    int cr3;
    int eip;
    int eflags;
    int eax;
    int ecx;
    int edx;
    int ebx;
    int esp;
    int ebp;
    int esi;
    int edi;
    short es;
    short reserved4;
    short cs;
    short reserved5;
    short ss;
    short reserved6;
    short ds;
    short reserved7;
    short fs;
    short reserved8;
    short gs;
    short reserved9;
    short ldt_segment_selector;
    short reserved10;
    short trap;
    short iomap_base_address;
};

extern void scheduler_init(void);
extern void the_architect_init(void);
extern void schedule(void);
extern inline void add_job(struct pcb *new_job);

extern struct pcb *current_running;
extern struct pcb the_architect;
extern struct _tss global_tss;
extern int tss_index;

extern dllist_t run_queue;

/* This macro assumes the pcb is within the same page as the stack pointer
 * argument*/
#define pcb_get(stack) \
    (struct pcb *)((char *)((unsigned long)(stack) | PAGE_OFFSET) - (sizeof(struct pcb)))

/* This macro assumes the pcb is in the next page, relative to the stack pointer
 * argument */
#define pcb_get_top(stack_top) \
    pcb_get(stack_top + PAGE_SIZE)

/* Get bottom of the kernel stack. Align to 8 byte boundary */
#define stack_get_bottom(stack_ptr) ((unsigned long)((char *)pcb_get_top(stack_ptr) - 1) & ~0xff)

#endif
