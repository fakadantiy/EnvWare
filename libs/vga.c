#include "vga.h"

unsigned int cursor;

static inline void outb(unsigned short port, unsigned char val) {
	__asm__ __volatile__ ("outb %0, %1" : : "a"(val), "Nd"(port));
}


void disable_cursor() {
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

void scroll() {
	for (int i = 0; i < (24 * 80 * 2); i++) {
		VIDMEM[i] = VIDMEM[i + 160];
	}

	for (int i = 24 * 80 * 2; i < 25 * 80 * 2; i += 2) {
		VIDMEM[i] = ' ';
		VIDMEM[i + 1] = 0x07;
	}

	if (cursor >= 80 * 25 * 2) {
		cursor -= 160;
	}
}

void printstr(const char *str) {
    while (*str) {
	putchar(*str++, 0x07);
    }
}

void printclr(const char *str, unsigned char clr) {
    while (*str) {
	putchar(*str++, clr);
    }
}

void putchar(char c, unsigned char clr) {
    if (c == '\n') {
	cursor = (cursor / (80*2) + 1) * (80*2);
    } else {
    	VIDMEM[cursor] = c;
   	VIDMEM[cursor+1] = clr;
    	cursor += 2;
    }

    if (cursor >= 80 * 25 * 2) {
	scroll();
    }
}

void clearscreen(void) {
    unsigned int j = 0;
    while (j < 80*25*2) {
        VIDMEM[j] = ' ';
        VIDMEM[j+1] = 0x07;
        j += 2;
    }
}

