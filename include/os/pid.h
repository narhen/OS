#ifndef __PID_H
#define __PID_H

typedef int pid_t;

extern inline void pid_init(void);
extern pid_t pid_alloc(void);
extern inline void pid_free(pid_t pid);

#endif
