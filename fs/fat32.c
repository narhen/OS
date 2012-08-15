#include <os/fs/fs.h>
#include <os/fs/fat32.h>
#include <os/drivers/disc.h>
#include <os/slab.h>
#include <os/print.h>

#include <string.h>

static int fat_version(struct bpb *b)
{
    int num_clusters = b->num_sectors ? b->num_sectors / b->spc
        : b->num_sectors_large / b->spc;

    if (num_clusters < 4085)
        return 12;
    else if (num_clusters < 65525)
        return 16;
    else
        return 32;
}

static inline int fat_sectors(struct bpb *bootblock)
{
    if (fat_version(bootblock) == 32) {
        struct fat32_ebpb *eb = (struct fat32_ebpb *)&bootblock->ebpb;
        return eb->table_size_32 * bootblock->FATs;
    }
    return bootblock->sectors_pr_FAT * bootblock->FATs;
}

int fat_init(struct ptable_entry *part, struct fat_part_desc *desc)
{
    char tmpb[512];

    block_read_lba(part->start_lba, tmpb);
    kmemcpy(&desc->bootrecord, tmpb, sizeof(struct bpb));

    if (kstrcmp(desc->bootrecord.OEM_ID, "mkdosfs"))
        return 0; /* error */

    desc->root_cyl_start_lba = part->start_lba + desc->bootrecord.reserved_sectors +
        fat_sectors(&desc->bootrecord);
    desc->type = fat_version(&desc->bootrecord);
    desc->fat_start_lba = part->start_lba + desc->bootrecord.reserved_sectors;
    if (desc->type == 32) {
        struct fat32_ebpb *eb = (struct fat32_ebpb *)&desc->bootrecord.ebpb[0];
        desc->root_cluster = eb->root_cluster;
    } else {
        desc->root_cluster = 2;
    }

    desc->fat = kmalloc(desc->bootrecord.sectors_pr_FAT * sizeof(long *));
    if (desc->fat == NULL)
        return 1;

    return 1;
}

static inline int cluster_to_sector(int cluster, struct fat_part_desc *desc)
{
    return ((cluster - 2) * desc->bootrecord.spc) + desc->root_cyl_start_lba;
}

static char *get_filename(struct dirent *d, char buffer[255])
{
    int i, ret = 0;
    char *ptr;
    struct dirent_long *dl = (struct dirent_long *)d;

    /* make sure we actually have long entries */
    if ((dl - 1)->attr != DENT_LONG) {
        kstrcpy(buffer, d->filename);
        if ((ptr = kstrchr(buffer, 0x20)) < &buffer[10])
            *ptr = 0;
        return buffer;
    }

    do {
        --dl;
        for (i = 0; i < 5 && dl->filename_1[i] != 0; ++i)
            buffer[ret++] = (char)dl->filename_1[i];
        if (i < 5 && dl->filename_1[i] == 0)
            break;
        for (i = 0; i < 6 && dl->filename_2[i] != 0; ++i)
            buffer[ret++] = (char)dl->filename_2[i];
        if (i < 6 && dl->filename_2[i] == 0)
            break;
        for (i = 0; i < 2 && dl->filename_3[i] != 0; ++i)
            buffer[ret++] = (char)dl->filename_3[i];
        if (i < 2 && dl->filename_2[i] == 0)
            break;
    } while (!(dl->order & 0x40));

    buffer[ret] = 0;
    return buffer;
}

/* @param cluster: cluster number
 * @param buffer: the buffer to store the cluster
 * @param cluster size: cluster size in sectors
 */
static inline void cluster_read(int cluster, char *buffer, int cluster_size,
        struct fat_part_desc *desc)
{
    int i, sect;

    sect = cluster_to_sector(cluster, desc);
    for (i = 0; i < cluster_size; ++i, ++sect)
        block_read_lba(sect, buffer + (i * 512));
}

int fat_creat(char *filepath)
{
    return 0;
}

/* @param bps: bytes per sector
*/
static int next_cluster(void **fat, int current, struct fat_part_desc *desc)
{
    int index;
    long **fatptr = (long **)fat;
    int bps = desc->bootrecord.bps;

    index = current  / (bps / sizeof(struct dirent));

    if (fatptr[index] == NULL) {
        fatptr[index] = kmalloc(bps);
        if (fatptr[index] == NULL) {
            /* failed to allocate memory */
            return -1;
        }
        block_read_lba(desc->fat_start_lba + index, (void *)fatptr[index]);
    }
    return fatptr[index][((index * bps) / sizeof(struct dirent)) - current] & 0x0fffffff;
}

