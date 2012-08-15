#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#include <os/list.h>

struct keycode {
    dllist_t list;
    unsigned char scancode[6];

    /* the actual keycode */
    unsigned char row :3;
    unsigned char col :5;

    unsigned char flags;
};

/* flags */
#define KEY_PRESSED     (1 << 0)
#define KEY_LSHIFT      (1 << 1)
#define KEY_RSHIFT      (1 << 2)
#define KEY_ALT         (1 << 3)
#define KEY_CTRL        (1 << 4)
#define KEY_CAPSLOCK    (1 << 5)
#define KEY_NUMLOCK     (1 << 6)
#define KEY_SCROLLOCK   (1 << 7)

extern struct keycode *getkeycode(void);
extern char getchar(void);

#endif
