#ifndef VGA_H
#define VGA_H

#define VIDMEM ((char*)0xB8000)

extern unsigned int cursor;

void scroll();
void printstr(const char *str);
void printclr(const char *str, unsigned char clr);
void putchar(char c, unsigned char clr);
void clearscreen(void);
void disable_cursor();

#endif
