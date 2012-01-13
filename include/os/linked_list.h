typedef struct llist {
    struct llist *next, *prev;
} llist_t;

static inline void llist_add(llist_t *list, llist_t *new)
{
    new->next = list;
    new->prev = list->prev;
    list->prev->next = new;
    list->prev = new;
}

static inline void llist_del(llist_t *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

#define llist_for_each_node(list, ptr) \
    for (ptr = list; ptr != list; ptr = ptr->next)
