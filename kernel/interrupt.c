#include <os/interrupt.h>
#include <os/print.h>
#include <os/kpanic.h>

void *syscall_table[256];

static struct idt_descriptor idt_table[100];
static struct __attribute__((packed)) {
    unsigned short limit;
    unsigned int base;
} idtr = {.limit = sizeof(idt_table), .base = (unsigned int)&idt_table};

#define die(error_code) { \
    set_color((get_color() & 0xf0) | FGCOLOR_RED); \
    kprintf_unlocked("%s - error code: %d eip: 0x%x, cs: 0x%x, eflags: 0x%x\n", \
            __FUNCTION__, error_code, eip, cs, eflags); \
    unsigned char *code = (unsigned char *)eip; \
    kprintf_unlocked("code: %x %x %x %x %x %x %x %x %x %x\n", code[0], code[1], code[2], \
            code[3], code[4], code[5], code[6], code[7], code[8], code[9]); \
    struct cpu_regiters regs; \
    get_reg_values(&regs); \
    kprintf_unlocked("eax: 0x%x, ebx: 0x%x, ecx: 0x%x, edx: 0x%x\n" \
            "esi: 0x%x, edi: 0x%x, ebp: 0x%x, esp: 0x%x\n" \
            "cs: 0x%x, ds: 0x%x, ss: 0x%x, fs: 0x%x, gs: 0x%x, es: 0x%x\n", \
            regs.eax, regs.ebx, regs.ecx, regs.edx, regs.esi, regs.edi, \
            regs.ebp, regs.esp, regs.cs, regs.ds, regs.ss, regs.fs, regs.gs, \
            regs.es); \
     long *stack = (long *)regs.esp; \
     kprintf_unlocked("Stack dump:\n"); \
     kprintf_unlocked("%p: 0x%x 0x%x 0x%x 0x%x\n", \
             stack, stack[0], stack[1], stack[2], stack[3]); \
     kprintf_unlocked("%p: 0x%x 0x%x 0x%x 0x%x\n", \
             &stack[4], stack[4], stack[5], stack[6], stack[7]); \
     __asm__("hlt"); \
}


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
void general_protection(long eip, long cs, long eflags, long esp, long ss)
{
    die(0);
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

static inline int _12_to_24(int time)
{
    if (time & 0x80) {
        /* PM */
        time &= 0x7f;
        time += 12;
    } else if (time == 12)
        time = 0;

    return time;
}

/* store current time in 24-hour format in 'time'.
 * bit 0-7 is seconds, bit 8-15 is minutes, bit 16-23 is hours,
 * bit 24 - 31 is year.
 * @return 1 if succesfully updated time. 0 if rtc is in update mode */
int get_current_time(int *time)
{
    int status;
    int tmp, _tmp;
#define NMI_DISABLE 0x80
#define BINARY_FORMAT 4
#define HOUR24_FORMAT 2

    outb(0x70, NMI_DISABLE | 0xa);
    if (inb(0x71) & 0x80)
        return 0; /* rtc is in update mode */

    outb(0x70, NMI_DISABLE | 0xb);
    status = inb(0x71);

    outb(0x70, NMI_DISABLE | 0x9);
    if (status & BINARY_FORMAT)
        *time = inb(0x71) << 24;
    else { /* BCD format */
        tmp = inb(0x71);
        *time = (((tmp / 16) * 10) + (tmp & 0xf)) << 24;
    }

    outb(0x70, NMI_DISABLE | 0x4);
    if (status & BINARY_FORMAT)
        tmp |= inb(0x71);
    else { /* BCD format */
        tmp = inb(0x71);
        _tmp = tmp & 0x80; /* store msb, which indicates PM or AM */
        tmp &= 0x7f;
        tmp = (((tmp / 16) * 10) + (tmp & 0xf));
    }
    if (!(status & HOUR24_FORMAT))
        tmp = _12_to_24(tmp | _tmp);
    *time |= tmp << 16;

    outb(0x70, NMI_DISABLE | 0x2);
    if (status & BINARY_FORMAT)
        *time |= inb(0x71) << 8;
    else { /* BCD format */
        tmp = inb(0x71);
        *time |= (((tmp / 16) * 10) + (tmp & 0xf)) << 8;
    }

    outb(0x70, NMI_DISABLE | 0x0);
    if (status & BINARY_FORMAT)
        *time |= inb(0x71);
    else { /* BCD format */
        tmp = inb(0x71);
        *time |= (((tmp / 16) * 10) + (tmp & 0xf));
    }

#undef NMI_DISABLE
#undef BINARY_FORMAT
#undef HOUR24_FORMAT

    return 1;
}
