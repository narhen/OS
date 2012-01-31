#include <os/interrupt.h>
#include <os/print.h>
#include <os/kpanic.h>

void *syscall_table[256];

static struct idt_descriptor idt_table[100];
static struct __attribute__((packed)) {
    unsigned short limit;
    unsigned int base;
} idtr = {.limit = sizeof(idt_table), .base = (unsigned int)&idt_table};

#define die(error_code) \
    set_fgcolor(FGCOLOR_RED); \
    kprintf("%s - error code: %d eip: 0x%x, cs: 0x%x, eflags: 0x%x", \
            __FUNCTION__, error_code, eip, cs, eflags); \
    while (1);


int interrupt_register(int n, unsigned long address, unsigned short selector,
        unsigned char type, unsigned char priv)
{

    struct idt_descriptor *idt;

    if (n < 0 || n > sizeof(idt_table) / sizeof(struct idt_descriptor) - 1)
        return 0;
    idt = &idt_table[n];

    idt->offset1 = (unsigned short)(address & 0xffff);
    idt->selector = selector;
    idt->zero = 0;
    idt->type = type | (priv << 5) | 0x80;
    idt->offset2 = (unsigned short)(address >> 16);

    return 1;
}

/* esp and ss is only valid if the privilege-level is changed */
void divide_error(long eip, long cs, long eflags, long esp, long ss)
{
    die(0);
}

/* esp and ss is only valid if the privilege-level is changed */
void debug(long eip, long cs, long eflags, long esp, long ss)
{
    die(0);
}

/* esp and ss is only valid if the privilege-level is changed */
void nmi_interrupt(long eip, long cs, long eflags, long esp, long ss)
{
    die(0);
}

/* esp and ss is only valid if the privilege-level is changed */
void int3(long eip, long cs, long eflags, long esp, long ss)
{
    die(0);
}

/* esp and ss is only valid if the privilege-level is changed */
void overflow(long eip, long cs, long eflags, long esp, long ss)
{
    die(0);
}

/* esp and ss is only valid if the privilege-level is changed */
void bound(long eip, long cs, long eflags, long esp, long ss)
{
    die(0);
}

/* esp and ss is only valid if the privilege-level is changed */
void invalid_opcode(long eip, long cs, long eflags, long esp, long ss)
{
    die(0);
}

/* esp and ss is only valid if the privilege-level is changed */
void device_not_available(long eip, long cs, long eflags, long esp, long ss)
{
    die(0);
}

/* esp and ss is only valid if the privilege-level is changed */
void double_fault(long eip, long cs, long eflags, long esp, long ss)
{
    /* abort */
    die(0);
}

/* esp and ss is only valid if the privilege-level is changed */
void coprosessor_segment_overrun(long eip, long cs, long eflags, long esp, long ss)
{
    die(0);
}

/* esp and ss is only valid if the privilege-level is changed */
void invalid_tss(long error_code, long eip, long cs, long eflags, long esp, long ss)
{
    die(error_code);
}

/* esp and ss is only valid if the privilege-level is changed */
void segment_not_present(long error_code, long eip, long cs, long eflags, long esp, long ss)
{
    die(error_code);
}

/* esp and ss is only valid if the privilege-level is changed */
void stack_segment_fault(long error_code, long eip, long cs, long eflags, long esp, long ss)
{
    die(error_code);
}

/* esp and ss is only valid if the privilege-level is changed */
void general_protection(long error_code, long eip, long cs, long eflags, long esp, long ss)
{
    die(error_code);
}

/* esp and ss is only valid if the privilege-level is changed */
void page_fault(void *args)
{
    kpanic("ohnoes");
}

void reserved(long eip, long cs, long eflags, long esp, long ss)
{
    die(0);
}

/* esp and ss is only valid if the privilege-level is changed */
void floating_point_error(long eip, long cs, long eflags, long esp, long ss)
{
    die(0);
}

/* esp and ss is only valid if the privilege-level is changed */
void alignment_check(long error_code, long eip, long cs, long eflags, long esp, long ss)
{
    die(error_code);
}

/* esp and ss is only valid if the privilege-level is changed */
void machine_check(long eip, long cs, long eflags, long esp, long ss)
{
    /* abort */
    die(0);
}

/* esp and ss is only valid if the privilege-level is changed */
void simd_floating_point_exception(long eip, long cs, long eflags, long esp, long ss)
{
    die(0);
}

void exceptions_init(void)
{
    int i;
    extern void page_fault_entry(void);
    extern void double_fault_entry(void);

    trap_gate_set(0, (unsigned long)divide_error);
    trap_gate_set(1, (unsigned long)debug);
    trap_gate_set(2, (unsigned long)nmi_interrupt);
    trap_gate_set(3, (unsigned long)int3);
    trap_gate_set(4, (unsigned long)overflow);
    trap_gate_set(5, (unsigned long)bound);
    trap_gate_set(6, (unsigned long)invalid_opcode);
    trap_gate_set(7, (unsigned long)device_not_available);
    trap_gate_set(8, (unsigned long)double_fault_entry);
    trap_gate_set(9, (unsigned long)coprosessor_segment_overrun);
    trap_gate_set(10, (unsigned long)invalid_tss);
    trap_gate_set(11, (unsigned long)segment_not_present);
    trap_gate_set(12, (unsigned long)stack_segment_fault);
    trap_gate_set(13, (unsigned long)general_protection);
    trap_gate_set(14, (unsigned long)page_fault_entry);
    trap_gate_set(15, (unsigned long)reserved);
    trap_gate_set(16, (unsigned long)floating_point_error);
    trap_gate_set(17, (unsigned long)alignment_check);
    trap_gate_set(18, (unsigned long)machine_check);
    trap_gate_set(19, (unsigned long)simd_floating_point_exception);

    for (i = 20; i <= 31; i++)
        trap_gate_set(i, (unsigned long)reserved);
}

void idt_init(void)
{
    int i;

    for (i = 32; i < sizeof(idt_table) / sizeof(struct idt_descriptor); ++i)
        idt_table[i].type = idt_table[i].zero = 0; /* mark as unused */

    __asm__("lidt %0\n"
            :: "m"(idtr));
}