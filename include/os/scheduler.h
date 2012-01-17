#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <os/pid.h>

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
    DECLARE_LIST(siblings);
    unsigned int status;
    void *p_dir;
    struct _tss *tss;
    long kern_esp, user_esp;
    DECLARE_LIST(run_queue);
};

struct __attribute__((packed)) _tss {
    short reserved;
    short link;
    int esp0;
    short reserved;
    short ss0;
    int esp1;
    short reserved;
    short ss1;
    int esp2;
    short reserved;
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
    short reserved;
    short es;
    short reserved;
    short cs;
    short reserved;
    short ss;
    short reserved;
    short ds;
    short reserved;
    short fs;
    short reserved;
    short gs;
    short reserved;
    short ldtr;
    short iopb_offset;
    short reserved;
};

#endif
