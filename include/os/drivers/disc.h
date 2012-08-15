#ifndef __DISK_H
#define __DISK_H

extern void block_read_chs(int cyl, int head, int sector, char *buffer);
extern void block_write_chs(int cyl, int head, int sector, char *buffer);
extern inline void block_write_lba(int lba, char *buffer);
extern inline void block_read_lba(int lba, char *buffer);

#endif
