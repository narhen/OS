#include <os/scheduler.h>
#include <os/page.h>
#include <os/boot.h>
#include <os/kpanic.h>
#include <os/util.h>
#include <os/print.h>


struct pcb *current_running;

struct pcb the_architect;
struct _tss global_tss = {0};
int tss_index;

DECLARE_LIST(run_queue);
static DECLARE_LIST(sleepers);
static DECLARE_LIST(zombies);

void the_architect_init(void)
{
    extern int init(struct memory_map *, int, unsigned long);
    struct page_descriptor *page;

    current_running = &the_architect;
    the_architect.pid = pid_alloc();
    the_architect.parent = &the_architect;
    the_architect.child = NULL;
    the_architect.status = JOB_RUNNING;
    the_architect.nr_switches = 0;

    INIT_LIST(&the_architect.siblings);
    INIT_LIST(&the_architect.run_queue);

    if ((page = pages_alloc(2, PAGEFL_PINNED)) == NULL)
        kpanic("Failed to allocate stack for the architect");

    the_architect.kern_esp = (long)page->paddr + (PAGE_SIZE * 2);
    the_architect.user_esp = 0;
    /* cr3 is initialized in kernel/start.asm */
    asm("mov    %%cr3, %%eax\n"
        :"=a"(the_architect.p_dir));
    __asm__ __volatile__("mov   %%eax, %%esp\n"
                         "mov   $0, %%ebp\n"
                         "jmp   init\n"
                         :: "a"(the_architect.kern_esp));
    /* dead code */
}

void scheduler_init(void)
{
    global_tss.esp0 = the_architect.kern_esp;
    global_tss.ss0 = KERNEL_DS;
    global_tss.link = 0x28; /* tss segment */
    global_tss.iomap_base_address = sizeof(struct _tss);

    list_add(&the_architect.run_queue, &run_queue);
}

static void dispatch(struct pcb *old)
{
    __asm__ __volatile__("mov   %%esp, %0\n"
                         "mov   %%ebp, %1\n"
                         :"=m"(old->kern_esp), "=m"(old->kern_ebp));
                         
    __asm__ __volatile__("mov   %%eax, %%esp\n"
                         "mov   %%ecx, %%ebp\n"
                         ::"a"(current_running->kern_esp),
                          "c"(current_running->kern_ebp));

    if (current_running->status & JOB_FIRST) {
        current_running->status &= ~JOB_FIRST;
        sti();
        __asm__ __volatile__("jmp   *%%eax\n"
                             ::"a"(current_running->entry));
    }
}

void schedule(void)
{
    struct pcb *old = current_running;

    current_running = list_get_item(run_queue.next, struct pcb, run_queue);
    list_unlink(&current_running->run_queue);
    list_add_tail(&current_running->run_queue, &run_queue);

    ++old->nr_switches;
    old->status &= ~JOB_RUNNING;

    dispatch(old);

    current_running->status |= JOB_RUNNING;
    current_running->status &= ~JOB_SLEEPING;
}

inline void add_job(struct pcb *new_job)
{
    list_add_tail(&new_job->run_queue, &run_queue);
}

void _yield(void)
{
    cli();
    schedule();
    sti();
}
