#include <os/sync.h>
#include <os/print.h>
#include <string.h>

#define VID_MEM 0xb8000

static DECLARE_SPINLOCK(screen_lock);
static volatile int line = 0, cur = 0;
static volatile unsigned char color = FGCOLOR_DEFAULT | BGCOLOR_DEFAULT;

inline void set_color(unsigned char c)
{
    color = c;
}

inline unsigned char get_color(void)
{
    return color;
}

inline void set_line(int n)
{
    line = n;
    cur = 0;
}

inline void clear_line(int n)
{
    int *src;

    if (n > 24)
        return;

    src = (int *)VID_MEM + (40 * n);
    while (src < (int *)VID_MEM + (40 * (n + 1)))
        *src++ = (((color << 8) | 0x20) << 16) | (color << 8) | 0x20;
}

static void scroll(void)
{
    int *src = (int *)VID_MEM + 40, *dest = (int *)VID_MEM;

    /* move everyting one line up */
    for (; src < (int *)VID_MEM + (40 * 24);)
        *dest++ = *src++;
    
    /* clear bottom line */
    clear_line(23);
}

void kputs(const char *str)
{
    short *ptr;

    spinlock_acquire(&screen_lock);
    if (line == 24) {
        scroll();
        line--;
    }

    cur %= 81;
    ptr = (short *)VID_MEM + (80 * line) + cur;
    for (; *str;) {
        if (cur == 80) {
            cur = 0;
            if (++line > 24) {
                scroll();
                --line;
            }
        }
        if (*str == '\n') {
            ptr += 80 - cur;
            cur = 0;
            ++str;
            line++;
            continue;
        } else
            *ptr = (short)(color << 8) | (short)*str;
        ++cur;
        ++ptr;
        ++str;
    }
    spinlock_release(&screen_lock);
}

void kputs_unlocked(const char *str)
{
    short *ptr;

    if (line == 24) {
        scroll();
        line--;
    }

    cur %= 81;
    ptr = (short *)VID_MEM + (80 * line) + cur;
    for (; *str;) {
        if (cur == 80) {
            cur = 0;
            if (++line > 24) {
                scroll();
                --line;
            }
        }
        if (*str == '\n') {
            ptr += 80 - cur;
            cur = 0;
            ++str;
            line++;
            continue;
        } else
            *ptr = (short)(color << 8) | (short)*str;
        ++cur;
        ++ptr;
        ++str;
    }
}

void status_line(const char *str)
{
    int i = 0;
    short *ptr = (short *)VID_MEM + (80 * 24);
    unsigned char def_color = get_color();

    spinlock_acquire(&screen_lock);

    set_color(FGCOLOR_GREEN|BGCOLOR_DEFAULT);
    /* Copy characters to video mapped memory */
    while (*str && i < 80) {
        *ptr = (short)(color << 8) | (short)*str;
        ++str;
        ++ptr;
        ++i;
    }

    /* Fill the rest of the line with spaces */
    while (i < 80) {
        *ptr = (short)(color << 8) | (short)' ';
        ++ptr;
        ++i;
    }

    set_color(def_color);

    spinlock_release(&screen_lock);
}

/* hex to string */
static char *htoa(char *s, unsigned n)
{
    int i = 0 ;
    unsigned tmp;

    do {
        tmp = n & 0xf;
        if (tmp <= 9)
            s[i++] = tmp + '0';
        else
            s[i++] = tmp - 10 + 'a';
    } while ((n >>= 4));
    s[i] = 0;
    kstrrev(s);

    return &s[i];
}

/* unsigned integer to string */
static char *uitoa(char *s, unsigned n)
{
    int i;

    i = 0;
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    s[i] = 0;
    kstrrev(s);

    return &s[i];

}

/* signed integer to string */
static char *sitoa(char *str, int num)
{
    if (num < 0) {
        *str++ = '-';
        return uitoa(str, (unsigned)-num);
    }

    return uitoa(str, (unsigned)num);
}

/* supported formats: %%, %d, %u, %x, %p, %s, %c
 * TODO:
 *  - return number of bytes copied into 'str' */
int kvsprintf(char *str, const char *fmt, va_list ap)
{
    char *ptr, *stmp;
    unsigned utmp;
    int itmp;
    char ctmp;

    for (; *fmt; fmt = ptr + 2) {
        ptr = kstrchr(fmt, '%');
        if (ptr == NULL) {
            kstrcpy(str, fmt);
            return 1;
        }

        str += kstrncpy(str, fmt, (int)ptr - (int)fmt);

        switch (*(ptr + 1)) {
            case 'd': /* signed int */
                itmp = va_arg(ap, int);
                str = sitoa(str, itmp);
                break;
            case 'u': /* unsigned int */
                utmp = va_arg(ap, unsigned);
                str = uitoa(str, utmp);
                break;
            case 'p': /* pointer */
                *str++ = '0';
                *str++ = 'x';
            case 'x': /* hex */
                utmp = va_arg(ap, unsigned);
                str = htoa(str, utmp);
                break;
            case 's': /* string */
                stmp = va_arg(ap, char *);
                str += kstrcpy(str, stmp);
                break;
            case '%':
                *str++ = '%';
                break;
            case 'c':
                ctmp = va_arg(ap, int);
                *str++ = ctmp;
                break;
            default:
                /* unsupported format */
                break;
        }
    }

    return 1;
}

int ksprintf(char *str, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = kvsprintf(str, fmt, ap);
    va_end(ap);

    return ret;
}

int kprintf(const char *fmt, ...)
{
    /* yes yes, a possible buffer overflow.. I know.
     * I'm to lazy to do something about it, atm */
    char buf[1024];
    va_list ap;
    int ret;

    kmemset(buf, 0, sizeof buf);

    va_start(ap, fmt);
    ret = kvsprintf(buf, fmt, ap);
    va_end(ap);

    kputs(buf);

    return ret;
}

int kprintf_unlocked(const char *fmt, ...)
{
    /* yes yes, a possible buffer overflow.. I know.
     * I'm to lazy to do something about it, atm */
    char buf[1024];
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = kvsprintf(buf, fmt, ap);
    va_end(ap);

    kputs_unlocked(buf);

    return ret;
}
