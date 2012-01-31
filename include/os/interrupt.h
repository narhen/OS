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

extern int interrupt_register(int n, unsigned long address, unsigned short selector,
        unsigned char type, unsigned char priv);
extern void exceptions_init(void);
extern void idt_init(void);

#define trap_gate_set(n, addr) interrupt_register(n, addr, KERNEL_CS, 0xf, 0)
#define int_gate_set(n, addr) interrupt_register(n, addr, KERNEL_CS, 0xe, 0)
#define sys_gate_set(n, addr) interrupt_register(n, addr, KERNEL_CS, 0xe, 3) 

#endif /* end of include guard: __INTERRUPT_H */
