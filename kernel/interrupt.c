#include <os/interrupt.h>

void *syscall_table[256];

static struct idt_descriptor idt_table[100];
static struct {
    unsigned short limit;
    unsigned int base;
} idtr = {.limit = sizeof(idt_table), .base = (unsigned int)idt_table};


int register_interrupt(int n, long address, unsigned short selector,
        unsigned char type, unsigned char priv)
{

    struct idt_descriptor *idt;

    if (n < 0 || n > sizeof(idt_table) / sizeof(struct idt_descriptor) - 1)
        return 0;
    idt = &idt_table[n];

    if (idt->type)
        return 0; /* allready in use */

    idt->selector = selector;
    idt->offset1 = (unsigned short)(address & 0xffff);
    idt->offset2 = (unsigned short)(address >> 16);
    idt->type = type | (priv << 5) | 0x80;

    return 1;
}

/* esp and ss is only valid if the privilege-level are changed */
void divide_error(long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void reserved(long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void nmi_interrupt(long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void int3(long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void overflow(long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void bound(long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void invalid_opcode(long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void device_not_available(long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void double_fault(long error_code, long eip, long cs, long eflags, long esp, long ss)
{
    /* abort */
}

/* esp and ss is only valid if the privilege-level are changed */
void coprosessor_segment_overrun(long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void invalid_tss(long error_code, long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void segment_not_present(long error_code, long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void stack_segment_fault(long error_code, long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void general_protection(long error_code, long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void page_fault(void *args)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void floating_point_error(long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void alignment_check(long error_code, long eip, long cs, long eflags, long esp, long ss)
{
}

/* esp and ss is only valid if the privilege-level are changed */
void machine_check(long eip, long cs, long eflags, long esp, long ss)
{
    /* abort */
}

/* esp and ss is only valid if the privilege-level are changed */
void simd_floating_point_exception(long eip, long cs, long eflags, long esp, long ss)
{
}

void exceptions_init(void)
{
    int i;
    extern void page_fault_entry(void);

    reg_trap_gate(0, (long)divide_error);
    reg_trap_gate(1, (long)reserved);
    reg_trap_gate(2, (long)nmi_interrupt);
    reg_trap_gate(3, (long)int3);
    reg_trap_gate(4, (long)overflow);
    reg_trap_gate(5, (long)bound);
    reg_trap_gate(6, (long)invalid_opcode);
    reg_trap_gate(7, (long)device_not_available);
    reg_trap_gate(8, (long)double_fault);
    reg_trap_gate(9, (long)coprosessor_segment_overrun);
    reg_trap_gate(10, (long)invalid_tss);
    reg_trap_gate(11, (long)segment_not_present);
    reg_trap_gate(12, (long)stack_segment_fault);
    reg_trap_gate(13, (long)general_protection);
    reg_trap_gate(14, (long)page_fault_entry);
    reg_trap_gate(15, (long)reserved);
    reg_trap_gate(16, (long)floating_point_error);
    reg_trap_gate(17, (long)alignment_check);
    reg_trap_gate(18, (long)machine_check);
    reg_trap_gate(19, (long)simd_floating_point_exception);

    for (i = 20; i < 31; i++)
        reg_trap_gate(i, (long)reserved);
}

void idt_init(void)
{
    /* TODO: register exception handlers */
    int i;

    for (i = 32; i < sizeof(idt_table) / sizeof(struct idt_descriptor); ++i)
        idt_table[i].type = idt_table[i].zero = 0; /* mark as unused */

    __asm__("lidt %0\n"
            :: "m"(idtr));
}
