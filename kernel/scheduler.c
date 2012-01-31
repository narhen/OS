#include <os/scheduler.h>
#include <os/page.h>
#include <os/boot.h>
#include <os/kpanic.h>
#include <os/util.h>


struct pcb *current_running;

struct pcb the_architect;
struct _tss global_tss = {0};
int tss_index;

static dllist_t run_queue;
static dllist_t sleepers;
static dllist_t zombies;

void the_architect_init(void)
{
    extern int init(struct memory_map *, int, unsigned long);
    struct page_descriptor *page;

    current_running = &the_architect;
    the_architect.pid = pid_alloc();
    the_architect.parent = &the_architect;
    the_architect.child = NULL;
    the_architect.status = JOB_RUNNING;

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

    INIT_LIST(&run_queue);
    INIT_LIST(&sleepers);
    INIT_LIST(&zombies);
    list_add(&the_architect.run_queue, &run_queue);
}

void schedule(void)
{
    struct pcb *old = current_running;

    current_running = list_get_item(run_queue.next, struct pcb, run_queue);
    current_running->status |= JOB_RUNNING;

    if (old->status & ~JOB_RUNNING) {
        list_add(&old->run_queue, &run_queue);
        old->status &= ~JOB_RUNNING;
        old->status |= JOB_SLEEPING;
    }
}

void _yield(void)
{
    cli();
    schedule();
    sti();
}
