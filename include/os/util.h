#ifndef __UTIL_H /* start of include guard */
#define __UTIL_H

#ifndef NULL
#define NULL ((void *)0)
#endif

static inline void cpuid(int init_eax, int *out_eax, int *out_ebx,
        int *out_ecx, int *out_edx)
{
    int dummy;
    if (out_eax == NULL)
        out_eax = &dummy;
    if (out_ebx == NULL)
        out_ebx = &dummy;
    if (out_ecx == NULL)
        out_ecx = &dummy;
    if (out_edx == NULL)
        out_edx = &dummy;

    asm volatile("cpuid\n"
            :"=a"(*out_eax), "=b"(*out_ebx), "=c"(*out_ecx), "=d"(*out_edx)
            :"a"(init_eax));
}

static inline void rdtsc(int *hi, int *lo)
{
    asm volatile("rdtsc\n"
            : "=a"(*lo), "=d"(*hi));
}

static inline void outb(short port, char data)
{
    asm volatile("outb %%al, %%dx\n"
            :
            : "a"(data), "d"(port));
}

static inline void inb(int port, char *data)
{
    asm volatile("inb %%dx, %%al\n"
            : "=a"(*data)
            : "d"(port));
}

static inline void wrmsr(int reg, int data_hi, int data_lo)
{
    asm volatile("wrmsr\n"
            :
            : "a"(data_lo), "d"(data_hi), "c"(reg));
}

static inline void rdmsr(int reg, int *data_hi, int *data_lo)
{
    asm volatile("rdmsr\n"
            : "=a"(*data_lo), "=d"(*data_hi)
            : "c"(reg));
}

static inline void cli(void)
{
    asm volatile("cli");
}

static inline void sti(void)
{
    asm volatile("sti");
}

#endif /* end of include guard: __UTIL_H */
