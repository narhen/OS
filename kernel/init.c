/*
 * init.c  - this is where it all starts
 *
 */

#include <os/kernel.h>
#include <os/util.h>
#include <os/boot.h>
#include <os/page.h>

/* @arg map: an array over the memory of the computer. NOT SORTED!
 * @arg n: number of elements in the map array
 * @arg max_addr: highest available address
 */
int init(struct memory_map *map, int n, unsigned long max_addr)
{
    paging_init(map, n, max_addr);

    return 0;
}
