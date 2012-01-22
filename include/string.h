#ifndef __STRING_H
#define __STRING_H

#ifndef NULL
#define NULL ((void *)0)
#endif

extern void kmemset(void *addr, char byte, unsigned n);
extern int kstrlen(const char *str);
extern inline int kstrlen(const char *str);
extern inline char *kstrchr(const char *str, char c);
extern inline int kstrcpy(char *dest, const char *src);
extern inline int kstrncpy(char *dest, const char *src, int n);
extern inline void kstrrev(char *s);

#endif
