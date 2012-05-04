#include <os/print.h>
#include <os/kpanic.h>
#include <string.h>

inline void get_reg_values(struct cpu_regiters *regs)
{
    __asm__ __volatile__(""
        :"=a"(regs->eax), "=b"(regs->ebx),
         "=c"(regs->ecx), "=d"(regs->edx),
         "=S"(regs->esi), "=D"(regs->edi));
    __asm__ __volatile__("mov   %%ebp, %%eax\n"
                         "mov   %%esp, %%ecx\n"
                         "mov   %%cs, %%edx\n"
                         "mov   %%ds, %%ebx\n"
                         "mov   %%ss, %%esi\n"
                         "mov   %%fs, %%edi\n"
                         :"=a"(regs->ebp), "=c"(regs->esp),
                          "=d"(regs->cs), "=b"(regs->ds), "=S"(regs->ss),
                          "=D"(regs->fs));
    __asm__ __volatile__("mov   %%gs, %%eax\n"
                         "mov   %%es, %%ebx\n"
                         :"=a"(regs->gs), "=b"(regs->es));
}

static void clear_screen(void)
{
    int i;
    char buf[81];
    set_line(0);

    kmemset(buf, ' ', sizeof buf - 1);
    buf[sizeof buf - 1] = 0;
    for (i = 0; i < 25; i++)
        kputs(buf);
    set_line(0);
}

void _kpanic(const char *file, const char *function, int line, const char *msg)
{
    char color = get_color();
    clear_screen();
    set_color(FGCOLOR_BLACK | BGCOLOR_RED);
    kprintf("KERNEL PANIC - %s: %s(), %d", file, function, line);
    kprintf("   %s", msg);
    set_color(color);

    asm("hlt");
}
