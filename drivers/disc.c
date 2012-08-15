#include <os/drivers/disc.h>
#include <os/util.h>

/*
;       Reading the harddisk using ports!
;       +-------------------------------+   by qark
;
;
;  This took me months to get working but I finally managed it.
;
;  This code only works for the 286+ so you must detect for 8088's somewhere
;  in your code.
;
;  Technical Information on the ports:
;      Port    Read/Write   Misc
;     ------  ------------ -------------------------------------------------
;       1f0       r/w       data register, the bytes are written/read here
;       1f1       r         error register  (look these values up yourself)
;       1f2       r/w       sector count, how many sectors to read/write
;       1f3       r/w       sector number, the actual sector wanted
;       1f4       r/w       cylinder low, cylinders is 0-1024
;       1f5       r/w       cylinder high, this makes up the rest of the 1024
;       1f6       r/w       drive/head
;                              bit 7 = 1
;                              bit 6 = 0
;                              bit 5 = 1
;                              bit 4 = 0  drive 0 select
;                                    = 1  drive 1 select
;                              bit 3-0    head select bits
;       1f7       r         status register
;                              bit 7 = 1  controller is executing a command
;                              bit 6 = 1  drive is ready
;                              bit 5 = 1  write fault
;                              bit 4 = 1  seek complete
;                              bit 3 = 1  sector buffer requires servicing
;                              bit 2 = 1  disk data read corrected
;                              bit 1 = 1  index - set to 1 each revolution
;                              bit 0 = 1  previous command ended in an error
;       1f7       w         command register
;                            commands:
;                              50h format track
;                              20h read sectors with retry
;                              21h read sectors without retry
;                              22h read long with retry
;                              23h read long without retry
;                              30h write sectors with retry
;                              31h write sectors without retry
;                              32h write long with retry
;                              33h write long without retry
;
;  Most of these should work on even non-IDE hard disks.
;  This code is for reading, the code for writing is the next article.
*/

#define SPT 62
#define HPC 2
#define NUM_HEADS 124

static inline int chs_to_lba(int c, int h, int s)
{
    return (c * NUM_HEADS + h) * SPT + (s - 1);
}

static inline void lba_to_chs(int lba, int *c, int *h, int *s)
{
    *c = lba / (SPT * HPC);
    *h = (lba / SPT) % HPC;
    *s = (lba % SPT) + 1;
}

void block_read_chs(int cyl, int head, int sector, char *buffer)
{
    int i;
    short *ptr = (short *)buffer;

    outb(0x1f6, head);
    outb(0x1f2, 1);
    outb(0x1f3, sector);
    outb(0x1f4, cyl & 0xff);
    outb(0x1f5, (cyl >> 8) & 0xff);
    outb(0x1f7, 0x20);

    while ((inb(0x1f7) & 0x08) == 0);

    for (i = 0; i < 256; ++i)
        *ptr++ = inw(0x1f0);
}

void block_write_chs(int cyl, int head, int sector, char *buffer)
{
    int i;
    short *ptr = (short *)buffer;

    outb(0x1f6, head);
    outb(0x1f2, 1);
    outb(0x1f3, sector);
    outb(0x1f4, cyl & 0xff);
    outb(0x1f5, (cyl >> 8) & 0xff);
    outb(0x1f7, 0x30);

    while ((inb(0x1f7) & 0x08) == 0);

    for (i = 0; i < 256; ++i)
        outw(0x1f0, *ptr++);
}

void block_write_lba(int lba, char *buffer)
{
    int c, h, s;

    lba_to_chs(lba, &c, &h, &s);
    block_write_chs(c, h, s, buffer);
}

void block_read_lba(int lba, char *buffer)
{
    int c, h, s;

    lba_to_chs(lba, &c, &h, &s);
    block_read_chs(c, h, s, buffer);
}
