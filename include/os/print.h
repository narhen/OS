#ifndef __PRINT_H /* start of include guard */
#define __PRINT_H

#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

#define FGCOLOR_DEFAULT 0x7
#define FGCOLOR_GREEN 0x2
#define FGCOLOR_RED 0x4
#define FGCOLOR_MAGENTA 0x5
#define FGCOLOR_ORANGE 0x6
#define FGCOLOR_GRAY 0x8
#define FGCOLOR_LIGHTBLUE 0x9
#define FGCOLOR_LIGHTGREEN 0xa
#define FGCOLOR_CYAN 0xb
#define FGCOLOR_BLACK 0x00

#define BGCOLOR_DEFAULT 0x00
#define BGCOLOR_RED (FGCOLOR_RED << 4)

extern int kvsprintf(char *str, const char *fmt, va_list ap);
extern int ksprintf(char *str, const char *fmt, ...);
extern int kprintf(const char *fmt, ...);
extern int kprintf_unlocked(const char *fmt, ...);
extern void kputs(const char *str);
extern void kputs_unlocked(const char *str);
extern void status_line(const char *str);
extern inline void clear_line(int n);
extern inline void set_color(unsigned char c);
extern inline unsigned char get_color(void);
extern inline void set_line(int n);

#endif /* end of include guard: __PRINT_H */
