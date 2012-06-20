#ifndef __PRINT_H /* start of include guard */
#define __PRINT_H

#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

#define FGCOLOR_DEFAULT 0x7
#define FGCOLOR_RED 0x4
#define FGCOLOR_BLACK 0x00

#define BGCOLOR_DEFAULT 0x00
#define BGCOLOR_RED 0x40

extern int vsprintf(char *str, const char *fmt, va_list ap);
extern int sprintf(char *str, const char *fmt, ...);
extern int kprintf(const char *fmt, ...);
extern void kputs(const char *str);
extern inline void set_color(unsigned char c);
extern inline unsigned char get_color(void);
extern inline void set_line(int n);

#endif /* end of include guard: __PRINT_H */
