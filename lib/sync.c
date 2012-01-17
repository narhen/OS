/*
 * sync.c - synchronization primitives
 */

#include <os/sync.h>

inline void spinlock_acquire(spinlock_t *lock)
{
    while (__sync_lock_test_and_set(lock, 1)) /* gcc builtin */
        while (*lock);
}

inline void spinlock_release(spinlock_t *lock)
{
    __sync_lock_release(lock); /* gcc builtin */
}
