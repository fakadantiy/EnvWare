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
                printstr("\n+----------------------------------+");
                printstr("\n| ls          - file list          |");
                printstr("\n| mk <file>   - create file        |");
                printstr("\n| mkdir <dir> - make a directory   |");
                printstr("\n| rm <file>   - delete file        |");
                printstr("\n| rmdir <dir> - delete a directory |");
                printstr("\n| cd <dir>    - change directory   |");
                printstr("\n| write <f> <text> - write         |");
                printstr("\n| cat <file>  - read               |");
                printstr("\n| info        - system information |");
                printstr("\n| shutdown    - shutdown          |");
                printstr("\n+----------------------------------+");

            } else if (strcmp(cmd, "info") == 0) {
                printstr("\n[EnvWare 1.0 | FAT16 | 32-bit x86]");
            } else if (strcmp(cmd, "shutdown") == 0) {
                outw(0xB004, 0x2000);
                __asm__ __volatile__("cli; hlt");
            } else if (strcmp(cmd, "ls") == 0) {
                fs_ls();
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
                printclr("unknown command\n", 0x4);
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
