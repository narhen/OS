#ifndef __KTHREAD_H
#define __KTHREAD_H

#include <os/pid.h>

typedef void (*kthread_task_t)(void *);

extern pid_t kthread_create(kthread_task_t, void *);

#endif
