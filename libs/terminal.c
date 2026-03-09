#include "keyboard.h"
#include "vga.h"
#include "string.h"
#include "fs.h"
#include "ata.h"

static char buf[256];
static int  len = 0;

static void parse(char *input, int ilen, char **cmd_out, char **arg_out) {
    int i;
    *cmd_out = input;
    *arg_out = 0;
    for (i = 0; i < ilen; i++) {
        if (input[i] == ' ') {
            input[i] = 0;
            *arg_out = &input[i + 1];
            break;
        }
    }
}

void printcolors();

void terminal_run(void) {
    printstr("\n> ");
    char c;
    while (1) {
        c = keyboard_getchar();
        if (!c) continue;

        switch (c) {
        case '\n': {
            buf[len] = 0;
            char *cmd, *arg;
            parse(buf, len, &cmd, &arg);

            if (strcmp(cmd, "help") == 0) {
                printstr("\n.----------------------------------.");
                printstr("\n| ls          - file list          |");
                printstr("\n| mk <file>   - create file        |");
                printstr("\n| mkdir <dir> - make a directory   |");
                printstr("\n| rm <file>   - delete file        |");
                printstr("\n| rmdir <dir> - delete a directory |");
                printstr("\n| cd <dir>    - change directory   |");
                printstr("\n| write <f> <text> - write         |");
                printstr("\n| cat <file>  - read               |");
		        printstr("\n| echo <text> - show text on screen|");
                printstr("\n| info        - system information |");
                printstr("\n| shutdown    - shutdown           |");
                printstr("\n| colors      - show color list    |");
                printstr("\n'----------------------------------'");

            } else if (strcmp(cmd, "info") == 0) {
                printstr("\n[EnvWare 1.0 | FAT16 | 32-bit x86]");
            } else if (strcmp(cmd, "shutdown") == 0) {
                outw(0xB004, 0x2000);
                __asm__ __volatile__("cli; hlt");
            } else if (strcmp(cmd, "ls") == 0) {
                fs_ls();
            } else if (strcmp(cmd, "echo") == 0) {
                printstr("\n");
                printstr(arg);
            } else if (strcmp(cmd, "colors") == 0){
                printcolors();
            } else if (strcmp(cmd, "mk") == 0) {
                if (arg) fs_create(arg);
                else printclr("\nusage: mk <filename>", 0x4);
            } else if (strcmp(cmd, "mkdir") == 0) {
                if (arg) fs_mkdir(arg);
                else printclr("\nusage: mkdir <dirname>", 0x4);
            } else if (strcmp(cmd, "rm") == 0) {
                if (arg) fs_delete(arg);
                else printclr("\nusage: rm <filename>", 0x4);
            } else if (strcmp(cmd, "rmdir") == 0) {
                if (arg) fs_rmdir(arg);
                else printclr("\nusage: rmdir <dirname>", 0x4);
            } else if (strcmp(cmd, "cd") == 0) {
                if (arg) fs_cd(arg);
                else printclr("\nusage: cd <dirname>", 0x4);
            } else if (strcmp(cmd, "cat") == 0) {
                if (arg) fs_read(arg);
                else printclr("\nusage: cat <filename>", 0x4);
            } else if (strcmp(cmd, "write") == 0) {
                if (arg) {
                    char *space = 0;
                    int i;
                    for (i = 0; arg[i]; i++) {
                        if (arg[i] == ' ') { space = &arg[i]; break; }
                    }
                    if (space) {
                        *space = 0;
                        char *fname = arg;
                        char *text  = space + 1;
                        fs_write(fname, text);
                    } else {
                        printclr("usage: write <filename> <text>\n", 0x4);
                    }
                } else {
                    printclr("usage: write <filename> <text>\n", 0x4);
                }

            } else if (cmd[0] != 0) {
                printclr("\nunknown command", 0x4);
            }

            len = 0;
            printstr("\n> ");
            break;
        }

        case '\b':
            if (len > 0) {
                len--;
                cursor -= 2;
                putchar(' ', 0x07);
                cursor -= 2;
            }
            break;

        default:
            if (len < 255) {
                buf[len++] = c;
                putchar(c, 0x07);
            }
        }
    }
}

