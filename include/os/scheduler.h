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

#define IS_THREAD(pcb) (pcb->user_esp == 0)

#define KERNEL_CS   0x08
#define KERNEL_DS   0x10

struct pcb {
    pid_t pid;
    struct pcb *parent, *child;
    dllist_t siblings;

    unsigned int status;
    void *p_dir;
    struct _tss *tss;
    long kern_esp, user_esp;

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

extern struct pcb *current_running;

#endif
