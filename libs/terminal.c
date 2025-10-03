#include "keyboard.h"
#include "vga.h"
#include "string.h"
#include "fs.h"
#include "ata.h"

char buf[128];
int len = 0;

void terminal_run(void) {
    printstr("\n> ");
    char c;
    while (1) {
        c = keyboard_getchar();
        if (!c) continue;
    
    switch (c) {
        case '\n' :
            buf[len] = 0;

            char *cmd = buf;
            char *arg = 0;
            for (int i = 0; i < len; i++) {
                if (buf[i] == ' ') {
                    buf[i] = 0;      // конец строки для команды
                    arg = &buf[i+1]; // аргумент после пробела
                    break;
                }
            }

       if (strcmp(cmd, "help") == 0) {
	printstr("\n-----------------------------");
	printstr("\n| ls - show all files       |");
	printstr("\n| mk - make file            |");
	printstr("\n| mkdir - make directory    |");
	printstr("\n| rm - delete file          |");
	printstr("\n| rmdir - delete directory  |");
	printstr("\n-----------------------------");
       } else if (strcmp(cmd, "info") == 0) {
	printstr("\n[---------------------------------]");
        printstr("\n[ Envware is a 32 bit operating   ]");
        printstr("\n[ system, written on C for x86    ]");
        printstr("\n[ computers with FAT12 file system]");
        printstr("\n[---------------------------------]");
        printstr("\n[ Envware version: dev            ]");
	printstr("\n[ something will be there . . .   ]");
        printstr("\n[---------------------------------]");
       } else if (strcmp(cmd, "shutdown") == 0) {
	outw(0xB004, 0x2000);
    	__asm__ __volatile__("cli; hlt"); 
       } else if (strcmp(cmd, "ls") == 0) {
        fs_ls();
       } else if (strcmp(cmd, "mk") == 0) {
        if (arg) {
            fs_create(arg); 
            printstr("\nCreated file");
        } else printclr("No filename ", 0x4);
       } else if (strcmp(cmd, "mkdir") == 0) {
	if (arg) {
	    fs_mkdir(arg);
	    printstr("\nCreated directory");
	}
       } else if (strcmp(cmd, "rm") == 0) {
        if (arg) { 
            fs_delete(arg); 
            printstr("\nDeleted file");
        } else printclr("\nNo filename ", 0x4);
	} else if (strcmp(cmd, "rmdir") == 0) {
	 if (arg) {
	    fs_rmdir(arg);
	    printstr("\nDeleted directory");
	 } else printclr("\nNo directory name", 0x4);
	} else if (strcmp(cmd, "cd") == 0) {
	 if (arg) {
	    fs_cd(arg);
	 } else printclr("\nNo directory name", 0x4);
            } else {
                printclr("\nUnknown command", 0x4);
            }
            len = 0;
            printstr("\n> ");
       break;
    case '\b':
       if (len > 0) {
        len--;
        cursor -= 2;
        putchar(' ', 0x07);
        cursor -= 2;
       }
       break;
    default:
            if (len < 127) buf[len++] = c;
            putchar(c, 0x07);
    }
    }    
} 
