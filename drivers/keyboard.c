#include <os/drivers/keyboard.h>
#include <os/slab.h>
#include <os/util.h>
#include <os/sync.h>

static DECLARE_LIST(keys);
static DECLARE_SPINLOCK(lock);

static unsigned char capslock = 0;
static unsigned char numlock = 0;
static unsigned char scrollock = 0;

/* scancode set 1 */
static struct {
    unsigned char row :3;
    unsigned char col :5;
    unsigned char pressed;
} scancodes[255] = {
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 1, .col = 1, .pressed = 1},
    {.row = 1, .col = 2, .pressed = 1},
    {.row = 1, .col = 3, .pressed = 1},
    {.row = 1, .col = 4, .pressed = 1}, /* 0x5 */
    {.row = 1, .col = 5, .pressed = 1},
    {.row = 1, .col = 6, .pressed = 1},
    {.row = 1, .col = 7, .pressed = 1},
    {.row = 1, .col = 8, .pressed = 1},
    {.row = 1, .col = 9, .pressed = 1}, /* 0xa */
    {.row = 1, .col = 10, .pressed = 1},
    {.row = 1, .col = 11, .pressed = 1},
    {.row = 1, .col = 12, .pressed = 1},
    {.row = 1, .col = 13, .pressed = 1},
    {.row = 2, .col = 0, .pressed = 1}, /* 0xf */
    {.row = 2, .col = 1, .pressed = 1},
    {.row = 2, .col = 2, .pressed = 1},
    {.row = 2, .col = 3, .pressed = 1},
    {.row = 2, .col = 4, .pressed = 1},
    {.row = 2, .col = 5, .pressed = 1}, /* 0x14 */
    {.row = 2, .col = 6, .pressed = 1},
    {.row = 2, .col = 7, .pressed = 1},
    {.row = 2, .col = 8, .pressed = 1},
    {.row = 2, .col = 9, .pressed = 1},
    {.row = 2, .col = 10, .pressed = 1}, /* 0x19 */
    {.row = 2, .col = 11, .pressed = 1},
    {.row = 2, .col = 12, .pressed = 1},
    {.row = 2, .col = 13, .pressed = 1},
    {.row = 5, .col = 0, .pressed = 1},
    {.row = 3, .col = 1, .pressed = 1}, /* 0x1e */
    {.row = 3, .col = 2, .pressed = 1},
    {.row = 3, .col = 3, .pressed = 1},
    {.row = 3, .col = 4, .pressed = 1},
    {.row = 3, .col = 5, .pressed = 1},
    {.row = 3, .col = 6, .pressed = 1}, /* 0x23 */
    {.row = 3, .col = 7, .pressed = 1},
    {.row = 3, .col = 8, .pressed = 1},
    {.row = 3, .col = 9, .pressed = 1},
    {.row = 3, .col = 10, .pressed = 1},
    {.row = 3, .col = 11, .pressed = 1}, /* 0x28 */
    {.row = 1, .col = 0, .pressed = 1},
    {.row = 4, .col = 0, .pressed = 1},
    {.row = 3, .col = 12, .pressed = 1},
    {.row = 4, .col = 2, .pressed = 1},
    {.row = 4, .col = 3, .pressed = 1}, /* 0x2d */
    {.row = 4, .col = 4, .pressed = 1},
    {.row = 4, .col = 5, .pressed = 1},
    {.row = 4, .col = 6, .pressed = 1},
    {.row = 4, .col = 7, .pressed = 1},
    {.row = 4, .col = 8, .pressed = 1}, /* 0x32 */
    {.row = 4, .col = 9, .pressed = 1},
    {.row = 4, .col = 10, .pressed = 1},
    {.row = 4, .col = 11, .pressed = 1},
    {.row = 4, .col = 12, .pressed = 1},
    {.row = 1, .col = 16, .pressed = 1}, /* 0x37 */
    {.row = 5, .col = 3, .pressed = 1},
    {.row = 5, .col = 4, .pressed = 1},
    {.row = 3, .col = 0, .pressed = 1},
    {.row = 0, .col = 1, .pressed = 1},
    {.row = 0, .col = 2, .pressed = 1}, /* 0x3c */
    {.row = 0, .col = 3, .pressed = 1},
    {.row = 0, .col = 4, .pressed = 1},
    {.row = 0, .col = 5, .pressed = 1},
    {.row = 0, .col = 6, .pressed = 1},
    {.row = 0, .col = 7, .pressed = 1}, /* 0x41 */
    {.row = 0, .col = 8, .pressed = 1},
    {.row = 0, .col = 9, .pressed = 1},
    {.row = 0, .col = 10, .pressed = 1},
    {.row = 1, .col = 14, .pressed = 1},
    {.row = 0, .col = 12, .pressed = 1}, /* 0x46 */
    {.row = 2, .col = 14, .pressed = 1},
    {.row = 2, .col = 15, .pressed = 1},
    {.row = 2, .col = 16, .pressed = 1},
    {.row = 1, .col = 17, .pressed = 1},
    {.row = 3, .col = 15, .pressed = 1}, /* 0x4b */
    {.row = 3, .col = 16, .pressed = 1},
    {.row = 3, .col = 17, .pressed = 1},
    {.row = 3, .col = 18, .pressed = 1},
    {.row = 4, .col = 13, .pressed = 1},
    {.row = 4, .col = 14, .pressed = 1}, /* 0x50 */
    {.row = 4, .col = 15, .pressed = 1},
    {.row = 5, .col = 11, .pressed = 1},
    {.row = 5, .col = 12, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1}, /* 0x55 */
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 11, .pressed = 1},
    {.row = 0, .col = 12, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1}, /* 0x5a */
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1}, /* 0x5f */
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1}, /* 0x64 */
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1}, /* 0x69 */
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1}, /* 0x6e */
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1}, /* 0x73 */
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1}, /* 0x78 */
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1}, /* 0x7d */
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 1},
    {.row = 0, .col = 0, .pressed = 0},
    {.row = 1, .col = 1, .pressed = 0}, /* 0x82 */
    {.row = 1, .col = 2, .pressed = 0},
    {.row = 1, .col = 3, .pressed = 0},
    {.row = 1, .col = 4, .pressed = 0},
    {.row = 1, .col = 5, .pressed = 0},
    {.row = 1, .col = 6, .pressed = 0}, /* 0x87 */
    {.row = 1, .col = 7, .pressed = 0},
    {.row = 1, .col = 8, .pressed = 0},
    {.row = 1, .col = 9, .pressed = 0},
    {.row = 1, .col = 10, .pressed = 0},
    {.row = 1, .col = 11, .pressed = 0}, /* 0x8c */
    {.row = 1, .col = 12, .pressed = 0},
    {.row = 1, .col = 13, .pressed = 0},
    {.row = 2, .col = 0, .pressed = 0},
    {.row = 2, .col = 1, .pressed = 0},
    {.row = 2, .col = 2, .pressed = 0}, /* 0x91 */
    {.row = 2, .col = 3, .pressed = 0},
    {.row = 2, .col = 4, .pressed = 0},
    {.row = 2, .col = 5, .pressed = 0},
    {.row = 2, .col = 6, .pressed = 0},
    {.row = 2, .col = 7, .pressed = 0}, /* 0x96 */
    {.row = 2, .col = 8, .pressed = 0},
    {.row = 2, .col = 9, .pressed = 0},
    {.row = 2, .col = 10, .pressed = 0},
    {.row = 2, .col = 11, .pressed = 0},
    {.row = 2, .col = 12, .pressed = 0}, /* 0x9b */
    {.row = 2, .col = 13, .pressed = 0},
    {.row = 5, .col = 0, .pressed = 0},
    {.row = 3, .col = 1, .pressed = 0},
    {.row = 3, .col = 2, .pressed = 0},
    {.row = 3, .col = 3, .pressed = 0}, /* 0xa0 */
    {.row = 3, .col = 4, .pressed = 0},
    {.row = 3, .col = 5, .pressed = 0},
    {.row = 3, .col = 6, .pressed = 0},
    {.row = 3, .col = 7, .pressed = 0},
    {.row = 3, .col = 8, .pressed = 0}, /* 0xa5 */
    {.row = 3, .col = 9, .pressed = 0},
    {.row = 3, .col = 10, .pressed = 0},
    {.row = 3, .col = 11, .pressed = 0},
    {.row = 1, .col = 0, .pressed = 0},
    {.row = 4, .col = 0, .pressed = 0}, /* 0xaa */
    {.row = 3, .col = 12, .pressed = 0},
    {.row = 4, .col = 2, .pressed = 0},
    {.row = 4, .col = 3, .pressed = 0},
    {.row = 4, .col = 4, .pressed = 0},
    {.row = 4, .col = 5, .pressed = 0}, /* 0xaf */
    {.row = 4, .col = 6, .pressed = 0},
    {.row = 4, .col = 7, .pressed = 0},
    {.row = 4, .col = 8, .pressed = 0},
    {.row = 4, .col = 9, .pressed = 0},
    {.row = 4, .col = 10, .pressed = 0}, /* 0xb4 */
    {.row = 4, .col = 11, .pressed = 0},
    {.row = 4, .col = 12, .pressed = 0},
    {.row = 1, .col = 16, .pressed = 0},
    {.row = 5, .col = 3, .pressed = 0},
    {.row = 5, .col = 4, .pressed = 0}, /* 0xb9 */
    {.row = 3, .col = 0, .pressed = 0},
    {.row = 0, .col = 1, .pressed = 0},
    {.row = 0, .col = 2, .pressed = 0},
    {.row = 0, .col = 3, .pressed = 0},
    {.row = 0, .col = 4, .pressed = 0}, /* 0xbe */
    {.row = 0, .col = 5, .pressed = 0},
    {.row = 0, .col = 6, .pressed = 0},
    {.row = 0, .col = 7, .pressed = 0},
    {.row = 0, .col = 8, .pressed = 0},
    {.row = 0, .col = 9, .pressed = 0}, /* 0xc3 */
    {.row = 0, .col = 10, .pressed = 0},
    {.row = 1, .col = 14, .pressed = 0},
    {.row = 0, .col = 12, .pressed = 0},
    {.row = 2, .col = 14, .pressed = 0},
    {.row = 2, .col = 15, .pressed = 0}, /* 0xc8 */
    {.row = 2, .col = 16, .pressed = 0},
    {.row = 1, .col = 17, .pressed = 0},
    {.row = 3, .col = 14, .pressed = 0},
    {.row = 3, .col = 15, .pressed = 0},
    {.row = 3, .col = 16, .pressed = 0}, /* 0xcd */
    {.row = 3, .col = 17, .pressed = 0},
    {.row = 4, .col = 13, .pressed = 0},
    {.row = 4, .col = 14, .pressed = 0},
    {.row = 4, .col = 15, .pressed = 0},
    {.row = 5, .col = 11, .pressed = 0}, /* 0xd2 */
    {.row = 5, .col = 12, .pressed = 0},
    {.row = 0, .col = 0, .pressed = 0},
    {.row = 0, .col = 0, .pressed = 0},
    {.row = 0, .col = 0, .pressed = 0},
    {.row = 0, .col = 11, .pressed = 0}, /* 0xd7 */
    {.row = 0, .col = 12, .pressed = 0},
};