static struct dirent *search_cluster(char *name, void *cluster, int dircount)
{
    int i;
    char filename[255];
    struct dirent *dir = cluster;

    for (i = 0; i < dircount; ++i, ++dir) {
        if (dir->attr == DENT_LONG || dir->attr == 0)
            continue;
        if (!kstrcmp(name, get_filename(dir, filename)))
            return dir;
    }
    return NULL;
}

    static struct dirent *search_dir(char *name, struct fat_part_desc *desc,
        int start_cluster, int dircount, char *cl)
{
    struct dirent *dir;
    int current_cluster = start_cluster;

    while (current_cluster < CLUSTER_END) {
        cluster_read(current_cluster, cl, desc->bootrecord.spc, desc);
        dir = search_cluster(name, cl, dircount);
        if (dir == NULL) {
            current_cluster = next_cluster(desc->fat, current_cluster,
                    desc);
        } else
            return dir;
    }

    return NULL;
}

/* used for debugging purposes */
static inline void print_buffer(void *dir, int n)
{
    int i;
    unsigned char *ptr = (unsigned char *)dir;

    for (i = 0; i < n; ++i, ++ptr) {
        kprintf("%02x ", *ptr);
        if (!((i + 1) % 16))
            kprintf("\n");
    }
}

static struct dirent *get_dirent(char *filepath, struct fat_part_desc *desc,
        int current_cluster, int dircount)
{
    struct dirent *dir, *ret;
    char cl[desc->bootrecord.spc * desc->bootrecord.bps];
    char *ptr, *name;

    name = filepath + 1;
    ptr = kstrchr(name, '/');
    while (ptr != NULL) {
        *ptr = 0;
        dir = search_dir(name, desc, current_cluster, dircount, cl);
        if (dir != NULL) {
            name = ptr + 1;
            *ptr = '/';
            ptr = kstrchr(name, '/');
            current_cluster = (dir->cluster_hi << 16) | dir->cluster_lo;
        } else {
            /* file does not exist! */
            *ptr = '/';
            return 0;
        }
    }
    /* check current directory for 'name' */
    dir = search_dir(name, desc, current_cluster, dircount, cl);
    if (dir == NULL)
        return NULL;
    ret = kmalloc(sizeof(struct dirent));
    kmemcpy(ret, dir, sizeof(struct dirent));

    return ret;
}

int fat_open(struct file_descriptor *fd, struct fat_part_desc *desc,
        char *filepath, unsigned int flags)
{
    struct fat_fd_meta *meta;
    struct dirent *dir;
    int current_cluster = desc->root_cluster;

    fd->filepos = 0;
    fd->fs_info = kmalloc(sizeof(struct fat_fd_meta));
    meta = fd->fs_info;
    meta->prtinfo = desc;
    meta->cluster = NULL;

    dir = get_dirent(filepath, meta->prtinfo, current_cluster, (desc->bootrecord.spc *
                desc->bootrecord.bps) / sizeof(struct dirent));
    if (dir == NULL) {
        /* file does not exist */
        kfree(fd->fs_info);
        return 0;
    }
    meta->ccluster = (dir->cluster_hi << 16) | dir->cluster_lo;
    meta->dir = dir;
    return 1;
}

int fat_read(struct file_descriptor *fd, void *buffer, unsigned int n)
{
    struct fat_fd_meta *meta = fd->fs_info;
    int cluster_size = meta->prtinfo->bootrecord.spc, i, ret;
    char *bufptr = buffer, *clptr;

    if (meta->dir->attr == DENT_DIRECTORY)
        return -1;

    if (meta->cluster == NULL) {
        meta->cluster = kmalloc(meta->prtinfo->bootrecord.bps *
                meta->prtinfo->bootrecord.spc);
        cluster_read(meta->ccluster, meta->cluster, cluster_size, meta->prtinfo);
    }

    i = fd->filepos % (meta->prtinfo->bootrecord.bps *
            meta->prtinfo->bootrecord.spc);
    clptr = (char *)meta->cluster + i;
    for (ret = 0; ret < n && fd->filepos + i < meta->dir->size; ++i, ++ret) {
        if (i >= cluster_size * meta->prtinfo->bootrecord.bps) {
            meta->ccluster = next_cluster(meta->prtinfo->fat, meta->ccluster,
                    meta->prtinfo);
            if (meta->ccluster >= CLUSTER_END)
                return ret;
            cluster_read(meta->ccluster, meta->cluster, cluster_size,
                    meta->prtinfo);
            fd->filepos += i;
            clptr = meta->cluster;
            i = 0;
        }
        *bufptr++ = *clptr++;
    }
    fd->filepos += ret;
    return ret;
}

