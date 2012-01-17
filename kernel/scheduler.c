#include <os/scheduler.h>
#include <os/page.h>

struct pcb the_architect;

void the_architect_init(void)
{
    struct page_descriptor *page;

    the_architect.pid = pid_alloc();
    the_architect.parent = &the_architect;
    the_architect.child = NULL;
    the_architect.status = JOB_RUNNING;

    if ((page = pages_alloc(2, PAGEFL_PINNED)) == NULL)
        kpanic("Failed to allocate stack for the architect");

    the_architect.esp = (long)page.paddr + (PAGE_SIZE * 2);

    /* TODO:
     *  - set up page directory and page tables
     *  - set up a tss
     */
}

void scheduler_init(void)
{

}
