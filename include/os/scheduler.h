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
    short reserved0;
    short link;
    int esp0;
    short reserved1;
    short ss0;
    int esp1;
    short reserved2;
    short ss1;
    int esp2;
    short reserved3;
    short ss2;
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
    short reserved4;
    short es;
    short reserved5;
    short cs;
    short reserved6;
    short ss;
    short reserved7;
    short ds;
    short reserved8;
    short fs;
    short reserved9;
    short gs;
    short reserved10;
    short ldtr;
    short iopb_offset;
    short reserved11;
};

extern void scheduler_init(void);
extern void the_architect_init(void);

extern struct pcb *current_running;

#endif
