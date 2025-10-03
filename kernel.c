#include "libs/keyboard.h"
#include "libs/vga.h"
#include "libs/terminal.h"
#include "libs/fs.h"

void faka(void) {
    clearscreen();
    disable_cursor();
    fs_init();
    printstr("Hello, and welcome to the ");
    printclr(" EnvWare dev version", 0x2);
    terminal_run();
}
