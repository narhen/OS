#include <os/scheduler.h>
#include <os/page.h>
#include <os/boot.h>
#include <os/kpanic.h>

extern int init(struct memory_map *, int, unsigned long);

struct pcb *current_running;

struct pcb the_architect;
struct _tss global_tss = {0};

void the_architect_init(void)
{
    struct page_descriptor *page;

    current_running = &the_architect;
    the_architect.pid = pid_alloc();
    the_architect.parent = &the_architect;
    the_architect.child = NULL;
    the_architect.status = JOB_RUNNING;

    if ((page = pages_alloc(2, PAGEFL_PINNED)) == NULL)
        kpanic("Failed to allocate stack for the architect");

    the_architect.kern_esp = (long)page->paddr + (PAGE_SIZE * 2);
    the_architect.user_esp = 0;
    asm("mov    %%cr3, %%eax\n"
        :"=a"(the_architect.p_dir));
    asm volatile("mov   %%eax, %%esp\n"
                 "mov   $0, %%ebp\n"
                 "jmp   init\n"
                 ::"a"(the_architect.kern_esp));
}

void scheduler_init(void)
{
    global_tss.esp0 = the_architect.kern_esp;
}
