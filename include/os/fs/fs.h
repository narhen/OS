#ifndef __FS_H
#define __FS_H

#define PTABLE_BOOTABLE 0x80
#define PTABLE_TYPE_FAT32 0x0b

struct __attribute__((packed)) ptable_entry {
    unsigned char status;
    unsigned char start_head;
    unsigned char start_sector :6;
    unsigned short start_cylinder :10;
    unsigned char type;
    unsigned char end_head;
    unsigned char end_sector :6;
    unsigned short end_cylinder :10;
    unsigned int start_lba;
    unsigned int num_sectors;
};

struct file_descriptor {
    unsigned int filepos;
    unsigned int flags;
    void *fs_info; /* dedicated to the filesystem driver,
                    * which may use it to store needed data */
};

struct dir {
    char filename[255];
};

extern void fs_init(void);

extern struct ptable_entry *ptable;

#endif
