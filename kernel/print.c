#include <os/sync.h>
#include <os/print.h>
#include <string.h>

#define VID_MEM 0xb8000

static DECLARE_SPINLOCK(screen_lock);
static int line = 0, cur = 0;
static char color = FGCOLOR_DEFAULT | BGCOLOR_DEFAULT;

inline void set_color(char c)
{
    color = c;
}

inline char get_color(void)
{
    return color;
}

inline void set_line(int n)
{
    line = n;
    cur = 0;
}

void setcur(int i)
{
    cur = i;
}

static inline void scroll(void)
{
    int *src = (int *)VID_MEM + 40, *dest = (int *)VID_MEM;

    /* move everyting one line up */
    for (; src < (int *)VID_MEM + (40 * 25);)
        *dest++ = *src++;
    
    /* clear bottom line */
    src = (int *)VID_MEM + (40 * 24);
    while (src < (int *)VID_MEM + (40 * 25))
        *src++ = (((color << 8) | 0x20) << 16) | (color << 8) | 0x20;
}

void kputs(const char *str)
{
    short *ptr;

//    spinlock_acquire(&screen_lock);
    if (line == 25) {
        scroll();
        line--;
    }

    cur %= 80;
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
    //    spinlock_release(&screen_lock);
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

/* supported formats: %%, %d, %u, %x, %p, %s
 * TODO:
 *  - return number of bytes cpoied into 'str' */
int kvsprintf(char *str, const char *fmt, va_list ap)
{
    char *ptr, *stmp;
    unsigned utmp;
    int itmp;

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
    char buf[128];
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = kvsprintf(buf, fmt, ap);
    va_end(ap);

    kputs(buf);

    return ret;
}
