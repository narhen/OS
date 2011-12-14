#ifndef __BOOT_H /* start of include guard */
#define __BOOT_H

#define MEM_AREA_NORMAL 1
#define MEM_AREA_UNUSABLE 2
#define MEM_AREA_ACPI_RECLAIMABLE 3
#define MEM_AREA_ACPI_NVS_MEM 4
#define MEM_AREA_BAD 5

struct __attribute__((packed)) memory_map {
    unsigned int addr_lo;
    unsigned int addr_hi;

    unsigned int length_lo;
    unsigned int length_hi;

    unsigned int type;
    unsigned int additional;
};

#endif /* end of include guard: __BOOT_H */
