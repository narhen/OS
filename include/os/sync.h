#ifndef __SYNCH_H /* start of include guard */
#define __SYNCH_H

typedef volatile int spinlock_t;

#define DECLARE_SPINLOCK(name) spinlock_t name = 0
inline void spinlock_acquire(spinlock_t *lock);
inline void spinlock_release(spinlock_t *lock);


#endif /* end of include guard: __SYNCH_H */
