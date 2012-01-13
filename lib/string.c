void memset(void *addr, char byte, unsigned n)
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
