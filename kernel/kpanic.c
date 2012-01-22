#include <os/print.h>
#include <string.h>

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
    clear_screen();
    set_fgcolor(FGCOLOR_RED);
    kprintf("KERNEL PANIC - %s: %s(), %d", file, function, line);
    kprintf("   %s", msg);
    set_fgcolor(FGCOLOR_DEFAULT);

    asm("hlt");
}
