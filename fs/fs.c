#include <os/slab.h>
#include <os/kpanic.h>
#include <os/drivers/disc.h>
#include <os/fs/fs.h>
#include <os/fs/fat32.h>
#include <os/print.h>

#include <string.h>

struct ptable_entry *ptable;
struct ptable_entry *root_partition;

static struct fat_part_desc partdesc;

static int mount_partition(struct ptable_entry *partition)
{
    if (!partition->type == PTABLE_TYPE_FAT32) {
        fat_init(partition, &partdesc);
        return 0;
    }

    return 1;
}

void fs_init(void)
{
    int i;
    char *ptr, *tableptr;
    char buffer[512];
    struct ptable_entry *pentry;

    if ((ptable = kmalloc(sizeof(struct ptable_entry) * 4)) == NULL)
        kpanic("Failed to allocate space for paritition table!");

    /* read partition table in MBR */
    block_read_lba(0, buffer);
    ptr = &buffer[0x1be];
    tableptr = (char *)ptable;
    for (i = 0; i < sizeof(struct ptable_entry) * 4; i++, tableptr++, ptr++)
        *tableptr = *ptr;

    /* search for the first bootable partition in partition table;
     * that is the root partition */
    root_partition = NULL;
    for (i = 0, pentry = ptable; i < 4; i++) {
        if (pentry->status == PTABLE_BOOTABLE) {
            root_partition = pentry;
            break;
        }
    }
    if (root_partition == NULL || !mount_partition(root_partition))
        kpanic("Failed to mount root partition!");
}

int sys_open(char *filepath, int flags)
{

    return 0;
}

int sys_close(int fd)
{

    return 0;
}

int sys_read(int fd, char *buffer, unsigned int n)
{

    return 0;
}

int sys_write(int fd, char *buffer, unsigned int n)
{

    return 0;
}

int sys_lseek(int fd, int pos, int whence)
{

    return 0;
}
