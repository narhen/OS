/*
 * init.c  - this is where it all starts
 *
 */

#include <os/kernel.h>
#include <os/util.h>
#include <os/boot.h>
#include <os/page.h>
#include <os/pid.h>
#include <os/scheduler.h>
#include <os/util.h>

/* @arg map: an array over the memory of the computer. NOT SORTED!
 * @arg n: number of elements in the map array
 * @arg max_addr: highest available address
 */
int init(struct memory_map *map, int n, unsigned long max_addr)
{
    static int omg = 0;
    if (!omg) {
        omg = 1;
        paging_init(map, n, max_addr);
        pid_init();
        the_architect_init();
        /* never reached */
    }
    /* now the architect (the parent of all processes) is running */
    scheduler_init();

    while (1);
    return 0;
}
