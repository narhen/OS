#ifndef __LIST_H
#define __LIST_H

#define DECLARE_LIST(name) dllist_t name; name.next = name.prev = &name

typedef struct dllist {
    struct dllist *next, *prev;
} dllist_t;

static inline list_add(dllist_t *list, dllist_t *new)
{

}

static inline list_remove(dllist_t *node)
{

}

#endif
