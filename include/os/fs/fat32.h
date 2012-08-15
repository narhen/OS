#ifndef __FAT32_H
#define __FAT32_H

#include <os/fs/fs.h>

#define CLUSTER_END 0x0ffffff8
#define CLUSTER_BAD 0x0ffffff7

/* directory entry, attribute values */
#define DENT_READ_ONLY 0x01
#define DENT_HIDDEN 0x02
#define DENT_SYSTEM 0x04
#define DENT_VOLUME_ID 0x08
#define DENT_DIRECTORY 0x10
#define DENT_ARCHIVE 0x20
#define DENT_LONG 0x0f

#define _SEEK_SET 1
#define _SEEK_CUR 2
#define _SEEK_END 4

struct __attribute__((packed)) bpb {
    char jmpcodenop[3]; // 0
    char OEM_ID[8]; // 3
    unsigned short bps; /* bytes per sector */ // 11
    unsigned char spc; /* sectors per cluster */ // 13
    unsigned short reserved_sectors; // 14
    unsigned char FATs; // 16
    unsigned short dirents; // 17
    unsigned short num_sectors; // 19
    unsigned char mdt; /* media descriptor type */ // 21
    unsigned short sectors_pr_FAT; /* FAT12/16 only */ // 22
    unsigned short spt; /* sectors pr track */ // 24
    unsigned short num_heads; // 26
    unsigned int hidden_sectors; // 28
    unsigned int num_sectors_large; /* only used if number of sectors // 32
                                     * is > 0xffff and num_sectors is 0 */
    unsigned char ebpb[54]; // 36
};

struct __attribute__((packed)) fat32_ebpb {
	/* extended fat32 stuff */
	unsigned int		table_size_32; // 36
	unsigned short		extended_flags; // 40
	unsigned short		fat_version; // 42
	unsigned int		root_cluster; // 44
	unsigned short		fat_info; // 48
	unsigned short		backup_BS_sector; // 50
	unsigned char 		reserved_0[12]; // 52
	unsigned char		drive_number; // 64
	unsigned char 		reserved_1; // 65
	unsigned char		boot_signature; // 66
	unsigned int 		volume_id; // 67
	unsigned char		volume_label[11]; // 71
	unsigned char		fat_type_label[8]; // 82
};
 
struct __attribute__((packed)) fat16_ebpb {
	/* extended fat12 and fat16 stuff */
	unsigned char		bios_drive_num;
	unsigned char		reserved1;
	unsigned char		boot_signature;
	unsigned int		volume_id;
	unsigned char		volume_label[11];
	unsigned char		fat_type_label[8];
 
};

struct __attribute__((packed)) fat_time {
    unsigned short hour :5;
    unsigned short minutes :6;
    unsigned short seconds :5;
};

struct __attribute__((packed)) fat_date {
    unsigned short year :7;
    unsigned short month :4;
    unsigned short day :5;
};

struct __attribute__((packed)) dirent {
    char filename[11]; // 0
    unsigned char attr; // 11
    char reserved; // 12
    unsigned char creat_time_d; // 13
    struct fat_time creat_time; // 14
    struct fat_date creat_date; // 16
    struct fat_date access_date; // 18
    unsigned short cluster_hi; // 20
    struct fat_time mod_time; // 22
    struct fat_date mod_date; // 24
    unsigned short cluster_lo; // 26
    unsigned int size; // 28
};

struct __attribute__((packed)) dirent_long {
    unsigned char order; /* order of this entry in the sequence of
                          * long filenames */
    short filename_1[5];
    unsigned char attr; /* should be 0x0f */
    unsigned char type;
    unsigned char checksum;
    short filename_2[6];
    short zero;
    short filename_3[2];
};

struct fat_part_desc {
    struct bpb bootrecord;
    int type; /* 32, 16, or 12 */
    int fat_start_lba;
    int root_cyl_start_lba;
    int root_cluster;
    void **fat; /* contains pointers to each sector in the fat.
                 * A pointer may be zero, in which case it is not currently
                 * present in memory.
                 * The size of this array should be bootrecord.sectors_pr_FAT *
                 * sizeof(long *) bytes,
                 * and should be allocated at initialization of the partition
                 * (fat_init()).
                 */
    };

struct fat_fd_meta {
    struct fat_part_desc *prtinfo;
    struct dirent *dir;
    int ccluster; /* current cluster */
    void *cluster;
};

struct stat {
    int size;
    struct fat_time creat_time;
    struct fat_date creat_date;
    struct fat_date access_date;
    struct fat_time mod_time;
    struct fat_date mod_date;
};

extern int fat_init(struct ptable_entry *part, struct fat_part_desc *desc);

#endif
