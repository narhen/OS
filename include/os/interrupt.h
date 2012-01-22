#ifndef __INTERRUPT_H /* start of include guard */
#define __INTERRUPT_H

#include <os/scheduler.h>

struct __attribute__((packed)) idt_descriptor {
    unsigned short offset1;
    unsigned short selector;
    unsigned char zero;
    unsigned char type;
    unsigned short offset2;
};

extern int register_interrupt(int n, long address, unsigned short selector,
        unsigned char type, unsigned char priv);
extern void exceptions_init(void);
extern void idt_init(void);

#define reg_trap_gate(n, addr) register_interrupt(n, addr, KERNEL_CS, 15, 0)
#define reg_int_gate(n, addr) register_interrupt(n, addr, KERNEL_CS, 14, 0)
#define reg_sys_gate(n, addr) register_interrupt(n, addr, KERNEL_CS, 14, 3)

#endif /* end of include guard: __INTERRUPT_H */
