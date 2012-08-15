#ifndef __LIST_H
#define __LIST_H

typedef struct dllist {
    struct dllist *next, *prev;
} dllist_t;

#define INIT_LIST(name) (name)->next = (name)->prev = (name)
#define DECLARE_LIST(name) dllist_t name = {&name, &name};

#define for_each_item(ptr, list) \
    for ((ptr) = (list)->next; (ptr) != (list); (ptr) = (ptr)->next)

#define list_get_item(item, type, member) \
    ((type *)((char *)(item) - (unsigned long)(&((type *)0)->member))) \


static inline void list_add_tail(dllist_t *new, dllist_t *list)
{
    new->next = list;
    new->prev = list->prev;
    list->prev->next = new;
    list->prev = new;
}

static inline void list_add_head(dllist_t *new, dllist_t *list)
{
    list_add_tail(new, list->next);
}

static inline void list_add(dllist_t *new, dllist_t *list)
{
    list_add_tail(new, list);
}


static inline void list_unlink(dllist_t *item)
{
    item->prev->next = item->next;
    item->next->prev = item->prev;
    item->next = item->prev = item;
}

static inline void list_move(dllist_t *src, dllist_t *dest)
{
    list_unlink(src);
    list_add(src, dest);
}

static inline int list_size(dllist_t *list)
{
    int ret = 0;
    dllist_t *ptr;

    for_each_item(ptr, list) {
        ++ret;
    }

    return ret;
}
#endif