void printcolors(){
    /* blue */
    printclr("\nA", 0x1); printclr("A", 0x11); printclr("A", 0x12); printclr("A", 0x13);
    printclr("A", 0x14); printclr("A", 0x15); printclr("A", 0x16); printclr("A", 0x17);
    printclr("A", 0x18); printclr("A", 0x19); printclr("A", 0x1A); printclr("A", 0x1B);
    printclr("A", 0x1C); printclr("A", 0x1D); printclr("A", 0x1E); printclr("A", 0x1F);
    printstr(" - 0x1");
    /* green */
    printclr("\nA", 0x2); printclr("A", 0x21); printclr("A", 0x22); printclr("A", 0x23);
    printclr("A", 0x24); printclr("A", 0x25); printclr("A", 0x26); printclr("A", 0x27);
    printclr("A", 0x28); printclr("A", 0x29); printclr("A", 0x2A); printclr("A", 0x2B);
    printclr("A", 0x2C); printclr("A", 0x2D); printclr("A", 0x2E); printclr("A", 0x2F);
    printstr(" - 0x2");
    /* light blue */
    printclr("\nA", 0x3); printclr("A", 0x31); printclr("A", 0x32); printclr("A", 0x33);
    printclr("A", 0x34); printclr("A", 0x35); printclr("A", 0x36); printclr("A", 0x37);
    printclr("A", 0x38); printclr("A", 0x39); printclr("A", 0x3A); printclr("A", 0x3B);
    printclr("A", 0x3C); printclr("A", 0x3D); printclr("A", 0x3E); printclr("A", 0x3F);
    printstr(" - 0x3");
    /* red */
    printclr("\nA", 0x4); printclr("A", 0x41); printclr("A", 0x42); printclr("A", 0x43);
    printclr("A", 0x44); printclr("A", 0x45); printclr("A", 0x46); printclr("A", 0x47);
    printclr("A", 0x48); printclr("A", 0x49); printclr("A", 0x4A); printclr("A", 0x4B);
    printclr("A", 0x4C); printclr("A", 0x4D); printclr("A", 0x4E); printclr("A", 0x4F);
    printstr(" - 0x4");
    /* purple */
    printclr("\nA", 0x5); printclr("A", 0x51); printclr("A", 0x52); printclr("A", 0x53);
    printclr("A", 0x54); printclr("A", 0x55); printclr("A", 0x56); printclr("A", 0x57);
    printclr("A", 0x58); printclr("A", 0x59); printclr("A", 0x5A); printclr("A", 0x5B);
    printclr("A", 0x5C); printclr("A", 0x5D); printclr("A", 0x5E); printclr("A", 0x5F);
    printstr(" - 0x5");
    /* brown */
    printclr("\nA", 0x6); printclr("A", 0x61); printclr("A", 0x62); printclr("A", 0x63);
    printclr("A", 0x64); printclr("A", 0x65); printclr("A", 0x66); printclr("A", 0x67);
    printclr("A", 0x68); printclr("A", 0x69); printclr("A", 0x6A); printclr("A", 0x6B);
    printclr("A", 0x6C); printclr("A", 0x6D); printclr("A", 0x6E); printclr("A", 0x6F);
    printstr(" - 0x6");
    /* gray */
    printclr("\nA", 0x7); printclr("A", 0x71); printclr("A", 0x72); printclr("A", 0x73);
    printclr("A", 0x74); printclr("A", 0x75); printclr("A", 0x76); printclr("A", 0x77);
    printclr("A", 0x78); printclr("A", 0x79); printclr("A", 0x7A); printclr("A", 0x7B);
    printclr("A", 0x7C); printclr("A", 0x7D); printclr("A", 0x7E); printclr("A", 0x7F);
    printstr(" - 0x7");
    /* black */
    printclr("\nA", 0x8); printclr("A", 0x81); printclr("A", 0x82); printclr("A", 0x83);
    printclr("A", 0x84); printclr("A", 0x85); printclr("A", 0x86); printclr("A", 0x87);
    printclr("A", 0x88); printclr("A", 0x89); printclr("A", 0x8A); printclr("A", 0x8B);
    printclr("A", 0x8C); printclr("A", 0x8D); printclr("A", 0x8E); printclr("A", 0x8F);
    printstr(" - 0x8");
    /* also blue */
    printclr("\nA", 0x9); printclr("A", 0x91); printclr("A", 0x92); printclr("A", 0x93);
    printclr("A", 0x94); printclr("A", 0x95); printclr("A", 0x96); printclr("A", 0x97);
    printclr("A", 0x98); printclr("A", 0x99); printclr("A", 0x9A); printclr("A", 0x9B);
    printclr("A", 0x9C); printclr("A", 0x9D); printclr("A", 0x9E); printclr("A", 0x9F);
    printstr(" - 0x9");
    /* lime */
    printclr("\nA", 0xA); printclr("A", 0xA1); printclr("A", 0xA2); printclr("A", 0xA3);
    printclr("A", 0xA4); printclr("A", 0xA5); printclr("A", 0xA6); printclr("A", 0xA7);
    printclr("A", 0xA8); printclr("A", 0xA9); printclr("A", 0xAA); printclr("A", 0xAB);
    printclr("A", 0xAC); printclr("A", 0xAD); printclr("A", 0xAE); printclr("A", 0xAF);
    printstr(" - 0xA");
    /* cyan */
    printclr("\nA", 0xB); printclr("A", 0xB1); printclr("A", 0xB2); printclr("A", 0xB3);
    printclr("A", 0xB4); printclr("A", 0xB5); printclr("A", 0xB6); printclr("A", 0xB7);
    printclr("A", 0xB8); printclr("A", 0xB9); printclr("A", 0xBA); printclr("A", 0xBB);
    printclr("A", 0xBC); printclr("A", 0xBD); printclr("A", 0xBE); printclr("A", 0xBF);
    printstr(" - 0xB");
    /* light red */
    printclr("\nA", 0xC); printclr("A", 0xC1); printclr("A", 0xC2); printclr("A", 0xC3);
    printclr("A", 0xC4); printclr("A", 0xC5); printclr("A", 0xC6); printclr("A", 0xC7);
    printclr("A", 0xC8); printclr("A", 0xC9); printclr("A", 0xCA); printclr("A", 0xCB);
    printclr("A", 0xCC); printclr("A", 0xCD); printclr("A", 0xCE); printclr("A", 0xCF);
    printstr(" - 0xC");
    /* pink */
    printclr("\nA", 0xD); printclr("A", 0xD1); printclr("A", 0xD2); printclr("A", 0xD3);
    printclr("A", 0xD4); printclr("A", 0xD5); printclr("A", 0xD6); printclr("A", 0xD7);
    printclr("A", 0xD8); printclr("A", 0xD9); printclr("A", 0xDA); printclr("A", 0xDB);
    printclr("A", 0xDC); printclr("A", 0xDD); printclr("A", 0xDE); printclr("A", 0xDF);
    printstr(" - 0xD");
    /* yellow */
    printclr("\nA", 0xE); printclr("A", 0xE1); printclr("A", 0xE2); printclr("A", 0xE3);
    printclr("A", 0xE4); printclr("A", 0xE5); printclr("A", 0xE6); printclr("A", 0xE7);
    printclr("A", 0xE8); printclr("A", 0xE9); printclr("A", 0xEA); printclr("A", 0xEB);
    printclr("A", 0xEC); printclr("A", 0xED); printclr("A", 0xEE); printclr("A", 0xEF);
    printstr(" - 0xE");
    /* white */
    printclr("\nA", 0xF); printclr("A", 0xF1); printclr("A", 0xF2); printclr("A", 0xF3);
    printclr("A", 0xF4); printclr("A", 0xF5); printclr("A", 0xF6); printclr("A", 0xF7);
    printclr("A", 0xF8); printclr("A", 0xF9); printclr("A", 0xFA); printclr("A", 0xFB);
    printclr("A", 0xFC); printclr("A", 0xFD); printclr("A", 0xFE); printclr("A", 0xFF);
    printstr(" - 0xF");
}
