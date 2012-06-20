#include <os/kthread.h>
#include <os/print.h>
#include <os/page.h>
#include <os/pid.h>
#include <os/scheduler.h>
#include <os/signal.h>

/** Starts a new kernel thread.
 * @param task is a pointer to the function the new thread will execute
 * @param args is a pointer to arguments for the new kernel thread
 */
pid_t kthread_create(kthread_task_t task, void *args)
{
    struct page_descriptor *stack;
    struct pcb *job;
    unsigned long *s_ptr;

    if ((stack = pages_alloc(2, PAGEFL_RECLAIMABLE)) == NULL) {
        kprintf("Unable to allocate stack for new kernel thread!!\n");
        return -1;
    }

    job = pcb_get_top(stack->paddr);

    job->pid = pid_alloc();
    job->status = JOB_FIRST;
    job->nr_switches = 0;

    INIT_LIST(&job->siblings);
    INIT_LIST(&job->run_queue);

    list_add_tail(&current_running->siblings, &job->siblings);
    job->parent = job->child = NULL;

    job->kern_esp = stack_get_bottom(stack->paddr);
    job->user_esp = 0;

    job->tss = &global_tss;
    job->p_dir = the_architect.p_dir;

    s_ptr = (unsigned long *)job->kern_esp;
    *s_ptr = (unsigned long)args;
    job->kern_esp -= sizeof(unsigned long);

    job->entry = (unsigned long)task;

    add_job(job);

    return job->pid;
}

void kthread_signal(pid_t pid, signal_t sig)
{

}
