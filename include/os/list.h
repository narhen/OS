#ifndef __LIST_H
#define __LIST_H

typedef struct dllist {
    struct dllist *next, *prev;
} dllist_t;

#define DECLARE_LIST(name) dllist_t name; name.next = name.prev = &name

static inline void list_add(dllist_t *list, dllist_t *new)
{

}

static inline void list_remove(dllist_t *node)
{

}

#endif
