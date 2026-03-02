#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEYBOARD_PORT 0x60
extern unsigned char last_scancode;

char keyboard_getchar(void);

#endif
