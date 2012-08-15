#ifndef NULL
#define NULL ((void *)0)
#endif

void kmemset(void *addr, char byte, unsigned n)
{
    unsigned int *start = addr;
    unsigned int set = (byte << 24) | (byte << 16) | (byte << 8) | byte;

    while (start < (unsigned int *)((char *)addr + n))
        *start++ = set;
    if (start == (unsigned int *)((char *)addr + n))
        return;

    char *tmp = (char *)start;
    while (tmp < (char *)((char *)addr + n))
        *tmp++ = byte;
}

void kmemcpy(void *dest, const void *src, int n)
{
    int i;
    int *dptr = (int *)dest;
    int *sptr = (int *)src;
    char *dp, *sp;

    for (i = 0; i < n >> 2; ++i)
        *dptr++ = *sptr++;
    dp = (char *)dptr;
    sp = (char *)sptr;
    for (i = 0; i < n % 4; ++i)
        *dp++ = *sp++;
}

int kmemcmp(void *a1, void *a2, int n)
{
    int i;
    char *p1 = a1, *p2 = a2;

    for (i = 0; i < n && *p1 == *p2; i++, ++p1, ++p2);

    return *p1 - *p2;
}

inline int kstrlen(const char *str)
{
    int ret = 0;
    while (*str++)
        ++ret;

    return ret;
}

inline char *kstrchr(const char *str, char c)
{
    for (; *str; ++str)
        if (*str == c)
            return (char *)str;

    return NULL;
}

inline int kstrcmp(char *dest, const char *src)
{
    while((*dest && *src) && (*dest++ == *src++));

    return *--dest - *--src;
}

inline int kstrcpy(char *dest, const char *src)
{
    int ret = 0;

    while ((*dest++ = *src++))
        ++ret;

    return ret;
}

inline int kstrncpy(char *dest, const char *src, int n)
{
    int ret = 0;

    while (ret < n && (*dest++ = *src++))
        ++ret;

    return ret;
}

/* reverse a string */
inline void kstrrev(char *s)
{
	int c, i, j;

	for (i = 0, j = kstrlen(s) - 1; i < j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}
