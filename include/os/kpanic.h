#ifndef __KPANIC_H
#define __KPANIC_H

typedef struct cpu_regiters {
    unsigned long eax, ebx, ecx, edx, esi, edi, ebp, esp;
    unsigned char cs, ds, ss, fs, gs, es;
} cpu_regiters_t;

extern void _kpanic(const char *file, const char *function, 
        int line, const char *msg);
extern inline void get_reg_values(struct cpu_regiters *regs);
#define kpanic(msg) _kpanic(__FILE__, __FUNCTION__, __LINE__, msg)

#endif
