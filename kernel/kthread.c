#include <os/kthread.h>
#include <os/pid.h>
#include <os/scheduler.h>
#include <os/signal.h>

pid_t kthread_create(kthread_task_t *job, void *args)
{
    return -1;
}

void kthread_signal(pid_t pid, signal_t sig)
{

}