int fat_write(int fd, void *buffer, unsigned int n)
{
    return 0;
}

void fat_close(struct file_descriptor *fd)
{
    struct fat_fd_meta *meta = fd->fs_info;

    kfree(meta->dir);
    kfree(meta->cluster);
    kfree(fd->fs_info);
}

static int follow_cluster_chain(void **fat, int start, int n, int clsize,
        struct fat_part_desc *desc)
{
    int i, ncl = start;

    for (i = 0; i < n; i++) {
        ncl = next_cluster(fat, ncl, desc);
        if (ncl >= CLUSTER_END)
            return CLUSTER_END;
    }

    return ncl;
}

int fat_lseek(struct file_descriptor *fd, int offset, int whence)
{
    struct fat_fd_meta *meta = fd->fs_info;
    int cluster_size = meta->prtinfo->bootrecord.spc, tmp;
    int sector_size = meta->prtinfo->bootrecord.bps, ncl;

    if (meta->dir->attr == DENT_DIRECTORY)
        return -1;

    switch (whence) {
        case _SEEK_SET:
            ncl = (meta->dir->cluster_hi << 16) | meta->dir->cluster_lo;
            tmp = fd->filepos + offset / (cluster_size * sector_size);
            if (offset >= cluster_size * sector_size) {
                ncl = follow_cluster_chain(meta->prtinfo->fat, ncl, tmp,
                        cluster_size, meta->prtinfo);
                if (ncl >= CLUSTER_END)
                    return -1;
                meta->ccluster = ncl;
                if (meta->cluster == NULL)
                    meta->cluster = kmalloc(cluster_size * sector_size);
                cluster_read(ncl, meta->cluster, cluster_size, meta->prtinfo);
            }
            fd->filepos = offset;
            break;
        case _SEEK_CUR:
            if (fd->filepos + offset >= cluster_size * sector_size) {
                ncl = meta->ccluster;
                tmp = fd->filepos + offset / (cluster_size * sector_size);
                ncl = follow_cluster_chain(meta->prtinfo->fat, ncl, tmp,
                        cluster_size, meta->prtinfo);
                if (ncl >= CLUSTER_END)
                    return -1;
                meta->ccluster = ncl;
                if (meta->cluster == NULL)
                    meta->cluster = kmalloc(cluster_size * sector_size);
                cluster_read(ncl, meta->cluster, cluster_size, meta->prtinfo);
                fd->filepos += offset;
            } else
                fd->filepos += offset;
            break;
        case _SEEK_END:
            break;
    }
    return 0;
}

int fat_mkdir(char *dirpath)
{
    return 0;
}

int fat_readdir(struct file_descriptor *fd, struct dir *d)
{
    struct fat_fd_meta *meta = fd->fs_info;
    int cluster_size = meta->prtinfo->bootrecord.spc;
    struct dirent *dir;

    if (meta->dir->attr != DENT_DIRECTORY)
        return -1;

    if (meta->cluster == NULL) {
        meta->cluster = kmalloc(meta->prtinfo->bootrecord.bps *
                meta->prtinfo->bootrecord.spc);
        cluster_read(meta->ccluster, meta->cluster, cluster_size, meta->prtinfo);
    }

    dir = meta->cluster;
    dir += fd->filepos;

    while (dir->attr == DENT_LONG || dir->attr == 0) {
        if ((fd->filepos * sizeof(struct dirent)) >= cluster_size * meta->prtinfo->bootrecord.bps) {
            meta->ccluster = next_cluster(meta->prtinfo->fat, meta->ccluster,
                    meta->prtinfo);
            if (meta->ccluster >= CLUSTER_END)
                return 0;
            cluster_read(meta->ccluster, meta->cluster, cluster_size,
                    meta->prtinfo);
            fd->filepos = 0;
        }
        ++dir;
        ++fd->filepos;
    }
    ++fd->filepos;
    get_filename(dir, d->filename);

    return 1;
}

int fat_rmdir(char *dirpath)
{
    return 0;
}

int fat_unlink(char *dirpath)
{
    return 0;
}

int fat_stat(char *filepath, struct stat *info, struct fat_part_desc *desc)
{
    struct dirent *dir;

    dir = get_dirent(filepath, desc, desc->root_cluster,
            (desc->bootrecord.spc * desc->bootrecord.bps) / sizeof(struct dirent));
    if (dir == NULL)
        return -1;

    info->size = dir->size;
    info->creat_time = dir->creat_time;
    info->creat_date = dir->creat_date;
    info->access_date = dir->access_date;
    info->mod_time = dir->mod_time;
    info->mod_date = dir->mod_date;

    kfree(dir);

    return 0;
}