void get_remaining(struct keycode *key)
{
    int count, num = 6, new;

    for (count = 1; count < num; count++) {
        key->scancode[count] = new = inb(0x60);
        if (count < 2 && key->scancode[0] == 0xe0) {
            if (new != 0x2a || new != 0xb7)
                return;
            else
                num = 4;
        }
    }
}

void kbd_handler(void)
{
    struct keycode *key;
    int index;

    if ((key = kmalloc(sizeof(struct keycode))) == NULL)
        return;

    INIT_LIST(&key->list);
    index = key->scancode[0] = inb(0x60);
    key->flags = 0;

    if (index == 0xe0 || index == 0xe1)
        get_remaining(key);
    else {
        key->row = scancodes[index].row;
        key->col = scancodes[index].col;
        if (scancodes[index].pressed)
            key->flags |=  KEY_PRESSED;
    }

    spinlock_acquire(&lock);
    list_add_tail(&key->list, &keys);
    spinlock_release(&lock);
}

struct keycode *getkeycode(void)
{
    struct keycode *key = NULL;

    if (keys.next != &keys) {
        spinlock_acquire(&lock);

        key = list_get_item(keys.next, struct keycode, list);
        list_unlink(&key->list);

        spinlock_release(&lock);
    } else
        return NULL;

    return key;
}

char *keymap[] = {
    "|1234567890",
    "qwertyuiop",
    "asdfghjkl",
    "<zxcvbnm,.-"
};

char getchar(void)
{
    struct keycode *key = getkeycode();
    char ret;

    if (key == NULL)
        return 0;

    if (!(key->flags & KEY_PRESSED)) {
        kfree(key);
        return 0;
    }

    if (key->row == 1 && key->col < 11)
        ret = keymap[0][key->col];
    else if (key->row == 2 && key->col < 12)
        ret = keymap[1][key->col - 1];
    else if (key->row == 3 && key->col < 12)
        ret = keymap[2][key->col - 1];
    else if (key->row == 4 && key->col < 12)
        ret = keymap[3][key->col - 1];

    kfree(key);

    return ret;
}
